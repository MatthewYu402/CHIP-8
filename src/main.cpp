#include "chip8.h"
#include <SDL.h>
#include <iostream>
#include <array>
#include <cstdint>

void print_display(const Chip8 &chip8) {
  const Chip8::Display &arr = chip8.get_display_array();
  for (std::size_t y = 0; y < Chip8::DISPLAY_HEIGHT; ++y) {
    for (std::size_t x = 0; x < Chip8::DISPLAY_WIDTH; ++x) {
      std::cout << (arr[y * Chip8::DISPLAY_WIDTH + x] ? '#' : ' ');
    }
    std::cout << '\n';
  }
}

void print_error(const char *reason, const char *error_message) {
  std::cerr << reason << ": " << "SDL_Error: " << error_message << '\n';
}

void cleanup(SDL_Window *window, SDL_Renderer *renderer, SDL_Texture *texture) {
  if (texture) SDL_DestroyTexture(texture);
  if (renderer) SDL_DestroyRenderer(renderer);
  if (window) SDL_DestroyWindow(window);
  SDL_Quit();
}

int main(int argc, char *argv[]) {
  // Screen constants
  constexpr int SCREEN_WIDTH = 640;
  constexpr int SCREEN_HEIGHT = 320;

  // Colors for screen and texture
  constexpr std::uint32_t COLOR_ON = 0xFFFFFFFF;
  constexpr std::uint32_t COLOR_OFF = 0xFF000000;

  // Check for rom argument
  if (argc < 2) {
    std::cerr << "Usage: chip8 <rom>\n";
    return 1;
  }

  Chip8 chip8;
  if (!chip8.load_rom(argv[1])) return 1;

  // chip8.dump_memory(0x000, 0xA0);
  // chip8.dump_memory(0x050, 80);
  // chip8.dump_memory(0x200, 132);

  for (std::size_t i = 0; i < 5000; ++i) {
    chip8.cycle();
  }

  print_display(chip8);

  // The window we'll be rendering to
  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;
  SDL_Texture *texture = nullptr;

  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    print_error("SDL could not initialize", SDL_GetError());
    return 1;
  }

  // Create window
  window = SDL_CreateWindow("CHIP-8 Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
  if (window == nullptr) {
    print_error("Window could not be created", SDL_GetError());
    cleanup(window, renderer, texture);
    return 1;
  }

  // Create a renderer for window
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (renderer == nullptr) {
    print_error("Renderer could not be created", SDL_GetError());
    cleanup(window, renderer, texture);
    return 1;
  }

  // Create a texture from renderer
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, Chip8::DISPLAY_WIDTH, Chip8::DISPLAY_HEIGHT);
  if (!texture) {
    print_error("Texture could not be created", SDL_GetError());
    cleanup(window, renderer, texture);
    return 1;
  }

  // Pixels array
  std::array<std::uint32_t, Chip8::DISPLAY_WIDTH * Chip8::DISPLAY_HEIGHT> pixels;

  // Loop until quit
  SDL_Event e;
  bool quit = false;
  while (!quit) {
    // Poll Events
    while (SDL_PollEvent(&e))
    {
      if (e.type == SDL_QUIT)
      {
        quit = true;
      }
    }

    // Convert display_array (bool) into pixels (RGB Color)
    const auto &display_array = chip8.get_display_array();
    for (std::size_t y = 0; y < Chip8::DISPLAY_HEIGHT; ++y) {
      for (std::size_t x = 0; x < Chip8::DISPLAY_WIDTH; ++x) {
        std::size_t index = y * Chip8::DISPLAY_WIDTH + x;
        pixels[index] = (display_array[index]) ? COLOR_ON : COLOR_OFF;
      }
    }

    if (SDL_UpdateTexture(texture, nullptr, pixels.data(), Chip8::DISPLAY_WIDTH * sizeof(std::uint32_t)) < 0) {
      print_error("Texture was unable to update", SDL_GetError());
      cleanup(window, renderer, texture);
      return 1;
    }
    if (SDL_RenderClear(renderer) < 0) {
      print_error("Renderer was unable to clear", SDL_GetError());
      cleanup(window, renderer, texture);
      return 1;
    }

    if (SDL_RenderCopy(renderer, texture, nullptr, nullptr) < 0) {
      print_error("Renderer was unable to copy", SDL_GetError());
      cleanup(window, renderer, texture);
      return 1;
    }
    SDL_RenderPresent(renderer);
    SDL_Delay(16);
  }
  // Destroy resources no longer needed
  cleanup(window, renderer, texture);

  return 0;
}
