#pragma once
#include <string>
#include <cstdint>
#include <cstddef>

class Chip8 {
public:
  Chip8();
  bool load_rom(const std::string& path);
  void cycle();
  void dump_memory(std::uint16_t start, std::size_t count) const;

private: 
  // Compile-time constnats
  static constexpr std::uint16_t  START_ADDRESS = 0x200;
  static constexpr std::size_t    MEMORY_SIZE = 4096;
  
  std::uint8_t memory[MEMORY_SIZE]{};  // 4KB
  std::uint8_t V[16]{};                // V0-VF, 8 bits each
  std::uint16_t I{};                   // index register
  std::uint16_t pc{START_ADDRESS};     // program counter
  std::uint16_t stack[16]{};           // stack
  std::uint8_t sp{};                   // stack pointer
  std::uint8_t delay_timer{}, sound_timer{}; 

};
  
