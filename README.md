# CHIP-8 Emulator

An emulator for **CHIP-8**, the interpreted virtual machine created in the 1970s for hobbyist computers. Give it a CHIP-8 ROM, and it reproduces the behavior of the original machine closely enough to run classic games like Pong, Breakout, and Space Invaders.

<img width="640" height="373" alt="pong" src="https://github.com/user-attachments/assets/576b5b32-f4ae-4f3b-bb37-bdeb4a80abf5" />

---

## Table of Contents

1. [Background](#background)
2. [Building and Running](#building-and-running)
3. [Architecture Specification](#architecture-specification)
4. [Execution Model](#execution-model)
5. [Instruction Set](#instruction-set)
6. [Quirks and Compatibility](#quirks-and-compatibility)
7. [Project Structure](#project-structure)
8. [Testing](#testing)
9. [Roadmap](#roadmap)
10. [References](#references)

---

## Background

### Origins

CHIP-8 was designed around **1977** by **Joseph Weisbecker**, an engineer at RCA. It is not a physical chip. It is a small, invented instruction set, plus an interpreter program that ran it on real hardware. The goal was to make video games easier to write than raw machine code, and to make them portable between machines.

It first ran on the **RCA COSMAC VIP**, a kit computer built around the **RCA 1802** CPU with a 16-key hex keypad, a low-resolution video chip, and a single-tone beeper. The original interpreter was about **512 bytes** long, which is why CHIP-8 programs still load at address `0x200` (512). Later ports included the **Telmac 1800** and the **ETI-660**.

### Uses

CHIP-8 was used almost entirely for simple video games. Its design reflects that: sprite drawing is a built-in instruction, XOR drawing gives free collision detection, and there are timers for pacing and sound, plus a random-number instruction. Programs were commonly shared as printed hex listings and typed in by hand.

### Descendants

| Year | Variant | Author | Changes |
|---|---|---|---|
| 1990 | CHIP-48 | Andreas Gustafsson | Port to the HP-48 calculator; altered several instruction behaviors (see [Quirks](#quirks-and-compatibility)). |
| 1991 | SUPER-CHIP (SCHIP) | Erik Bryntse | 128×64 hi-res mode, scrolling, 16×16 sprites. |
| 2014 | XO-CHIP | John Earnest | Expanded memory, color planes, and improved audio. |

Today CHIP-8 is the standard first emulator project, because it covers the core ideas of CPU emulation (fetch-decode-execute, registers, a stack, memory, timers, and I/O) in only 35 instructions.

---

## Building and Running

**Requirements**

- A C++20 compiler (developed with `clang++` on macOS)
- `make`
- [SDL2](https://wiki.libsdl.org/SDL2/FrontPage) for graphics and input

On macOS with Homebrew:

```sh
brew install sdl2
```

The Makefile locates SDL2 with `sdl2-config`, so no paths need to be edited.

**Build**

```sh
make
```

The executable is written to `build/chip8`. To rebuild from scratch, run `make clean && make`.

**Run**

```sh
make run ROM=roms/path/to/rom.ch8
```

or run the executable directly:

```sh
./build/chip8 roms/path/to/rom.ch8
```

Many ROM filenames contain spaces or brackets, so quote the path:

```sh
make run ROM="roms/chip8-roms/games/Pong (1 player).ch8"
```

Press **Esc** or close the window to quit, and press **0** to restart the ROM (see [Controls](#controls)).

**Getting ROMs**

Classic games are available in [kripod/chip8-roms](https://github.com/kripod/chip8-roms). Clone it into `roms/`, which is gitignored:

```sh
git clone https://github.com/kripod/chip8-roms roms/chip8-roms
```

Only plain CHIP-8 ROMs are supported. SUPER-CHIP and XO-CHIP programs halt with an "unknown opcode" message.

### Controls

The original 16-key hex keypad is mapped to the left side of a QWERTY keyboard:

```
 CHIP-8 keypad        Keyboard
┌───┬───┬───┬───┐   ┌───┬───┬───┬───┐
│ 1 │ 2 │ 3 │ C │   │ 1 │ 2 │ 3 │ 4 │
├───┼───┼───┼───┤   ├───┼───┼───┼───┤
│ 4 │ 5 │ 6 │ D │   │ Q │ W │ E │ R │
├───┼───┼───┼───┤   ├───┼───┼───┼───┤
│ 7 │ 8 │ 9 │ E │   │ A │ S │ D │ F │
├───┼───┼───┼───┤   ├───┼───┼───┼───┤
│ A │ 0 │ B │ F │   │ Z │ X │ C │ V │
└───┴───┴───┴───┘   └───┴───┴───┴───┘
```

Keys are matched by **physical position** (SDL scancodes), so the grid stays the same on non-QWERTY layouts such as AZERTY.

**Emulator controls** (not part of the CHIP-8 keypad):

| Key | Action |
|---|---|
| **0** | Restart the current ROM from scratch |
| **Esc** | Quit |

The **0** key on your keyboard is not CHIP-8 key `0`; that one is **X**.

Example: in Pong, the left paddle uses CHIP-8 keys `1` (up) and `4` (down), which are **1** and **Q** on the keyboard.

---

## Architecture Specification

### Memory

CHIP-8 has **4 KB (4096 bytes)** of byte-addressable RAM, addressed `0x000`–`0xFFF`. Addresses are 12 bits wide.

```
0x000 ┬─ Reserved (originally held the interpreter)
      │    0x050–0x09F: built-in font (by convention)
0x200 ┼─ Program ROM is loaded here; PC starts here
      │
0xFFF ┴─ End of RAM
```

| Address | Decimal | Meaning |
|---|---|---|
| `0x000` | 0 | Start of RAM |
| `0x050` | 80 | Start of font data |
| `0x200` | 512 | Program start |
| `0xFFF` | 4095 | Last byte of RAM |

### Registers

| Register | Size | Description |
|---|---|---|
| `V0`–`VF` | 16 × 8-bit | General-purpose registers. **`VF` is also the flag register** (carry, borrow, and collision), so programs should not use it for data. |
| `I` | 16-bit | Index register, usually holding a memory address (only the low 12 bits are used). |
| `PC` | 16-bit | Program counter: the address of the next instruction. |
| `SP` | 8-bit | Stack pointer. |
| `DT` | 8-bit | Delay timer. |
| `ST` | 8-bit | Sound timer. |

### Stack

The stack holds 16-bit return addresses for subroutine calls (`2NNN`) and returns (`00EE`). The original COSMAC VIP supported 12 levels. This emulator supports **16**, which is the modern convention.

### Timers

Both timers count down at **60 Hz** until they reach 0, independent of instruction speed.

- **Delay timer (`DT`):** read and written by programs for timing.
- **Sound timer (`ST`):** a tone plays while `ST > 0`. CHIP-8 has a single tone, so pitch is up to the emulator.

### Display

- Resolution: **64 × 32** monochrome pixels; origin `(0, 0)` is the top-left.
- Sprites are **8 pixels wide** and **1–15 rows tall**, with each byte forming one row (most significant bit = leftmost pixel).
- Sprites are **XORed** onto the screen. If any pixel is turned from on to off, `VF` is set to `1` (collision); otherwise `VF = 0`.
- The starting coordinates wrap around the screen (`X mod 64`, `Y mod 32`); pixels that extend past the edge are clipped.

### Built-in Font

Sixteen sprites for the hex digits `0`–`F`, each 4 pixels wide and 5 rows tall (5 bytes per character, 80 bytes total), stored starting at `0x050`.

| Char | Bytes | Char | Bytes |
|---|---|---|---|
| `0` | `F0 90 90 90 F0` | `8` | `F0 90 F0 90 F0` |
| `1` | `20 60 20 20 70` | `9` | `F0 90 F0 10 F0` |
| `2` | `F0 10 F0 80 F0` | `A` | `F0 90 F0 90 90` |
| `3` | `F0 10 F0 10 F0` | `B` | `E0 90 E0 90 E0` |
| `4` | `90 90 F0 10 10` | `C` | `F0 80 80 80 F0` |
| `5` | `F0 80 F0 10 F0` | `D` | `E0 90 90 90 E0` |
| `6` | `F0 80 F0 90 F0` | `E` | `F0 80 F0 80 F0` |
| `7` | `F0 10 20 40 40` | `F` | `F0 80 F0 80 80` |

The address of the sprite for digit `d` is `0x050 + d × 5`.

### Input

Sixteen keys, numbered `0x0`–`0xF`. The emulator tracks whether each key is currently pressed (see [Controls](#controls)).

---

## Execution Model

### Instruction Format

Every instruction is **2 bytes**, stored **big-endian** (most significant byte first). For example, the instruction `0xA22A` is stored as `A2` at address `n` and `2A` at address `n + 1`.

### The Cycle

The emulator repeatedly performs:

1. **Fetch:** read the two bytes at `PC` and combine them into a 16-bit opcode: `opcode = (mem[PC] << 8) | mem[PC + 1]`. Then advance `PC` by 2.
2. **Decode:** split the opcode into its fields (below) and identify the instruction.
3. **Execute:** update the machine's state.

Building the opcode with a shift and OR, rather than reading two bytes directly into a 16-bit integer, keeps the result correct regardless of the host machine's endianness.

### Opcode Fields

| Field | Meaning | Extraction |
|---|---|---|
| `X` | 2nd nibble: a register index | `(opcode & 0x0F00) >> 8` |
| `Y` | 3rd nibble: a register index | `(opcode & 0x00F0) >> 4` |
| `N` | 4th nibble: a 4-bit value | `opcode & 0x000F` |
| `NN` | Low byte: an 8-bit value | `opcode & 0x00FF` |
| `NNN` | Low 12 bits: an address | `opcode & 0x0FFF` |

### Timing

The main loop runs at **60 frames per second**. Each frame, the emulator:

1. Reads keyboard input
2. Executes `CYCLES_PER_FRAME` instructions (default 10, so roughly 600 instructions per second)
3. Decrements the timers once (`tick_timers()`)
4. Turns the beep on or off depending on whether `ST > 0`
5. Draws the display
6. Sleeps for the rest of the ~16.67 ms frame

The timers are ticked by the frame loop, not by the CPU, so changing the instruction speed never changes how fast the timers count down. The original hardware had no fixed instruction rate; ~500–1000 instructions per second suits most programs.

### Sound

The beep is a 440 Hz square wave generated in an SDL audio callback (44,100 samples per second, 16-bit mono, 512-sample buffer). The callback only ever produces the wave; the main loop decides whether it is heard by pausing or unpausing the audio device each frame. Because the main thread never touches the callback's state while the device is running, no locking is needed.

### Configuration

These settings are constants at the top of `main.cpp`:

| Constant | Default | Meaning |
|---|---|---|
| `SCALE` | 15 | Window size multiplier (64×32 → 960×480) |
| `CYCLES_PER_FRAME` | 10 | Instructions executed per frame (CPU speed) |
| `TARGET_FPS` | 60 | Frame rate; also the timer rate |
| `COLOR_ON` / `COLOR_OFF` | white / black | Pixel colors (ARGB) |

Pitch and volume are fields of `BeepState` (440 Hz, volume 3000 of 32767).

---

## Instruction Set

| Opcode | Mnemonic | Description |
|---|---|---|
| `0NNN` | `SYS NNN` | Call a native machine routine. **Ignored** by modern emulators. |
| `00E0` | `CLS` | Clear the display. |
| `00EE` | `RET` | Return from a subroutine: pop the address from the stack into `PC`. |
| `1NNN` | `JP NNN` | Jump to `NNN`. |
| `2NNN` | `CALL NNN` | Push `PC` onto the stack, then jump to `NNN`. |
| `3XNN` | `SE VX, NN` | Skip the next instruction if `VX == NN`. |
| `4XNN` | `SNE VX, NN` | Skip the next instruction if `VX != NN`. |
| `5XY0` | `SE VX, VY` | Skip the next instruction if `VX == VY`. |
| `6XNN` | `LD VX, NN` | `VX = NN`. |
| `7XNN` | `ADD VX, NN` | `VX += NN` (8-bit wraparound; `VF` is **not** changed). |
| `8XY0` | `LD VX, VY` | `VX = VY`. |
| `8XY1` | `OR VX, VY` | `VX \|= VY`. |
| `8XY2` | `AND VX, VY` | `VX &= VY`. |
| `8XY3` | `XOR VX, VY` | `VX ^= VY`. |
| `8XY4` | `ADD VX, VY` | `VX += VY`; `VF = 1` on carry, else `0`. |
| `8XY5` | `SUB VX, VY` | `VX = VX - VY`; `VF = 1` if no borrow (`VX >= VY`), else `0`. |
| `8XY6` | `SHR VX` | Shift right by 1; `VF` = the bit shifted out. See quirks. |
| `8XY7` | `SUBN VX, VY` | `VX = VY - VX`; `VF = 1` if no borrow (`VY >= VX`), else `0`. |
| `8XYE` | `SHL VX` | Shift left by 1; `VF` = the bit shifted out. See quirks. |
| `9XY0` | `SNE VX, VY` | Skip the next instruction if `VX != VY`. |
| `ANNN` | `LD I, NNN` | `I = NNN`. |
| `BNNN` | `JP V0, NNN` | Jump to `NNN + V0`. See quirks. |
| `CXNN` | `RND VX, NN` | `VX = (random byte) & NN`. |
| `DXYN` | `DRW VX, VY, N` | Draw an `N`-row sprite from memory at `I` to `(VX, VY)`; `VF` = collision. |
| `EX9E` | `SKP VX` | Skip the next instruction if the key in `VX` is pressed. |
| `EXA1` | `SKNP VX` | Skip the next instruction if the key in `VX` is not pressed. |
| `FX07` | `LD VX, DT` | `VX = DT`. |
| `FX0A` | `LD VX, K` | Wait until a key is pressed **and released**, then store it in `VX` (COSMAC VIP behavior). Execution halts; timers keep running. |
| `FX15` | `LD DT, VX` | `DT = VX`. |
| `FX18` | `LD ST, VX` | `ST = VX`. |
| `FX1E` | `ADD I, VX` | `I += VX`. |
| `FX29` | `LD F, VX` | `I` = address of the font sprite for the hex digit in `VX`. |
| `FX33` | `LD B, VX` | Store the binary-coded decimal of `VX`: hundreds at `I`, tens at `I+1`, ones at `I+2`. |
| `FX55` | `LD [I], VX` | Store `V0` through `VX` in memory starting at `I`. See quirks. |
| `FX65` | `LD VX, [I]` | Load `V0` through `VX` from memory starting at `I`. See quirks. |

> **Flag ordering:** for `8XY4`–`8XYE`, compute the flag value first, write the result to `VX`, and then write the flag to `VF`. Otherwise, when `X` is `F`, the result overwrites the flag. Test ROMs check this.

---

## Quirks and Compatibility

CHIP-48 (1990) changed the behavior of several instructions, and later programs were written against those changes. As a result, there are two "correct" behaviors for the instructions below. Classic 1970s programs expect the COSMAC VIP behavior; most programs from the 1990s onward expect CHIP-48/SCHIP behavior.

| Instruction | COSMAC VIP (original) | CHIP-48 / SCHIP | This emulator |
|---|---|---|---|
| `8XY6` / `8XYE` | Sets `VX = VY`, then shifts | Shifts `VX` in place; ignores `VY` | CHIP-48 |
| `FX55` / `FX65` | `I` is incremented (ends at `I + X + 1`) | `I` is unchanged | CHIP-48 |
| `BNNN` | Jumps to `NNN + V0` | Jumps to `XNN + VX` | COSMAC VIP |
| `8XY1` / `8XY2` / `8XY3` | Resets `VF` to 0 | `VF` unchanged | CHIP-48 |
| `DXYN` | Waits for vertical blank (max ~60 draws/sec) | No wait | No wait |
| Sprites at screen edges | Clipped | Clipped | Clipped |

The current behavior is a mix of both platforms. The plan is to make each quirk configurable, defaulting to the COSMAC VIP behavior, and to verify the result against Timendus test 5 (quirks). Each quirk is marked with a `// QUIRK:` comment in `chip8.cpp`.

---

## Project Structure

```
.
├── Makefile          # builds build/chip8; links SDL2 via sdl2-config
├── README.md
├── src/
│   ├── chip8.h       # core: machine state and public interface
│   ├── chip8.cpp     # core: ROM loading, fetch-decode-execute, timers, input state
│   └── main.cpp      # frontend: SDL2 window, audio, key mapping, 60 Hz main loop
├── roms/             # ROM files (downloaded collections are gitignored)
└── build/            # object files and the executable (gitignored)
```

The emulator is split into two layers:

- **Core (`chip8.h` / `chip8.cpp`):** machine state, ROM loading, and the fetch-decode-execute cycle. It has no knowledge of windows, keyboards, or audio devices; it exposes the display through `get_display_array()` and receives input through `set_key()`.
- **Frontend (`main.cpp`):** renders the 64×32 display to a scaled SDL2 window, maps keyboard input to the 16 keys, plays the beep, and runs the main loop that drives `cycle()` and `tick_timers()`. It reads the sound state through `is_sound_on()`.

Keeping these separate lets the core be tested without a window.

---

## Testing

Test ROMs used to verify correctness:

| ROM | Verifies | Status |
|---|---|---|
| Timendus 2: IBM Logo | Fetch, decode, memory, and drawing (`00E0`, `1NNN`, `6XNN`, `7XNN`, `ANNN`, `DXYN`) | Pass |
| Timendus 3: Corax+ opcode test | Each instruction's basic behavior | Pass |
| Timendus 4: Flags | `VF` carry, borrow, and shift behavior, including flag ordering | Pass |
| Timendus 5: Quirks | Which platform's quirks the emulator matches | Not yet run |
| Timendus 6: Keypad | `EX9E`, `EXA1`, `FX0A` (including wait-for-release) | Pass |
| Timendus 7: Beep | Sound timer and audio output (SOS pattern, manual control with key B) | Pass (see known issues) |
| Pong (from chip8-roms) | Input, timers, and gameplay end to end | Playable |

Unrecognized opcodes are reported (with their address) and halt execution, which makes missing or incorrect instructions easy to find.

### Known Issues

- **The first beep after launch can be cut off.** The audio device starts its output stream on the first unpause, which can take longer than a short beep. Later beeps are unaffected. Planned fix: keep the device running and gate the wave with a `std::atomic<bool>`.
- **Frame timing runs slightly slow.** `SDL_Delay` tends to oversleep, and the overshoot accumulates. Planned fix: deadline-based frame scheduling.
- **Some games may depend on quirks** (see [Quirks](#quirks-and-compatibility)); behavior differences are being collected for the configurable-quirks work.
- On macOS, a harmless `IMKCFRunLoopWakeUpReliable` message may be printed at startup by the system's input method framework.

---

## Roadmap

- [x] Window that draws a 64×32 grid
- [x] Machine state and ROM loader
- [x] Fetch-decode-execute loop
- [x] IBM Logo opcodes; logo displays
- [x] Remaining opcodes; opcode and flag tests pass
- [x] Keyboard input
- [x] Delay and sound timers counting down at 60 Hz
- [x] Keypad test (Timendus 6) passes
- [x] Audible tone while `ST > 0`
- [x] Restart (0) and quit (Esc) keys
- [ ] Glitch-free first beep (always-running audio with `std::atomic<bool>`)
- [ ] Deadline-based frame timing
- [ ] Configurable quirks with COSMAC VIP defaults; quirks test (Timendus 5) passes
- [ ] Configurable speed and colors
- [ ] RAII wrapper for SDL resources
- [ ] _Stretch:_ SUPER-CHIP support

---

## References

- Cowgod's CHIP-8 Technical Reference: http://devernay.free.fr/hacks/chip8/C8TECH10.HTM
- Tobias V. Langhoff, *Guide to making a CHIP-8 emulator*: https://tobiasvl.github.io/blog/write-a-chip-8-emulator/
- Timendus, CHIP-8 Test Suite: https://github.com/Timendus/chip8-test-suite
- kripod, CHIP-8 ROM collection: https://github.com/kripod/chip8-roms
- SDL2 documentation (wiki): https://wiki.libsdl.org/SDL2/FrontPage
- Joseph Weisbecker, "An Easy Programming System," *BYTE*, December 1978
