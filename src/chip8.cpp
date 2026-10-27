#include "Chip8.h"
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <iomanip>

Chip8::Chip8() {}

bool Chip8::load_rom(const std::string& path) {
  // 1. Check if file is opened and is the right path
  std::ifstream istrm(path, std::ios::binary);
  if (!istrm.is_open()) {
    std::cerr << "failed to open " << path << '\n';
    return false;
  }

  // 2. Check if size exceeds 3584 bytes, not a valid program
  std::uintmax_t max_bytes = MEMORY_SIZE - START_ADDRESS;
  std::uintmax_t f_size = std::filesystem::file_size(path);
  if (max_bytes < f_size) {
    std::cerr<< "file exceeds memory capacity: " << f_size << '\n';
    return false;
  }  
    
  // 3. Read in one bulk operation instead of byte by byte
  istrm.read(reinterpret_cast<char*>(memory + START_ADDRESS), static_cast<std::streamsize>(f_size));

  return true;
}

void Chip8::cycle() {}

void Chip8::dump_memory(std::uint16_t start, std::size_t count) const {
  // 1. Wraparound
  if (start >= MEMORY_SIZE) {
    std::cerr << "start goes past 4KB memory bound: " << start << '\n';
    return;
  }

  // 2. Bounds checking
  if (count > MEMORY_SIZE - start) {
    count = MEMORY_SIZE - start;
  }

  std::cout << std::hex << std::setfill('0'); 
  for (std::size_t i = 0; i < count; ++i) {
    std::size_t address = start + i;
    if (i % 16 == 0) { // new row
      if (i != 0) {
        std::cout << '\n';
      }
      std::cout << std::setw(4) << address << ':';
    }

    std::cout << " " << std::setw(2) << static_cast<int>(memory[address]);
  }

  // Revert back
  std::cout << '\n' << std::dec;

}



