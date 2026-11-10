#include "lightworks/rgb.hpp"
#include "lightworks/span.hpp"
#include "lightworks/ws2815.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
  if (!condition) {
    std::fprintf(stderr, "FAIL %s\n", message);
    failures = 1;
  }
}

using Encoder = lightworks::Ws2815Encoder<3000000>;

static_assert(Encoder::kZero == 0x80);
static_assert(Encoder::kOne == 0xC0);
static_assert(Encoder::kZeroHigh == 1);
static_assert(Encoder::kOneHigh == 2);
static_assert(Encoder::kResetBytes == 106);
static_assert(Encoder::encoded_size(144) == 144u * 24u + 106u);
static_assert(Encoder::encoded_size(1) == 130u);

void check_channel_bits() {
  // G = 0x01 (only the LSB), R = 0x80 (only the MSB), B = 0.
  // Wire order is G, then R, then B, each MSB first.
  const lightworks::Rgb pixel{0x80, 0x01, 0x00};
  std::uint8_t out[130] = {};
  const bool encoded = Encoder::encode(lightworks::Span<const lightworks::Rgb>(&pixel, 1),
                                       lightworks::Span<std::uint8_t>(out, 130));
  expect(encoded, "encode one pixel");

  for (int bit = 0; bit < 7; ++bit) {
    expect(out[static_cast<std::size_t>(bit)] == Encoder::kZero, "G leading zeros");
  }
  expect(out[7] == Encoder::kOne, "G LSB");
  expect(out[8] == Encoder::kOne, "R MSB");
  for (int bit = 9; bit < 16; ++bit) {
    expect(out[static_cast<std::size_t>(bit)] == Encoder::kZero, "R trailing zeros");
  }
  for (int bit = 16; bit < 24; ++bit) {
    expect(out[static_cast<std::size_t>(bit)] == Encoder::kZero, "B zero");
  }
  for (std::size_t index = 24; index < 130; ++index) {
    expect(out[index] == 0, "reset tail");
  }
}

void check_green_order() {
  const lightworks::Rgb pixel{0x00, 0xFF, 0x00};
  std::uint8_t out[130] = {};
  expect(Encoder::encode(lightworks::Span<const lightworks::Rgb>(&pixel, 1),
                         lightworks::Span<std::uint8_t>(out, 130)),
         "encode green");
  for (int bit = 0; bit < 8; ++bit) {
    expect(out[static_cast<std::size_t>(bit)] == Encoder::kOne, "G all ones");
  }
  for (int bit = 8; bit < 24; ++bit) {
    expect(out[static_cast<std::size_t>(bit)] == Encoder::kZero, "R and B zero");
  }
}

void check_strip_length() {
  lightworks::Rgb pixels[144] = {};
  for (lightworks::Rgb& pixel : pixels) {
    pixel = lightworks::Rgb{0, 40, 0};
  }
  constexpr std::size_t kBytes = 144u * 24u + 106u;
  static std::uint8_t out[kBytes] = {};
  expect(Encoder::encode(lightworks::Span<const lightworks::Rgb>(pixels, 144),
                         lightworks::Span<std::uint8_t>(out, kBytes)),
         "encode 144");
  expect(out[0] == Encoder::kZero, "dim green G MSB is clear");
  expect(out[kBytes - 1] == 0, "last reset byte");
  expect(out[144u * 24u - 1] == Encoder::kZero, "last color bit of B=0");

  std::uint8_t short_out[16] = {};
  expect(!Encoder::encode(lightworks::Span<const lightworks::Rgb>(pixels, 144),
                          lightworks::Span<std::uint8_t>(short_out, 16)),
         "short buffer is rejected");
}

}  // namespace

int main() {
  check_channel_bits();
  check_green_order();
  check_strip_length();
  if (failures != 0) {
    return EXIT_FAILURE;
  }
  std::puts("ws2815 encode ok");
  return EXIT_SUCCESS;
}
