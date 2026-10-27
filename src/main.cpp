#include "chip8.h"
#include <iostream>

int main(int argc, char*argv[]) {
  // Check for rom argument
  if (argc < 2) {
    std::cout << "Usage: chip8 <rom>\n";
    return 1;
  }
  
  Chip8 chip8 = Chip8();
  chip8.load_rom(argv[1]);
  chip8.dump_memory(0x200, 64);

  return 0;
}
