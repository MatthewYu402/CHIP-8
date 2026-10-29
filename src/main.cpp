#include "chip8.h"
#include <iostream>

void print_display(const Chip8& chip8) {
  const Chip8::Display& arr = chip8.get_display_array();
  for (std::size_t y = 0; y < Chip8::DISPLAY_HEIGHT; ++y) {
    for (std::size_t x = 0; x < Chip8::DISPLAY_WIDTH; ++x) {
      std::cout << (arr[y * Chip8::DISPLAY_WIDTH + x] ? '#' : ' ');
    }
    std::cout << '\n';
  }
}

int main(int argc, char* argv[]) {
  // Check for rom argument
  if (argc < 2) {
    std::cerr << "Usage: chip8 <rom>\n";
    return 1;
  }
  
  Chip8 chip8;
  if (!chip8.load_rom(argv[1])) return 1;

  //chip8.dump_memory(0x000, 0xA0);
  //chip8.dump_memory(0x050, 80);
  //chip8.dump_memory(0x200, 132);
  
  for (size_t i = 0; i < 300; ++i) {
    chip8.cycle();
  }

  print_display(chip8);


  return 0;
}
