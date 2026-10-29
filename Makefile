# ── Compiler and flags ─────────────────────────────────────
CXX      := clang++
CXXFLAGS := -std=c++20 -Wall -Wextra -MMD -MP -Wpedantic -Wimplicit-fallthrough
LDFLAGS  :=
LDLIBS   :=

# ── Directories and output ─────────────────────────────────
SRC_DIR   := src
BUILD_DIR := build
TARGET    := $(BUILD_DIR)/chip8

# ── File lists (computed automatically) ────────────────────
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

# ── Targets ────────────────────────────────────────────────
.PHONY: all clean run

all: $(TARGET)

# Link: combine all object files into the executable
$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LDLIBS)

# Compile: turn each src/X.cpp into build/X.o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create the build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $@

# Build, then run with a ROM:  make run ROM=roms/ibm.ch8
run: $(TARGET)
	./$(TARGET) $(ROM)

# Delete everything generated
clean:
	rm -rf $(BUILD_DIR)

# Pull in auto-generated header dependencies (if they exist)
-include $(DEPS)
