#pragma once

#include <cstddef>
#include <cstdint>

namespace lightworks::h743 {

// ST-LINK virtual COM port on the Nucleo-H743ZI2: USART3, PD8 TX, PD9 RX, AF7.
// Baud assumes APB1 is still 120 MHz (clock.cpp leaves the APB prescaler at /1
// and the USART kernel on pclk1). Proposed, unverified against a scope.
class Usart3Vcp {
 public:
  void init();
  std::size_t write(const std::uint8_t* data, std::size_t n);
  std::size_t read(std::uint8_t* data, std::size_t n);
};

}  // namespace lightworks::h743
