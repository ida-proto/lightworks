#include "lightworks/link.hpp"

#include "sbus/sbus.h"

#include <cstdio>
#include <cstdint>

namespace {

int passed = 0;
int failed = 0;

void expect(bool condition, const char* message) {
  if (condition) {
    ++passed;
    return;
  }
  ++failed;
  std::fprintf(stderr, "FAIL %s\n", message);
}

class Pipe {
 public:
  std::size_t write(const std::uint8_t* data, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
      if (count_ == kCap) {
        return i;
      }
      buf_[count_++] = data[i];
    }
    return n;
  }

  std::size_t read(std::uint8_t* data, std::size_t n) {
    std::size_t take = n;
    if (one_byte_ && take > 1u) {
      take = 1u;
    }
    const std::size_t have = count_ - read_;
    if (take > have) {
      take = have;
    }
    for (std::size_t i = 0; i < take; ++i) {
      data[i] = buf_[read_++];
    }
    return take;
  }

  void flip(std::size_t index) { buf_[index] = static_cast<std::uint8_t>(buf_[index] ^ 0xFFu); }
  void set_one_byte(bool on) { one_byte_ = on; }
  std::size_t size() const { return count_; }

 private:
  static constexpr std::size_t kCap = 128;
  std::uint8_t buf_[kCap]{};
  std::size_t count_ = 0;
  std::size_t read_ = 0;
  bool one_byte_ = false;
};

sbus_msg_t heartbeat() {
  sbus_msg_t msg;
  sbus_msg_clear(&msg);
  const sbus_heartbeat_t body{SBUS_HB_OK, 0u, 12u, 0};
  sbus_pack_heartbeat(&msg, &body);
  msg.version = SBUS_PROTO_MAJOR;
  msg.src = SBUS_NODE_GLOW_FIRST;
  msg.dst = SBUS_NODE_BROADCAST;
  msg.prio = sbus_default_prio(msg.type);
  return msg;
}

void check_round_trip(bool one_byte) {
  Pipe pipe;
  pipe.set_one_byte(one_byte);
  lightworks::Link<Pipe> link(pipe);
  const sbus_msg_t sent = heartbeat();
  expect(link.send(sent), one_byte ? "send one-byte" : "send");
  sbus_msg_t got;
  sbus_msg_clear(&got);
  expect(link.poll(got), one_byte ? "poll one-byte" : "poll");
  expect(got.type == SBUS_HEARTBEAT && got.src == SBUS_NODE_GLOW_FIRST, one_byte ? "fields one-byte" : "fields");
  sbus_heartbeat_t body{};
  expect(sbus_unpack_heartbeat(&got, &body) && body.uptime_s == 12u && body.state == SBUS_HB_OK,
         one_byte ? "payload one-byte" : "payload");
}

void check_corruption() {
  Pipe pipe;
  lightworks::Link<Pipe> link(pipe);
  expect(link.send(heartbeat()), "send before corrupt");
  expect(pipe.size() > 4u, "frame has a body");
  pipe.flip(pipe.size() / 2u);
  sbus_msg_t got;
  sbus_msg_clear(&got);
  expect(!link.poll(got), "corrupt frame dropped");
  expect(link.send(heartbeat()), "send after corrupt");
  expect(link.poll(got) && got.type == SBUS_HEARTBEAT, "next frame still decodes");
}

}  // namespace

int main() {
  check_round_trip(false);
  check_round_trip(true);
  check_corruption();
  std::printf("lightworks_link: %d passed, %d failed\n", passed, failed);
  return failed == 0 ? 0 : 1;
}
