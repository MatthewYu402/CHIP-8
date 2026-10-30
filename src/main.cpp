#include "chip8.h"
#include <SDL.h>
#include <iostream>
#include <array>
#include <cstdint>
#include <cmath>

// For debugging
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

void cleanup(SDL_Window *window, SDL_Renderer *renderer, SDL_Texture *texture, SDL_AudioDeviceID device)
{
  if (texture) SDL_DestroyTexture(texture);
  if (renderer) SDL_DestroyRenderer(renderer);
  if (window) SDL_DestroyWindow(window);
  if (device) SDL_CloseAudioDevice(device);
  SDL_Quit();
}

// Audio
struct BeepState {
  int tone_hz {440};
  std::int16_t volume {3000};
  int samples_per_cycle {};
  int phase {};
};


void audio_callback(void* userdata, Uint8* stream, int len) {
  Sint16* samples = reinterpret_cast<Sint16*>(stream);
  int count = len / static_cast<int>(sizeof(Sint16)); // 1024 bytes -> 512 samples
  BeepState* beep = static_cast<BeepState*>(userdata);

  // Place a sample into the buffer
  for (int i = 0; i < count; ++i) {
    Sint16 sample = static_cast<Sint16>((beep->phase < beep->samples_per_cycle / 2) ? +beep->volume : -beep->volume);
    samples[i] = sample;
    beep->phase += 1;
    if (beep->phase == beep->samples_per_cycle) beep->phase = 0; 
  }
}

// Key Mappings
int map_key(SDL_Scancode scancode) {
  // Map from QWERTY keyboard to CHIP-8
  switch(scancode) {
    case SDL_SCANCODE_1:
      return 0x1;

    case SDL_SCANCODE_2:
      return 0x2;

    case SDL_SCANCODE_3:
      return 0x3;

    case SDL_SCANCODE_4:
      return 0xC;

    case SDL_SCANCODE_Q:
      return 0x4;

    case SDL_SCANCODE_W:
      return 0x5;

    case SDL_SCANCODE_E:
      return 0x6;

    case SDL_SCANCODE_R:
      return 0xD;

    case SDL_SCANCODE_A:
      return 0x7;

    case SDL_SCANCODE_S:
      return 0x8;

    case SDL_SCANCODE_D:
      return 0x9;

    case SDL_SCANCODE_F:
      return 0xE;

    case SDL_SCANCODE_Z:
      return 0xA;

    case SDL_SCANCODE_X:
      return 0x0;

    case SDL_SCANCODE_C:
      return 0xB;

    case SDL_SCANCODE_V:
      return 0xF;

    default:
      return -1;
  }
}

int main(int argc, char *argv[]) {
  // Screen constants
  constexpr int SCALE = 15;
  constexpr int SCREEN_WIDTH = Chip8::DISPLAY_WIDTH * SCALE;
  constexpr int SCREEN_HEIGHT = Chip8::DISPLAY_HEIGHT * SCALE;
  constexpr double TARGET_FPS = 60.0;
  constexpr int CYCLES_PER_FRAME = 10;
  constexpr double FRAME_MS = 1000.0 / TARGET_FPS;

  // Audio constants
  constexpr int SAMPLE_RATE = 44100;

  // Colors for screen and texture
  constexpr std::uint32_t COLOR_ON = 0xFFFFFFFF;
  constexpr std::uint32_t COLOR_OFF = 0xFF000000;

  // Check for rom argument
  if (argc < 2) {
    std::cerr << "Usage: chip8 <rom>\n";
    return 1;
  }

  // Construct Chip8 object
  Chip8 chip8;
  if (!chip8.load_rom(argv[1])) return 1;

  // chip8.dump_memory(0x000, 0xA0);
  // chip8.dump_memory(0x050, 80);
  // chip8.dump_memory(0x200, 132);

  //print_display(chip8);

  // Beep state lives outside device to outlive everything that uses it
  BeepState beep;

  // The window we'll be rendering to
  SDL_Window* window = nullptr;
  SDL_Renderer* renderer = nullptr;
  SDL_Texture* texture = nullptr;
  
  //
  SDL_AudioSpec want{};
  want.freq     = SAMPLE_RATE;
  want.format   = AUDIO_S16SYS;
  want.channels = 1;
  want.callback = audio_callback;
  want.userdata = &beep;

  SDL_AudioSpec have{}; // SDL fills this in

  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
    print_error("SDL could not initialize", SDL_GetError());
    return 1;
  }

  // Initialize audio device
  SDL_AudioDeviceID device = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
  if (!device)
  {
    print_error("SDL count not create audio device", SDL_GetError());
    return 1;
  }

  beep.samples_per_cycle = have.freq / beep.tone_hz;

  // Create window
  window = SDL_CreateWindow("CHIP-8 Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
  if (window == nullptr) {
    print_error("Window could not be created", SDL_GetError());
    cleanup(window, renderer, texture, device);
    return 1;
  }

  // Create a renderer for window
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (renderer == nullptr) {
    print_error("Renderer could not be created", SDL_GetError());
    cleanup(window, renderer, texture, device);
    return 1;
  }

  // Create a texture from renderer
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, Chip8::DISPLAY_WIDTH, Chip8::DISPLAY_HEIGHT);
  if (!texture) {
    print_error("Texture could not be created", SDL_GetError());
    cleanup(window, renderer, texture, device);
    return 1;
  }



  // Pixels array
  std::array<std::uint32_t, Chip8::DISPLAY_WIDTH * Chip8::DISPLAY_HEIGHT> pixels;

  // Loop until quit
  SDL_Event event;
  bool quit = false;
  while (!quit) {
    std::uint64_t start = SDL_GetPerformanceCounter();
    // Poll Events
    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_QUIT)
      {
        quit = true;
      }

      if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
        SDL_Scancode scancode = event.key.keysym.scancode;

        // Emulator Controls (not a part of CHIP-8 keypad): key-down only
        if (event.type == SDL_KEYDOWN) {
          if (scancode == SDL_SCANCODE_ESCAPE) {
            quit = true;
            continue;
          }

          if (scancode == SDL_SCANCODE_0 && event.key.repeat == 0) {
            chip8 = Chip8{};
            if (!chip8.load_rom(argv[1])) {
              std::cerr << "failed to reload ROM: " << argv[1] << '\n';
              quit = true;
            }
            continue;
          }
        }

        // CHIP-8 keypad
        int index = map_key(scancode);
        if (index != -1) chip8.set_key(index, event.type == SDL_KEYDOWN);
      }
    }

    // CPU's share of work for this frame
    for (int i = 0; i < CYCLES_PER_FRAME; ++i) { 
      chip8.cycle();
    }

    chip8.tick_timers(); // timers' one countdown for this slice

    SDL_PauseAudioDevice(device, chip8.is_sound_on() ? 0 : 1);

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
      cleanup(window, renderer, texture, device);
      return 1;
    }
    if (SDL_RenderClear(renderer) < 0) {
      print_error("Renderer was unable to clear", SDL_GetError());
      cleanup(window, renderer, texture, device);
      return 1;
    }

    if (SDL_RenderCopy(renderer, texture, nullptr, nullptr) < 0) {
      print_error("Renderer was unable to copy", SDL_GetError());
      cleanup(window, renderer, texture, device);
      return 1;
    }
    SDL_RenderPresent(renderer); // The screen's one refresh

    // Wait: cap the frame rate (target 60fps = ~16.67 ms per frame)
    std::uint64_t end = SDL_GetPerformanceCounter();
    double elapsed_ms = (end - start) / static_cast<double>(SDL_GetPerformanceFrequency()) * 1000.0;

    if (elapsed_ms < FRAME_MS) {
      SDL_Delay(static_cast<uint32_t>(std::floor(FRAME_MS - elapsed_ms)));
    }
  }
  // Destroy resources no longer needed
  cleanup(window, renderer, texture, device);

  return 0;
}
