#pragma once

#include "lightworks/rgb.hpp"

namespace lightworks {

// Control plane. Later serial, Wi-Fi, sensors, and OTA write a request here.
// This pass has no input wired, so poll never reports a color and the effect
// keeps its own idle color.
class Control {
 public:
  bool poll(Rgb& requested) const {
    (void)requested;
    return false;
  }
};

}  // namespace lightworks
