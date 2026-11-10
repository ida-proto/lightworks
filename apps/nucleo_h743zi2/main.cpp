#include "app.hpp"

#include "status_led.hpp"

int main() {
  lightworks::h743::status_led::init();
  lightworks::app::run();
}
