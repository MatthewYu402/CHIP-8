#include "chip8.h"
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <iomanip>
#include <algorithm>

Chip8::Chip8() {
  // Load in the font into memory starting from 0x050
  std::copy(FONT.begin(), FONT.end(), memory + FONT_ADDRESS);
}

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

void Chip8::cycle() {
  // 1. Fetch
  std::uint16_t opcode = (memory[pc] << 8) | memory[pc + 1];
  pc += 2;

  // 2. Decode
  std::uint8_t family_nib = (opcode & 0xF000) >> 12;
  std::uint8_t X = (opcode & 0x0F00) >> 8;
  std::uint8_t Y = (opcode & 0x00F0) >> 4;
  std::uint8_t N = opcode & 0x000F;
  std::uint8_t NN = opcode & 0x00FF;
  std::uint16_t NNN = opcode & 0x0FFF;

  // 3. Execute
  switch (family_nib) {
    case 0:
      switch (NN) {
        case 0xE0:
        // clear screen
          break;

        case 0xEE:
          pc = stack[sp];
          sp -= 1;
          break;

        default:
          break;
          
      }

      break;
    

  
    case 1:
      pc = NNN;
      break;

    case 2:
      stack[sp] = pc;
      sp += 1;
      pc = NNN;
      break;

    case 3:
      if (V[X] == NN) pc += 2;
      break;

    case 4:
      if (V[X] != NN) pc += 2;
      break;

    case 5:
      if (V[X] == V[Y]) pc += 2;
      break;

    case 6:
      V[X] = NN;
      break;

    case 7:
      V[X] += NN;
      break;

    case 8:
      switch(N) {
        case 0:
          V[X] = V[Y];
          break;

        case 1: 
          V[X] |= V[Y];
          break;

        case 2:
          V[X] &= V[Y];
          break;

        case 3:
          V[X] ^= V[Y];
          break;

        case 4:
          break;

        case 5:
          break;

        case 6:
          V[X] >>= 1;
          break;

        case 7:
          break;

        case 0xE: 
          V[X] <<= 1;
          break;

        default:

      }

    case 9:
      if (V[X] != V[Y]) pc += 2;
      break;
    case 0xA:
      I = NNN;
      break;

    case 0xB:
      pc = NNN + V[0]; // QUIRK:
      break;

    case 0xC:
      // TODO: Use the rand function
      //V[X] = (use rand function) & NN;
      break;

    case 0xD:
      // TODO: Draw with a library
      break;

    case 0xE:
      switch(NN) {
        case 0x9E:
          break;

        case 0xA1:
          break;
      }

      default:
        break;
      
      break;

    case 0xF:
      switch (NN) {
        case 0x07:
          V[X] = delay_timer;
          break;
        
        case 0x0A:
          // TODO: Key Press
          break;

        case 0x15:
          delay_timer = V[X];
          break;

        case 0x18:
          sound_timer = V[X];
          break;

        case 0x1E:
          I += V[X];
          break;

        case 0x29:
          // TODO: Address of font sprite for hex digit in V[X]
          break;

        case 0x33:
          // Store binary coded decimal
          break;

        case 0x55:
          // TODO: Store V[0]
          break;

        case 0x65:
          // TODO: Store V[0]
          break;

        default:
          break;
      }
  }
}

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



