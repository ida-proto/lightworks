#include "usart3_vcp.hpp"

#include "board.h"

#include "stm32h7xx.h"

namespace lightworks::h743 {
namespace {

constexpr std::uint32_t kTxPin = 8u;
constexpr std::uint32_t kRxPin = 9u;
constexpr std::uint32_t kAf = 7u;
constexpr std::uint32_t kSpinLimit = 100000u;

// Round to the nearest divider. 120 MHz / 115200 is 1041.67, so the field is 1042.
constexpr std::uint32_t kBaud = 115200u;
constexpr std::uint32_t kBrr = (BOARD_APB1_HZ + (kBaud / 2u)) / kBaud;

static_assert(BOARD_APB1_HZ == BOARD_HCLK_HZ, "USART3 clock follows the undivided APB1");
static_assert(kBrr >= 16u && kBrr <= 65535u, "BRR out of range");

void enable_clock(volatile std::uint32_t& reg, std::uint32_t bit) {
  reg |= bit;
  const std::uint32_t sink = reg;
  (void)sink;
}

void config_pin(std::uint32_t pin, bool pullup) {
  const std::uint32_t shift = pin * 2u;
  const std::uint32_t mask = 3u << shift;
  GPIOD->MODER = (GPIOD->MODER & ~mask) | (2u << shift);
  GPIOD->OTYPER &= ~(1u << pin);
  GPIOD->OSPEEDR = (GPIOD->OSPEEDR & ~mask) | (2u << shift);
  const std::uint32_t pull = pullup ? (1u << shift) : 0u;
  GPIOD->PUPDR = (GPIOD->PUPDR & ~mask) | pull;

  const std::uint32_t nibble = (pin - 8u) * 4u;
  const std::uint32_t nibble_mask = 0xFu << nibble;
  GPIOD->AFR[1] = (GPIOD->AFR[1] & ~nibble_mask) | (kAf << nibble);
}

bool wait_txe() {
  for (std::uint32_t spin = 0; spin < kSpinLimit; ++spin) {
    if ((USART3->ISR & USART_ISR_TXE_TXFNF) != 0u) {
      return true;
    }
  }
  return false;
}

}  // namespace

void Usart3Vcp::init() {
  enable_clock(RCC->AHB4ENR, RCC_AHB4ENR_GPIODEN);
  enable_clock(RCC->APB1LENR, RCC_APB1LENR_USART3EN);
  config_pin(kTxPin, false);
  config_pin(kRxPin, true);

  USART3->CR1 = 0u;
  USART3->BRR = kBrr;
  USART3->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

std::size_t Usart3Vcp::write(const std::uint8_t* data, std::size_t n) {
  std::size_t sent = 0;
  for (; sent < n; ++sent) {
    if (!wait_txe()) {
      break;
    }
    USART3->TDR = data[sent];
  }
  return sent;
}

std::size_t Usart3Vcp::read(std::uint8_t* data, std::size_t n) {
  std::size_t got = 0;
  while (got < n && (USART3->ISR & USART_ISR_RXNE_RXFNE) != 0u) {
    data[got] = static_cast<std::uint8_t>(USART3->RDR & 0xFFu);
    ++got;
  }
  return got;
}

}  // namespace lightworks::h743
