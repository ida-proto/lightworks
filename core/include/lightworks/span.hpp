#pragma once

#include <cstddef>

namespace lightworks {

// C++17 stand-in for std::span. Pointer plus length, with begin/end so a
// frame can be a range-for. It does not own the memory.
template <typename T>
class Span {
 public:
  constexpr Span() = default;
  constexpr Span(T* data, std::size_t size) : data_(data), size_(size) {}

  constexpr T* data() const { return data_; }
  constexpr std::size_t size() const { return size_; }
  constexpr bool empty() const { return size_ == 0; }

  constexpr T* begin() const { return data_; }
  constexpr T* end() const { return data_ + size_; }

  constexpr T& operator[](std::size_t index) const { return data_[index]; }

 private:
  T* data_ = nullptr;
  std::size_t size_ = 0;
};

}  // namespace lightworks
