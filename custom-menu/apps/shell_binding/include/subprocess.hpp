#pragma once

#include <optional>
#include <signal.h>
#include <string>
#include <time.h>
#include <vector>

namespace subprocess {
namespace plumbing {

/**
 * Spawns a background process with pipes for stdin, stdout, and stderr.
 * If out_stdin_fd is not provided, the child process's stdin is /dev/null.
 * If out_stdout_fd or out_stderr_fd are not provided, those pipes are connected to the parent process's stdout/stderr.
 * If out_stdout_fd and out_stderr_fd are the same pointer, they share the same pipe.
 *
 * @param argv Argument vector for the process (argv[0] is the program name)
 * @param out_stdout_fd The file descriptor for reading stdout from the child process
 * @param out_stderr_fd The file descriptor for reading stderr from the child process
 * @param out_stdin_fd The file descriptor for writing stdin to the child process
 * @return The PID of the spawned process, or -1 on error (errno set)
 */
pid_t spawn(const std::vector<std::string> &argv,
            int *out_stdout_fd = nullptr,
            int *out_stderr_fd = nullptr,
            int *out_stdin_fd = nullptr);

/**
 * Non-blocking read from pipe_fd, appends any available bytes to out.
 *
 * @param pipe_fd The file descriptor of the pipe to read from
 * @param out The string to append the read data to
 * @return 1 if some bytes were read and appended, 0 if no bytes available right now (EAGAIN/EWOULDBLOCK) or
 * interrupted, -1 on error (errno set), 2 on EOF (pipe closed) (errno unchanged)
 */
int read_pipe_nonblocking(int pipe_fd, std::string &out);

/**
 * Blocking read from pipe_fd until EOF, appends all read bytes to out.
 *
 * @param pipe_fd The file descriptor of the pipe to read from
 * @param out The string to append the read data to
 * @return 0 on success, -1 on error (errno set)
 */
int drain_pipe_blocking(int pipe_fd, std::string &out);

/**
 * Check if the process with the given PID has exited without blocking.
 * If the process has exited, it reaps the process and sets the exit code in out_exit_code.
 *
 * @param pid The PID of the process to check
 * @param out_exit_code Pointer to an integer to store the exit code
 * @return 0 if still running, 1 if exited normally, 2 if terminated by signal, -1 on error (errno set)
 */
int poll(pid_t pid, int *out_exit_code);

/**
 * Send a signal to the process with the given PID.
 *
 * @param pid The PID of the process to send the signal to
 * @param signal The signal number to send
 * @return 0 on success, the error code on failure
 */
int send_signal(pid_t pid, int signal);

} // namespace plumbing

/**
 * Class representing a subprocess with optional captured stdout/stderr and stdin pipe.
 */
class process {
  pid_t pid = -1;

  int stdout_fd = -1;
  int stderr_fd = -1;
  int stdin_fd = -1;

  std::string captured_stdout = "";
  std::string captured_stderr = "";

  std::optional<int> exit_code = std::nullopt;

  const std::string empty_string = "";

public:
  /**
   * Create a Popen object to run a subprocess.
   */
  process() = default;

  ~process();

  process &operator=(const process &) = delete;
  process(const process &) = delete;

  process &operator=(process &&other) noexcept {
    if (this != &other) {
      pid = other.pid;
      stdout_fd = other.stdout_fd;
      stderr_fd = other.stderr_fd;
      stdin_fd = other.stdin_fd;
      captured_stdout = std::move(other.captured_stdout);
      captured_stderr = std::move(other.captured_stderr);
      exit_code = other.exit_code;

      other.pid = -1;
      other.stdout_fd = -1;
      other.stderr_fd = -1;
      other.stdin_fd = -1;
      other.exit_code = std::nullopt;
    }
    return *this;
  }
  process(process &&other) noexcept { *this = std::move(other); }

  /**
   * Start the process and run it in the background.
   *
   * @param argv Argument vector for the process (argv[0] is the program name)
   * @param capture_stdout Whether to capture stdout (if false, stdout is inherited from the parent)
   * @param capture_stderr Whether to capture stderr (if false, stderr is inherited from the parent)
   * @param provide_stdin Whether to provide a stdin pipe (if false, stdin is /dev/null)
   * @param stderr_to_stdout Whether to redirect stderr to stdout
   * @return nullopt on success, or the error code on failure
   */
  std::optional<int> run(const std::vector<std::string> &argv,
                         bool capture_stdout = true,
                         bool capture_stderr = false,
                         bool provide_stdin = false,
                         bool stderr_to_stdout = false);

  /**
   * Check if the process is still alive.
   * While at it, drain any available output from stdout/stderr pipes to prevent pipe blocking.
   *
   * You should call this or get_exit_code() periodically to keep pipes drained.
   *
   * @return true if the process is still running, false if it has exited
   */
  bool is_alive();

  /**
   * Get the exit code of the process if it has exited.
   * While at it, drain any remaining output from stdout/stderr pipes to prevent pipe blocking.
   *
   * You should call this or is_alive() periodically to keep pipes drained.
   *
   * @return The exit code if the process has exited, or nullopt if it is still running
   */
  std::optional<int> get_exit_code() {
    if (!is_alive())
      return exit_code;
    return std::nullopt;
  }

  /**
   * Get the captured stdout output of the process.
   * If the process is still running, returns an empty string.
   *
   * @return The captured stdout output
   */
  const std::string &get_stdout() const {
    if (!exit_code.has_value())
      return empty_string;
    return captured_stdout;
  }

  /**
   * Get the captured stderr output of the process.
   * If the process is still running, returns an empty string.
   *
   * @return The captured stderr output
   */
  const std::string &get_stderr() const {
    if (!exit_code.has_value())
      return empty_string;
    return captured_stderr;
  }

  /**
   * Get the stdin file descriptor for writing to the process's stdin.
   *
   * @return The stdin file descriptor, or nullopt if stdin was not provided
   */
  std::optional<int> get_stdin_fd() const {
    if (stdin_fd >= 0)
      return stdin_fd;
    return std::nullopt;
  }

  /**
   * Send a signal to the process.
   *
   * @param signal The signal number to send
   * @return 0 on success, error code on failure, or nullopt if the process is not alive
   */
  std::optional<int> send_signal(const int signal) {
    if (!is_alive())
      return std::nullopt;
    return plumbing::send_signal(pid, signal);
  }

  /**
   * Terminate the process with SIGTERM.
   *
   * @return 0 on success, error code on failure, or nullopt if the process is not alive
   */
  std::optional<int> terminate() {
    const auto res = send_signal(SIGTERM);
    (void) is_alive(); // Attempt to reap and drain pipes
    return res;
  }

  /**
   * Kill the process with SIGKILL.
   *
   * @return 0 on success, error code on failure, or nullopt if the process is not alive
   */
  std::optional<int> kill() {
    const auto res = send_signal(SIGKILL);
    if (res.has_value() && *res == 0) {
      // Reap the process to avoid zombies
      while (is_alive()) {
        nanosleep((const timespec[]) { { 0, 10 * 1000 * 1000 } }, nullptr);
      }
    }
    return res;
  }
};
} // namespace subprocess
