#pragma once
#include <string>
#include <cstdint>
#include <cstddef>
#include <array>
#include <random>

class Chip8 {
public:
  // Display 
  static constexpr std::size_t DISPLAY_WIDTH = 64;
  static constexpr std::size_t DISPLAY_HEIGHT = 32;

  // Aliases
  using Display = std::array<bool, DISPLAY_WIDTH * DISPLAY_HEIGHT>;

  // Member Functions
  Chip8(); 
  bool load_rom(const std::string& path);
  void cycle();
  void dump_memory(std::uint16_t start, std::size_t count) const;
  const Display& get_display_array() const { return display_array; }

private: 
  // Compile-time constants
  static constexpr std::uint16_t  START_ADDRESS = 0x200;
  static constexpr std::size_t    MEMORY_SIZE = 4096;
  static constexpr std::uint16_t  FONT_ADDRESS = 0x050;
   
  static constexpr auto FONT = std::to_array<std::uint8_t>({
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80, // F
  });

  static_assert(FONT.size() == 80);
  static_assert(FONT_ADDRESS + FONT.size() <= START_ADDRESS);
  
  std::uint8_t memory[MEMORY_SIZE]{};  // 4KB
  std::uint8_t V[16]{};                // V0-VF, 8 bits each
  std::uint16_t I{};                   // index register
  std::uint16_t pc{START_ADDRESS};     // program counter
  std::uint16_t stack[16]{};           // stack
  std::uint8_t sp{};                   // stack pointer
  std::uint8_t delay_timer{}, sound_timer{};

  // Random generator
  std::mt19937 gen{std::random_device{}()};
  std::uniform_int_distribution<int> dist{0, 255};

  // Display array: 64 x 32 (2048)
  Display display_array{};
};
  
