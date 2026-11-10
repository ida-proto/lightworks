#pragma once

#include "lightworks/rgb.hpp"
#include "lightworks/span.hpp"

#include <array>
#include <cstddef>

namespace lightworks {

// Two pixel frames. An effect writes back() and then publish() makes that
// frame the one the output driver reads. The other buffer becomes back().
template <std::size_t PixelCount>
class FrameBuffer {
 public:
  static constexpr std::size_t kPixels = PixelCount;

  Span<Rgb> back() { return Span<Rgb>(buffers_[back_].data(), kPixels); }
  Span<const Rgb> front() const { return Span<const Rgb>(buffers_[front_].data(), kPixels); }

  void publish() {
    const std::size_t published = back_;
    back_ = front_;
    front_ = published;
  }

 private:
  std::array<std::array<Rgb, kPixels>, 2> buffers_{};
  std::size_t front_ = 0;
  std::size_t back_ = 1;
};

}  // namespace lightworks
