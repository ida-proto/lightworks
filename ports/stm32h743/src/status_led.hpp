#pragma once

namespace lightworks::h743::status_led {

void init();
void set(bool on);
void fault_on();

}  // namespace lightworks::h743::status_led
