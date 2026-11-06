CXX = arm-none-eabi-g++
# -O0 keeps each C++ operation visible as its own instructions.
# Freestanding: no libstdc++, no exceptions, no RTTI, no atexit registration.
CXXFLAGS = -mcpu=cortex-m7 -mthumb -std=c++17 -O0 -g -Wall -Wextra \
	-ffreestanding -fno-exceptions -fno-rtti \
	-fno-use-cxa-atexit -fno-threadsafe-statics
LDFLAGS = -T linker.ld -nostdlib -nostartfiles -lgcc

all: firmware.bin

firmware.elf: startup.s main.cpp linker.ld stm32h743.hpp
	$(CXX) $(CXXFLAGS) startup.s main.cpp -o firmware.elf $(LDFLAGS)

firmware.bin: firmware.elf
	arm-none-eabi-objcopy -O binary firmware.elf firmware.bin

clean:
	rm -f firmware.elf firmware.bin
