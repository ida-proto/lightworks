#pragma once

#include <cstdint>

// Register addresses for the STM32H743 (see RM0433).
//
// A peripheral register is a word in the processor's address space, not a
// variable in RAM. volatile forces every read and write to reach the bus:
// the compiler must not cache the value in a register or delete the access.

namespace stm32 {

inline volatile std::uint32_t& reg(std::uintptr_t address) {
  return *reinterpret_cast<volatile std::uint32_t*>(address);
}

// Reset and clock control. GPIO is silent until its clock enable bit is set.
// Enumerators are compile-time numbers. They have no address and no flash slot.
namespace rcc {
enum : std::uintptr_t {
  kBase = 0x58024400u,
  kAhb4enr = kBase + 0xE0u,
};
enum : std::uint32_t {
  kGpioeEn = 1u << 4,
};
}  // namespace rcc

// GPIO port E. Each pin has two mode bits in MODER. ODR bit N drives pin N.
namespace gpioe {
enum : std::uintptr_t {
  kBase = 0x58021000u,
  kModer = kBase + 0x00u,
  kOdr = kBase + 0x14u,
};
}  // namespace gpioe

}  // namespace stm32
