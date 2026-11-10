#pragma once

#include <cstdint>

namespace lightworks {

// Core pixel order is RGB. The WS2815 wire order is GRB, and the encoder
// swaps the channels. Ports never see a packed wire byte from an effect.
struct Rgb {
  std::uint8_t r;
  std::uint8_t g;
  std::uint8_t b;
};

}  // namespace lightworks
