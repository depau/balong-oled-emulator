#include <cassert>
#include <cerrno>
#include <fcntl.h>
#include <signal.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include "subprocess.hpp"

using namespace subprocess::plumbing;

union pipe_fds {
  int fds[2];
  struct {
    int read_fd = -1;
    int write_fd = -1;
  };
};

pid_t subprocess::plumbing::spawn(const std::vector<std::string> &argv,
                                  int *out_stdout_fd,
                                  int *out_stderr_fd,
                                  int *out_stdin_fd) {
  assert(!argv.empty() && !argv[0].empty());

  if (out_stdout_fd != nullptr)
    *out_stdout_fd = -1;
  if (out_stderr_fd != nullptr)
    *out_stderr_fd = -1;
  if (out_stdin_fd != nullptr)
    *out_stdin_fd = -1;

  // Reads on p[0], writes on p.write_fd
  pipe_fds p_stdout{};
  pipe_fds p_stderr{};
  pipe_fds p_stdin{};

  int saved_errno = 0;

  {
    if (out_stdout_fd != nullptr) {
      if (::pipe(p_stdout.fds) != 0) {
        saved_errno = errno;
        goto fail_exit;
      }

      (void) ::fcntl(p_stdout.read_fd, F_SETFD, (::fcntl(p_stdout.read_fd, F_GETFD, 0) | FD_CLOEXEC));
      (void) ::fcntl(p_stdout.write_fd, F_SETFD, (::fcntl(p_stdout.write_fd, F_GETFD, 0) | FD_CLOEXEC));
    }
    if (out_stderr_fd != nullptr && out_stderr_fd != out_stdout_fd) {
      if (::pipe(p_stderr.fds) != 0) {
        saved_errno = errno;
        goto fail_exit;
      }

      (void) ::fcntl(p_stderr.read_fd, F_SETFD, (::fcntl(p_stderr.read_fd, F_GETFD, 0) | FD_CLOEXEC));
      (void) ::fcntl(p_stderr.write_fd, F_SETFD, (::fcntl(p_stderr.write_fd, F_GETFD, 0) | FD_CLOEXEC));
    }
    if (out_stdin_fd != nullptr) {
      if (::pipe(p_stdin.fds) != 0) {
        saved_errno = errno;
        goto fail_exit;
      }

      (void) ::fcntl(p_stdin.read_fd, F_SETFD, (::fcntl(p_stdin.read_fd, F_GETFD, 0) | FD_CLOEXEC));
      (void) ::fcntl(p_stdin.write_fd, F_SETFD, (::fcntl(p_stdin.write_fd, F_GETFD, 0) | FD_CLOEXEC));
    } else {
      const int devnull = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
      if (devnull < 0) {
        saved_errno = errno;
        goto fail_exit;
      }

      p_stdin.read_fd = devnull;
    }

    const pid_t pid = ::fork();
    if (pid < 0) {
      // Fork failed
      saved_errno = errno;
      goto fail_exit;
    }

    // Child
    if (pid == 0) {
      // stdin
      if (::dup2(p_stdin.read_fd, STDIN_FILENO) < 0)
        _exit(127);

      // stdout
      if (out_stdout_fd != nullptr && ::dup2(p_stdout.write_fd, STDOUT_FILENO) < 0)
        _exit(127);

      // stderr
      if (out_stderr_fd != nullptr && ::dup2(p_stderr.write_fd, STDERR_FILENO) < 0)
        _exit(127);

      // Close fds not needed in child
      if (out_stdout_fd != nullptr)
        for (const int fd : p_stdout.fds)
          if (fd >= 0)
            ::close(fd);
      if (out_stderr_fd != nullptr && out_stderr_fd != out_stdout_fd)
        for (const int fd : p_stderr.fds)
          if (fd >= 0)
            ::close(fd);
      if (out_stdin_fd != nullptr)
        for (const int fd : p_stdin.fds)
          if (fd >= 0)
            ::close(fd);

      // Build argv for execvp
      std::vector<char *> cargv;
      cargv.reserve(argv.size() + 1);
      for (size_t i = 0; i < argv.size(); ++i) {
        cargv.push_back(const_cast<char *>(argv[i].c_str()));
      }
      cargv.push_back(nullptr);

      ::execvp(cargv[0], cargv.data());
      _exit(127);
    }

    // Parent
    if (out_stdin_fd != nullptr && p_stdin.read_fd >= 0)
      ::close(p_stdin.read_fd); // close read end
    if (out_stdout_fd != nullptr)
      ::close(p_stdout.write_fd); // close write end
    if (out_stderr_fd != nullptr && out_stderr_fd != out_stdout_fd)
      ::close(p_stderr.write_fd); // close write end

    // Set nonblocking read end.
    const int flags = ::fcntl(p_stdout.read_fd, F_GETFL, 0);
    if (flags >= 0)
      (void) ::fcntl(p_stdout.read_fd, F_SETFL, flags | O_NONBLOCK);

    if (out_stderr_fd != nullptr && out_stderr_fd != out_stdout_fd) {
      const int err_flags = ::fcntl(p_stderr.read_fd, F_GETFL, 0);
      if (err_flags >= 0)
        (void) ::fcntl(p_stderr.read_fd, F_SETFL, err_flags | O_NONBLOCK);
    }

    if (out_stdout_fd != nullptr)
      *out_stdout_fd = p_stdout.read_fd;
    if (out_stderr_fd != nullptr)
      *out_stderr_fd = p_stderr.read_fd;
    if (out_stdin_fd != nullptr)
      *out_stdin_fd = p_stdin.write_fd;

    return pid;
  }

fail_exit:
  if (out_stdout_fd != nullptr)
    for (const int fd : p_stdout.fds)
      if (fd >= 0)
        ::close(fd);
  if (out_stderr_fd != nullptr && out_stderr_fd != out_stdout_fd)
    for (const int fd : p_stderr.fds)
      if (fd >= 0)
        ::close(fd);
  if (out_stdin_fd != nullptr)
    for (const int fd : p_stdin.fds)
      if (fd >= 0)
        ::close(fd);
  errno = saved_errno;
  return -1;
}

int subprocess::plumbing::read_pipe_nonblocking(const int pipe_fd, std::string &out) {
  if (pipe_fd < 0) {
    errno = EINVAL;
    return -1;
  }

  char buf[4096];
  bool read_any = false;

  for (;;) {
    ssize_t n = ::read(pipe_fd, buf, sizeof(buf));
    if (n > 0) {
      out.append(buf, static_cast<size_t>(n));
      read_any = true;
      continue; // keep draining until EAGAIN or EOF
    }
    if (n == 0)
      return 2; // EOF
    if (errno == EINTR)
      return read_any ? 1 : 0;
    if (errno == EAGAIN || errno == EWOULDBLOCK)
      return read_any ? 1 : 0;
    return -1;
  }
}

int subprocess::plumbing::drain_pipe_blocking(const int pipe_fd, std::string &out) {
  if (pipe_fd < 0) {
    errno = EINVAL;
    return -1;
  }

  const int flags = ::fcntl(pipe_fd, F_GETFL, 0);
  if (flags >= 0)
    (void) ::fcntl(pipe_fd, F_SETFL, flags & ~O_NONBLOCK);

  char buf[4096];
  for (;;) {
    ssize_t n = ::read(pipe_fd, buf, sizeof(buf));
    if (n > 0) {
      out.append(buf, static_cast<size_t>(n));
      continue;
    }
    if (n == 0)
      return 0; // EOF
    if (errno == EINTR)
      continue;
    return -1;
  }
}

int subprocess::plumbing::poll(const pid_t pid, int *out_exit_code) {
  if (pid <= 0 || out_exit_code == nullptr) {
    errno = EINVAL;
    return -1;
  }

  int status = 0;
  pid_t r;
  do {
    r = ::waitpid(pid, &status, WNOHANG);
  } while (r < 0 && errno == EINTR);

  if (r == 0)
    return 0; // still running
  if (r < 0)
    return -1; // error

  if (WIFEXITED(status)) {
    *out_exit_code = WEXITSTATUS(status);
    return 1;
  }
  if (WIFSIGNALED(status)) {
    *out_exit_code = 128 + WTERMSIG(status);
    return 2;
  }

  *out_exit_code = 255;
  return 2;
}

int subprocess::plumbing::send_signal(const pid_t pid, const int signal) {
  if (pid <= 0) {
    return EINVAL;
  }
  return (::kill(pid, signal) == 0) ? 0 : errno;
}

subprocess::process::~process() {
  if (pid > 0 && !exit_code.has_value()) {
    const auto res = this->kill();
    if (res.has_value() && *res == 0) {
      // Reap the process
      (void) ::waitpid(this->pid, nullptr, 0);
    }
  }
}

std::optional<int> subprocess::process::run(const std::vector<std::string> &argv,
                                            const bool capture_stdout,
                                            const bool capture_stderr,
                                            const bool provide_stdin,
                                            const bool stderr_to_stdout) {
  assert(!(capture_stderr && stderr_to_stdout) && "Cannot both capture stderr and redirect stderr to stdout");
  assert(pid <= 0 && "Process is already running");

  pid = -1;
  captured_stderr.clear();
  captured_stdout.clear();
  exit_code = std::nullopt;

  const int child_pid = spawn(argv,
                              capture_stdout ? &this->stdout_fd : nullptr,
                              capture_stderr ? &this->stderr_fd : (stderr_to_stdout ? &this->stdout_fd : nullptr),
                              provide_stdin ? &this->stdin_fd : nullptr);
  if (child_pid < 0)
    return errno;

  pid = child_pid;
  return std::nullopt;
}

bool subprocess::process::is_alive() {
  if (pid <= 0)
    return false;
  if (exit_code.has_value())
    return false;

  int code = 0;
  const int res = poll(pid, &code);
  if (res < 0) {
    perror("subprocess::Popen::is_alive: poll failed");
    abort(); // fatal error polling process
  }
  if (res == 0) {
    // Still running, drain pipes
    if (stdout_fd >= 0)
      (void) read_pipe_nonblocking(stdout_fd, captured_stdout);
    if (stderr_fd >= 0)
      (void) read_pipe_nonblocking(stderr_fd, captured_stderr);
    return true;
  }

  // Process has exited
  exit_code = code;
  pid = -1;
  // Drain pipes
  if (stdout_fd >= 0) {
    (void) drain_pipe_blocking(stdout_fd, captured_stdout);
    ::close(stdout_fd);
  }
  if (stderr_fd >= 0) {
    (void) drain_pipe_blocking(stderr_fd, captured_stderr);
    ::close(stderr_fd);
  }
  if (stdin_fd >= 0) {
    ::close(stdin_fd);
  }

  stdin_fd = -1;
  stdout_fd = -1;
  stderr_fd = -1;

  return false;
}
