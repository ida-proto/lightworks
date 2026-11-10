#pragma once

#include "lightworks/rgb.hpp"
#include "lightworks/span.hpp"

namespace lightworks::h743 {

// SPI1 MOSI on PB5, DMA1 Stream 0, DMAMUX request spi1_tx_dma.
// One instance. The bitstream is hardware; submit() only starts a transfer
// and waits for the SPI end-of-transfer interrupt.
class Ws2815SpiDma {
 public:
  void init();
  bool submit(Span<const Rgb> pixels);
  bool busy() const;

 private:
  bool transfer(Span<const Rgb> pixels);
};

}  // namespace lightworks::h743
