#pragma once

#include "lightworks/rgb.hpp"
#include "lightworks/span.hpp"

#include <cstddef>
#include <cstdint>

namespace lightworks {

// WS2815 timings from the Worldsemi WS2815B datasheet, in picoseconds.
// One wire bit is eight SPI bits, MSB first. A 1-code is the high pattern
// that lands in the datasheet window for this SPI clock. Changing the clock
// fails the build when no pattern still fits.
//
//   T0H 220-380 ns, T0L 900-5000 ns
//   T1H 580-840 ns, T1L 600-5000 ns
//   bit period >= 1.25 us
//   reset low > 280 us
template <std::uint32_t SpiHz>
class Ws2815Encoder {
 public:
  static constexpr std::uint32_t kSpiHz = SpiHz;

 private:
  static constexpr std::uint64_t kPsPerSec = 1000000000000ull;

  static constexpr std::uint64_t bit_ps_floor() {
    return SpiHz == 0u ? 0ull : kPsPerSec / static_cast<std::uint64_t>(SpiHz);
  }

  static constexpr std::uint64_t bit_ps_ceil() {
    if (SpiHz == 0u) {
      return 0ull;
    }
    return (kPsPerSec + static_cast<std::uint64_t>(SpiHz) - 1ull) / static_cast<std::uint64_t>(SpiHz);
  }

  // Smallest high-bit count whose pulse and the remaining low time both sit
  // inside the datasheet window. Zero means this SPI clock cannot encode it.
  static constexpr int choose_high(std::uint64_t high_min_ps, std::uint64_t high_max_ps,
                                   std::uint64_t low_min_ps, std::uint64_t low_max_ps) {
    for (int high = 1; high <= 7; ++high) {
      const auto bits = static_cast<std::uint64_t>(high);
      const auto lows = static_cast<std::uint64_t>(8 - high);
      const std::uint64_t high_floor = bit_ps_floor() * bits;
      const std::uint64_t high_ceil = bit_ps_ceil() * bits;
      const std::uint64_t low_floor = bit_ps_floor() * lows;
      const std::uint64_t low_ceil = bit_ps_ceil() * lows;
      if (high_floor >= high_min_ps && high_ceil <= high_max_ps && low_floor >= low_min_ps &&
          low_ceil <= low_max_ps) {
        return high;
      }
    }
    return 0;
  }

  static constexpr std::uint8_t pattern(int high) {
    if (high <= 0 || high >= 8) {
      return 0;
    }
    return static_cast<std::uint8_t>(0xFFu << (8 - high));
  }

  // Byte count whose low time is strictly longer than min_ps. A truncated
  // picosecond period underestimates the real byte, so the count is rounded up.
  static constexpr std::size_t choose_reset(std::uint64_t byte_ps, std::uint64_t min_ps) {
    if (byte_ps == 0ull) {
      return 0;
    }
    std::uint64_t elapsed = 0;
    std::size_t count = 0;
    while (elapsed <= min_ps) {
      elapsed += byte_ps;
      ++count;
    }
    return count;
  }

 public:
  static constexpr std::uint64_t kBitPsFloor = bit_ps_floor();
  static constexpr std::uint64_t kBitPsCeil = bit_ps_ceil();
  static constexpr int kZeroHigh = choose_high(220000ull, 380000ull, 900000ull, 5000000ull);
  static constexpr int kOneHigh = choose_high(580000ull, 840000ull, 600000ull, 5000000ull);
  static constexpr std::uint8_t kZero = pattern(kZeroHigh);
  static constexpr std::uint8_t kOne = pattern(kOneHigh);
  static constexpr std::uint64_t kBytePsFloor = kBitPsFloor * 8ull;
  static constexpr std::uint64_t kResetMinPs = 280000000ull;
  static constexpr std::size_t kResetBytes = choose_reset(kBytePsFloor, kResetMinPs);

  static_assert(SpiHz > 0u, "SPI clock must be non-zero");
  static_assert(kBitPsFloor > 0ull, "SPI clock is too fast to time a WS2815 bit");
  static_assert(kZeroHigh != 0, "SPI clock cannot form a WS2815 0-code");
  static_assert(kOneHigh != 0, "SPI clock cannot form a WS2815 1-code");
  static_assert(kBytePsFloor >= 1250000ull, "WS2815 bit period is shorter than 1.25 us");
  static_assert(kResetBytes * kBytePsFloor > kResetMinPs, "reset gap must exceed 280 us");

  static constexpr std::size_t encoded_size(std::size_t pixels) { return pixels * 24u + kResetBytes; }

  // Writes GRB, MSB first, then the reset tail. Returns false when out is short.
  static bool encode(Span<const Rgb> pixels, Span<std::uint8_t> out) {
    const std::size_t need = encoded_size(pixels.size());
    if (out.size() < need) {
      return false;
    }
    std::size_t index = 0;
    for (const Rgb& pixel : pixels) {
      index = write_channel(out, index, pixel.g);
      index = write_channel(out, index, pixel.r);
      index = write_channel(out, index, pixel.b);
    }
    for (std::size_t byte = 0; byte < kResetBytes; ++byte) {
      out[index] = 0;
      ++index;
    }
    return true;
  }

 private:
  static std::size_t write_channel(Span<std::uint8_t> out, std::size_t index, std::uint8_t value) {
    for (int bit = 7; bit >= 0; --bit) {
      const auto mask = static_cast<std::uint8_t>(1u << static_cast<unsigned>(bit));
      const bool one = (value & mask) != 0;
      out[index] = one ? kOne : kZero;
      ++index;
    }
    return index;
  }
};

}  // namespace lightworks
