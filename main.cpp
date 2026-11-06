#include "stm32h743.hpp"

// Busy-wait. The counter is volatile so every increment is a real store/load.
void delay() {
  for (volatile unsigned i = 0; i < 1000000u; ++i) {
  }
}

// PE1 is the yellow user LED (LD2) on the Nucleo-H743ZI.
// The constructor runs before main, from the startup .init_array walk.
class Led {
 public:
  Led() {
    volatile std::uint32_t& ahb4enr = stm32::reg(stm32::rcc::kAhb4enr);
    ahb4enr |= stm32::rcc::kGpioeEn;
    // A following read waits until that write has reached RCC, so GPIOE's
    // clock is on before the first access to the port.
    std::uint32_t const clock_on = ahb4enr;
    (void)clock_on;

    volatile std::uint32_t& moder = stm32::reg(stm32::gpioe::kModer);
    moder &= ~(3u << (kPin * 2u));
    moder |= (1u << (kPin * 2u));
  }

  void toggle() {
    stm32::reg(stm32::gpioe::kOdr) ^= (1u << kPin);
  }

 private:
  static constexpr unsigned kPin = 1;
};

static Led led;

int main() {
  while (1) {
    led.toggle();
    delay();
  }
}
