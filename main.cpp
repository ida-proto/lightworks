// The preprocessor inserts stm32h743.hpp here. That header declares
// stm32::reg and the RCC and GPIOE register addresses used below.
#include "stm32h743.hpp"

// Defines delay. It returns nothing and spends time by counting.
// The counter is volatile, so every increment is a real load, add, and store.
void delay() {
  // Sets i to 0, then repeats while i is below 1000000, adding 1 after each pass.
  // volatile forces a load and a store on every use of i.
  for (volatile unsigned i = 0; i < 1000000u; ++i) {
    // The body is empty. The wait is the test and the increment in the for line.
  }
  // Ends delay and returns to the caller.
}

// Defines the Led class. PE1 is the yellow user LED (LD2) on the Nucleo-H743ZI.
class Led {
  // The members below are callable from outside Led.
 public:
  // Constructor. It runs when a Led is created. The only Led is the static
  // object below, so startup calls this from .init_array before main.
  Led() {
    // Aliases RCC_AHB4ENR (0x580244E0) with a volatile reference, so later uses of ahb4enr hit that register.
    volatile std::uint32_t& ahb4enr = stm32::reg(stm32::rcc::kAhb4enr);
    // Reads RCC_AHB4ENR, sets bit 4 (GPIOEEN) to turn on the GPIOE clock, and writes the register back.
    ahb4enr |= stm32::rcc::kGpioeEn;
    // Reads RCC_AHB4ENR into clock_on. That read waits until the enable write has reached RCC,
    // so the GPIOE clock is on before the first access to the port.
    std::uint32_t const clock_on = ahb4enr;
    // Discards clock_on. The read already happened; this marks the variable used for -Wunused-variable.
    (void)clock_on;

    // Aliases GPIOE_MODER (0x58021000) with a volatile reference.
    volatile std::uint32_t& moder = stm32::reg(stm32::gpioe::kModer);
    // Clears MODE1, bits 3 and 2 of GPIOE_MODER. kPin is 1, so the field starts at bit 2 (1 * 2).
    moder &= ~(3u << (kPin * 2u));
    // Sets bit 2 of GPIOE_MODER. MODE1 becomes 01 binary, which selects general-purpose output.
    moder |= (1u << (kPin * 2u));
    // Closes the constructor. PE1 is now an output.
  }

  // Defines toggle. It returns nothing and flips the LED on each call.
  void toggle() {
    // XORs bit 1 of GPIOE_ODR, which flips the PE1 output level and toggles LD2.
    stm32::reg(stm32::gpioe::kOdr) ^= (1u << kPin);
    // Closes toggle.
  }

  // The member below is visible only inside Led.
 private:
  // Compile-time constant 1, the pin index of PE1. Uses are folded into the shifts above, so it has no storage.
  static constexpr unsigned kPin = 1;
  // Closes class Led.
};

// Defines one Led named led, with static storage and internal linkage.
// Startup zero-fills it, then runs Led::Led before main.
static Led led;

// Entry point. Reset_Handler reaches this with bl main after .data copy, .bss clear, and constructors.
int main() {
  // Repeats the body forever. The condition is the constant 1, so the test is always true.
  while (1) {
    // Calls Led::toggle on led, flipping PE1.
    led.toggle();
    // Calls delay, counting one million iterations before the next flip.
    delay();
    // Closes the while body. Execution returns to the while test.
  }
  // Closes main. The while loop keeps running, so control stays in main.
}
