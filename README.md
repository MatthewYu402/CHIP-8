# CHIP-8 Emulator

An emulator for **CHIP-8**, the interpreted virtual machine created in the 1970s for hobbyist computers. Give it a CHIP-8 ROM, and it reproduces the behavior of the original machine closely enough to run classic games like Pong, Breakout, and Space Invaders.

<!-- TODO: add a screenshot or GIF of a game running -->

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

<!-- TODO: fill in once the language and libraries are chosen -->

**Requirements**

- Language / toolchain: _TBD_
- Graphics, input, and audio library: _TBD_ (e.g., SDL2, pygame, HTML canvas)

**Build**

```
TBD
```

**Run**

```
TBD <path/to/rom.ch8>
```

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

Each frame (at 60 frames per second), the emulator:

1. Reads keyboard input
2. Executes about 10 instructions (roughly 600 instructions per second)
3. Decrements the timers
4. Draws the display
5. Plays the tone if `ST > 0`

The original hardware had no fixed instruction rate; ~500–1000 instructions per second suits most programs.

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
| `FX0A` | `LD VX, K` | Wait until a key is pressed, then store it in `VX`. Execution halts; timers keep running. |
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
| `8XY6` / `8XYE` | Sets `VX = VY`, then shifts | Shifts `VX` in place; ignores `VY` | _TBD_ |
| `FX55` / `FX65` | `I` is incremented (ends at `I + X + 1`) | `I` is unchanged | _TBD_ |
| `BNNN` | Jumps to `NNN + V0` | Jumps to `XNN + VX` | _TBD_ |
| `8XY1` / `8XY2` / `8XY3` | Resets `VF` to 0 | `VF` unchanged | _TBD_ |
| `DXYN` | Waits for vertical blank (max ~60 draws/sec) | No wait | _TBD_ |
| Sprites at screen edges | Clipped | Clipped | Clipped |

<!-- TODO: fill in the last column, or document how to configure each quirk -->

---

## Project Structure

<!-- TODO: update with the real file layout -->

The emulator is split into two layers:

- **Core (virtual machine):** machine state, ROM loading, and the fetch-decode-execute cycle. It has no knowledge of windows, keyboards, or audio devices.
- **Frontend (I/O):** renders the 64×32 display to a scaled window, maps keyboard input to the 16 keys, plays the tone, and runs the main loop.

Keeping these separate lets the core be tested without a window.

---

## Testing

Test ROMs used to verify correctness:

| ROM | Verifies |
|---|---|
| IBM Logo | Fetch, decode, memory, and drawing (`00E0`, `1NNN`, `6XNN`, `7XNN`, `ANNN`, `DXYN`) |
| Timendus CHIP-8 Test Suite | Each instruction, flag behavior, quirks, keypad, and sound |

Unrecognized opcodes are reported (with their address) and halt execution, which makes missing or incorrect instructions easy to find.

---

## Roadmap

- [ ] Window that draws a 64×32 grid
- [ ] Machine state and ROM loader
- [ ] Fetch-decode-execute loop
- [ ] IBM Logo opcodes; logo displays
- [ ] Remaining opcodes; test suite passes
- [ ] Keyboard input
- [ ] Delay and sound timers; audible tone
- [ ] Configurable quirks
- [ ] _Stretch:_ SUPER-CHIP support

---

## References

- Cowgod's CHIP-8 Technical Reference: http://devernay.free.fr/hacks/chip8/C8TECH10.HTM
- Tobias V. Langhoff, *Guide to making a CHIP-8 emulator*: https://tobiasvl.github.io/blog/write-a-chip-8-emulator/
- Timendus, CHIP-8 Test Suite: https://github.com/Timendus/chip8-test-suite
- Joseph Weisbecker, "An Easy Programming System," *BYTE*, December 1978