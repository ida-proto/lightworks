#pragma once

#include "sbus/sbus.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace lightworks {

// A transport moves bytes. The app names the concrete type.
// write and read return how many bytes they moved. read returns 0 when idle.
template <typename Transport>
struct TransportTraits {
  template <typename T>
  static auto test(int) -> decltype(std::declval<T&>().write(static_cast<const std::uint8_t*>(nullptr), std::size_t{0}),
                                     std::declval<T&>().read(static_cast<std::uint8_t*>(nullptr), std::size_t{0}),
                                     std::true_type{});
  template <typename>
  static auto test(...) -> std::false_type;
  static constexpr bool ok = decltype(test<Transport>(0))::value;
};

// COBS frames on top of any byte pipe. CAN would be a different transport
// that already has framing; this one is the UART / stream path.
template <typename Transport>
class Link {
 public:
  explicit Link(Transport& transport) : transport_(transport) {
    static_assert(TransportTraits<Transport>::ok, "Transport is missing read/write");
    sbus_decoder_init(&decoder_);
  }

  bool send(const sbus_msg_t& msg) {
    std::uint8_t frame[SBUS_FRAME_MAX];
    const std::size_t n = sbus_frame_encode(&msg, frame, sizeof frame);
    if (n == 0u) {
      return false;
    }
    return transport_.write(frame, n) == n;
  }

  // Pulls until one valid frame, or until the transport is empty.
  bool poll(sbus_msg_t& out) {
    std::uint8_t chunk[16];
    for (;;) {
      const std::size_t n = transport_.read(chunk, sizeof chunk);
      if (n == 0u) {
        return false;
      }
      for (std::size_t i = 0; i < n; ++i) {
        if (sbus_decoder_push(&decoder_, chunk[i], &out)) {
          return true;
        }
      }
    }
  }

 private:
  Transport& transport_;
  sbus_decoder_t decoder_{};
};

}  // namespace lightworks
