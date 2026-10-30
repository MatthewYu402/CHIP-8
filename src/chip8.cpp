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
  std::error_code ec;
  std::uintmax_t f_size = std::filesystem::file_size(path, ec);
  
  // 3. Error Checks
  if (ec) {
    std::cerr << "invalid path: " << path << "error: " << ec.message() << '\n';
    return false;
  }

  if (f_size == 0) {
    std::cerr << "empty file: " << f_size << '\n';
    return false;
  }

  if (max_bytes < f_size) {
    std::cerr<< "file exceeds memory capacity: " << f_size << '\n';
    return false;
  }  
    
  // 4. Read in one bulk operation instead of byte by byte
  istrm.read(reinterpret_cast<char*>(memory + START_ADDRESS), static_cast<std::streamsize>(f_size));
 
  if (istrm.gcount() != static_cast<std::streamsize>(f_size)) {
    std::cerr << "input stream could not read the whole file" << '\n';
    return false;
  }

  return true;
}

void Chip8::cycle() {
  if (halted) return; 

  // 0.1 Pre-fetch check
  if (pc + 1 >= MEMORY_SIZE) {
    halted = true;
    std::cerr << std::hex <<"program counter goes beyond memory 0xFFF: " << std::setw(4) << 
      std::setfill('0') << pc << '\n';
    std::cerr << std::dec;
    return;
  }

  // 1. Fetch
  std::uint16_t opcode = (memory[pc] << 8) | memory[pc + 1];
  pc += 2;

  if (opcode == 0x0000) {
    halt("executed 0000 (pc probably in empty memory", opcode);
    return;
  }

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
      switch (opcode) {
       case 0x00E0:
          display_array.fill(false);
          break;

        case 0x00EE:
          if (sp == 0) {
            halt("return with empty stack", opcode);
            break;
          }
          sp -= 1;
          pc = stack[sp];
          break;

        default:
          halt("unknown opcode", opcode);
          break;
          
      }

      break;
    
    case 1:
      pc = NNN;
      break;

    case 2:
      if (sp == 16) {
        halt("stack overflow", opcode);
        break;
      }
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
      if (N != 0) {
        halt("invalid opcode (last nibble must be 0)", opcode);
        break;
      }
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

        // QUIRK: Leaves VF unchanged; original resets VF to 0
        case 1: 
          V[X] |= V[Y];
          break;

        // QUIRK: Leaves VF unchanged; original resets VF to 0
        case 2:
          V[X] &= V[Y];
          break;

        // QUIRK: Leaves VF unchanged; original resets VF to 0
        case 3:
          V[X] ^= V[Y];
          break;

        case 4: {
          unsigned int sum = V[X] + V[Y];
          V[X] = sum;
          V[0xF] = sum > 255;
          break;
        }

        case 5: {
          std::uint8_t flag = V[X] >= V[Y];
          V[X] -= V[Y];
          V[0xF] = flag;
          break;
        }

        // QUIRK: Shifts V[X] in place; original COSMAC VIP shifted V[Y] into V[X]
        case 6: {
          std::uint8_t shifted_bit = (V[X] & 0x1);
          V[X] >>= 1;
          V[0xF] = shifted_bit;
          break;
        }

        case 7: {
          std::uint8_t flag = V[Y] >= V[X];
          V[X] = V[Y] - V[X];
          V[0xF] = flag;
          break;

        }

        // QUIRK: Shifts V[X] in place; original COSMAC VIP shifted V[Y] into V[X]
        case 0xE: {
          std::uint8_t shifted_bit = (V[X] & 0x80) >> 7;
          V[X] <<= 1;
          V[0xF] = shifted_bit;
          break;
        }

        default:
          halt("unknown 8XY_ instruction", opcode);
          break;

      }
      break;

    case 9:
      if (N != 0) {
        halt("invalid opcode (last nibble must be 0)", opcode);
        break;
      }
      if (V[X] != V[Y]) pc += 2;
      break;

    case 0xA:
      I = NNN;
      break;

    // QUIRK: COSMAC VIP jumps to NNN + V0; Chip-48 jumps to XNN + VX
    case 0xB:
      pc = NNN + V[0];
      break;

    case 0xC: {
      std::uint8_t random_byte = static_cast<std::uint8_t>(dist(gen));
      V[X] = random_byte & NN;
      break;
    }

    case 0xD: {
      std::uint8_t start_x = V[X] % DISPLAY_WIDTH;
      std::uint8_t start_y = V[Y] % DISPLAY_HEIGHT;
      V[0xF] = 0;

      for (std::size_t row = 0; row < N; ++row) {
        std::uint8_t sprite_byte = memory[(I + row) & 0xFFF];
        for (std::size_t col = 0; col < 8; ++col) {
          if (start_x + col >= DISPLAY_WIDTH || start_y + row >= DISPLAY_HEIGHT) {
            continue;
          }
          if (sprite_byte & (0x80 >> col)) {
            std::size_t x = start_x + col;
            std::size_t y = start_y + row;
            std::size_t index = y * DISPLAY_WIDTH + x;

            if (display_array[index]) {
              V[0xF] = 1;
            }
            display_array[index] = !display_array[index];
          }
        }
      }
      break;
    }

    case 0xE:
      switch(NN) {
        case 0x9E:
          if (key_state[V[X] & 0xF]) pc += 2;
          break;

        case 0xA1:
          if (!key_state[V[X] & 0xF]) pc += 2;
          break;

        default:
          halt("unknown EX__ instruction", opcode);
          break;
      }

      break;

    case 0xF:
      switch (NN) {
        case 0x07:
          V[X] = delay_timer;
          break;
        
        case 0x0A: {
          if (waiting_key == KEY_COUNT) {
            // Phase 1: look for a press
            for (std::size_t i = 0; i < key_state.size(); ++i) {
              if (key_state[i]) {
                waiting_key = i;
                break;
              }
            }
            pc -= 2;
          } else if (key_state[waiting_key]){
            // Phase 2: key still held, keep waiting
            pc -= 2;
          } else {
            // Key released: done 
            V[X] = waiting_key;
            waiting_key = KEY_COUNT;
          }
          break;
        }

        case 0x15:
          delay_timer = V[X];
          break;

        case 0x18:
          sound_timer = V[X];
          break;

        case 0x1E:
          I += V[X];
          break;

        case 0x29: {
          std::uint8_t digit = V[X] & 0xF;
          I = FONT_ADDRESS + 5 * digit;
          break;
        }

        case 0x33: {
          std::uint8_t hundred = V[X] / 100;
          std::uint8_t tens = (V[X] / 10) % 10;
          std::uint8_t ones = V[X] % 10;
          memory[(I & 0xFFF)] = hundred;
          memory[(I + 1) & 0xFFF] = tens;
          memory[(I + 2) & 0xFFF] = ones;
          break;
        }

        // QUIRK: I is left unchanged; the original COSMAC VIP leave I = I + X + 1
        case 0x55:
          // store
          for (std::size_t i = 0; i <= X; ++i) {
            memory[(I + i) & 0xFFF] = V[i];
          }
          break;

        // QUIRK: I is left unchanged; the original COSMAC VIP leave I = I + X + 1
        case 0x65:
          // load
          for (std::size_t i = 0; i <= X; ++i) {
            V[i] = memory[(I + i) & 0xFFF];
          }
          break;

        default:
          halt("unknown FX__ instruction", opcode);
          break;
      }
      break;

    default:
      halt("unknown opcode", 0);
      break;
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

void Chip8::halt(const char* reason, std::uint16_t opcode) {
  halted = true;
  std::cerr << std::hex << std::setfill('0');
  std::cerr << "halt: " << reason << " 0x" << std::setw(4) << 
    opcode << " at 0x" << std::setw(4) << pc - 2 << '\n';
  std::cerr << std::dec << std::setfill(' ');
}

void Chip8::set_key(std::size_t index, bool pressed) {
    if (index >= KEY_COUNT) {
      std::cerr << "index is larger than 16: " << index << '\n';
      return;
    }

    key_state[index] = pressed;
}

void Chip8::tick_timers() {
  if (delay_timer > 0) delay_timer -= 1;
  if (sound_timer > 0) sound_timer -= 1;
}

