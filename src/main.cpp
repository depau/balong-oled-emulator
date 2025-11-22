#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <memory>

#include "display.h"
#include "hooks.h"
#include "sdl_utils.h"

int main(int, char **) {
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
  get_display().run_forever();

  TTF_Quit();
  SDL_Quit();

  return 0;
}
