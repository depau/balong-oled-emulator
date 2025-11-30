#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <memory>

#include "display.h"
#include "hooks.h"
#include "sdl_utils.h"

int main(int argc, char *argv[]) {
  bool is_short = false;

  if (argc > 1 && std::string(argv[1]) == "--help") {
    std::cout << "Usage: " << argv[0] << " --short\n";
    return 0;
  } else if (argc > 1 && std::string(argv[1]) == "--short") {
    is_short = true;
  }

  setup_hooks();

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cerr << "Could not initialize SDL: " << SDL_GetError() << '\n';
    return 1;
  }

  if (TTF_Init() == -1) {
    std::cerr << "Could not initialize SDL_ttf: " << TTF_GetError() << '\n';
    SDL_Quit();
    return 1;
  }

  set_display(std::make_unique<Display>());
  get_display().set_short_screen_mode(is_short);
  get_display().run_forever();

  TTF_Quit();
  SDL_Quit();

  return 0;
}
