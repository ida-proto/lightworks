#include "status_led.hpp"

#include "stm32h7xx.h"

#include <cstdint>

namespace lightworks::h743::status_led {
namespace {

constexpr std::uint32_t kLd2Pin = 1u;   // PE1, yellow, active high
constexpr std::uint32_t kLd3Pin = 14u;  // PB14, red, active high

void enable_clock(std::uint32_t bit) {
  RCC->AHB4ENR |= bit;
  (void)RCC->AHB4ENR;
}

void config_output(GPIO_TypeDef* port, std::uint32_t pin) {
  const std::uint32_t shift = pin * 2u;
  const std::uint32_t mask = 3u << shift;
  port->MODER = (port->MODER & ~mask) | (1u << shift);
  port->OTYPER &= ~(1u << pin);
  port->OSPEEDR = (port->OSPEEDR & ~mask) | (2u << shift);
  port->PUPDR &= ~mask;
  port->BSRR = 1u << (pin + 16u);
}

}  // namespace

void init() {
  enable_clock(RCC_AHB4ENR_GPIOBEN);
  enable_clock(RCC_AHB4ENR_GPIOEEN);
  config_output(GPIOE, kLd2Pin);
  config_output(GPIOB, kLd3Pin);
}

void set(bool on) { GPIOE->BSRR = on ? GPIO_BSRR_BS1 : GPIO_BSRR_BR1; }

void fault_on() {
  enable_clock(RCC_AHB4ENR_GPIOBEN);
  config_output(GPIOB, kLd3Pin);
  GPIOB->BSRR = GPIO_BSRR_BS14;
}

}  // namespace lightworks::h743::status_led
