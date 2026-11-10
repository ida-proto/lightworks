#include "ws2815_spi_dma.hpp"

#include "board.h"
#include "status_led.hpp"

#include "lightworks/ws2815.hpp"

#include "FreeRTOS.h"
#include "semphr.h"

#include "stm32h7xx.h"

#include <cstddef>
#include <cstdint>

namespace lightworks::h743 {
namespace {

constexpr std::size_t kPixels = BOARD_PIXEL_COUNT;
constexpr std::size_t kTxBytes = Ws2815Encoder<BOARD_SPI_HZ>::encoded_size(kPixels);
constexpr std::uint32_t kTimeoutMs = 50u;
constexpr std::uint32_t kMosiPin = 5u;

// MBR 011 is kernel_clock / 16. MBR_2 alone is /32.
constexpr std::uint32_t kSpiBaud = SPI_CFG1_MBR_0 | SPI_CFG1_MBR_1;
constexpr std::uint32_t kSpiEightBit = 7u << SPI_CFG1_DSIZE_Pos;
constexpr std::uint32_t kDmaMemoryToPeripheral =
    DMA_SxCR_DIR_0 | DMA_SxCR_MINC | DMA_SxCR_PL_1;
constexpr std::uint32_t kDmaClearFlags = DMA_LIFCR_CTCIF0 | DMA_LIFCR_CHTIF0 | DMA_LIFCR_CTEIF0 |
                                         DMA_LIFCR_CDMEIF0 | DMA_LIFCR_CFEIF0;
constexpr std::uint32_t kSpiClearFlags = SPI_IFCR_EOTC | SPI_IFCR_OVRC | SPI_IFCR_MODFC;

static_assert(BOARD_SPI_PRESCALER == 16u, "kSpiBaud encodes MBR /16");
static_assert(kSpiBaud == (0x3u << SPI_CFG1_MBR_Pos), "MBR /16 is bits 0 and 1");
static_assert(kTxBytes <= 65535u, "SPI TSIZE and DMA NDTR are 16-bit");

// D2 SRAM1, 32-byte aligned, so a later MPU can mark it non-cacheable.
// Caches are off today. Startup does not zero this section.
alignas(32) std::uint8_t tx_buffer[kTxBytes] __attribute__((section(".dma_buffers"), used));

SemaphoreHandle_t done_sem = nullptr;
StaticSemaphore_t done_storage;
volatile bool busy_flag = false;
bool ready = false;

void enable_clock(volatile std::uint32_t& reg, std::uint32_t bit) {
  reg |= bit;
  const std::uint32_t enabled = reg;
  (void)enabled;
}

void wait_dma_idle() {
  for (std::uint32_t spin = 0; spin < 1000000u; ++spin) {
    if ((DMA1_Stream0->CR & DMA_SxCR_EN) == 0u) {
      return;
    }
  }
}

void stop_transfer() {
  SPI1->CR1 &= ~SPI_CR1_SPE;
  DMA1_Stream0->CR &= ~DMA_SxCR_EN;
  for (std::uint32_t spin = 0; spin < 1000000u; ++spin) {
    if ((SPI1->CR1 & SPI_CR1_SPE) == 0u && (DMA1_Stream0->CR & DMA_SxCR_EN) == 0u) {
      break;
    }
  }
  SPI1->IFCR = kSpiClearFlags;
  DMA1->LIFCR = kDmaClearFlags;
}

void configure_mosi_low() {
  const std::uint32_t shift = kMosiPin * 2u;
  const std::uint32_t mask = 3u << shift;
  GPIOB->BSRR = GPIO_BSRR_BR5;
  GPIOB->OTYPER &= ~GPIO_OTYPER_OT5;
  GPIOB->PUPDR &= ~GPIO_PUPDR_PUPD5;
  // High speed, not very-high, to keep the edge quieter on a jumper wire.
  GPIOB->OSPEEDR = (GPIOB->OSPEEDR & ~GPIO_OSPEEDR_OSPEED5) | GPIO_OSPEEDR_OSPEED5_1;
  GPIOB->AFR[0] = (GPIOB->AFR[0] & ~GPIO_AFRL_AFSEL5) | (5u << GPIO_AFRL_AFSEL5_Pos);
  GPIOB->MODER = (GPIOB->MODER & ~mask) | (1u << shift);
}

void switch_mosi_to_af() {
  const std::uint32_t shift = kMosiPin * 2u;
  const std::uint32_t mask = 3u << shift;
  GPIOB->MODER = (GPIOB->MODER & ~mask) | (2u << shift);
}

}  // namespace

extern "C" void SPI1_IRQHandler(void) {
  if ((SPI1->SR & SPI_SR_EOT) == 0u) {
    return;
  }
  SPI1->IFCR = kSpiClearFlags;
  if (done_sem == nullptr) {
    return;
  }
  BaseType_t wake = pdFALSE;
  (void)xSemaphoreGiveFromISR(done_sem, &wake);
  portYIELD_FROM_ISR(wake);
}

void Ws2815SpiDma::init() {
  if (ready) {
    return;
  }

  enable_clock(RCC->AHB4ENR, RCC_AHB4ENR_GPIOBEN);
  enable_clock(RCC->APB2ENR, RCC_APB2ENR_SPI1EN);
  enable_clock(RCC->AHB1ENR, RCC_AHB1ENR_DMA1EN);

  configure_mosi_low();

  // Simplex transmitter so the RX FIFO cannot stall the shift register.
  // SSM + SSI keep NSS high without a pin. AFCNTR holds MOSI after SPE clears.
  SPI1->CR1 = SPI_CR1_SSI;
  SPI1->CFG1 = kSpiEightBit | kSpiBaud | SPI_CFG1_TXDMAEN;
  SPI1->CFG2 = SPI_CFG2_MASTER | SPI_CFG2_SSM | SPI_CFG2_AFCNTR | SPI_CFG2_COMM_0;
  SPI1->IER = SPI_IER_EOTIE;
  SPI1->IFCR = kSpiClearFlags;

  DMAMUX1_Channel0->CCR = BOARD_SPI1_TX_DMA_REQUEST & DMAMUX_CxCR_DMAREQ_ID;
  DMA1_Stream0->CR &= ~DMA_SxCR_EN;
  wait_dma_idle();
  DMA1->LIFCR = kDmaClearFlags;
  DMA1_Stream0->PAR = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&SPI1->TXDR));
  DMA1_Stream0->FCR = 0;
  DMA1_Stream0->CR = kDmaMemoryToPeripheral;

  done_sem = xSemaphoreCreateBinaryStatic(&done_storage);
  if (done_sem == nullptr) {
    status_led::fault_on();
    return;
  }

  NVIC_SetPriority(SPI1_IRQn, 6u);
  NVIC_EnableIRQ(SPI1_IRQn);
  switch_mosi_to_af();
  ready = true;
}

bool Ws2815SpiDma::busy() const { return busy_flag; }

bool Ws2815SpiDma::submit(Span<const Rgb> pixels) {
  if (!ready || pixels.size() != kPixels) {
    status_led::fault_on();
    return false;
  }
  busy_flag = true;
  const bool sent = transfer(pixels);
  busy_flag = false;
  if (!sent) {
    status_led::fault_on();
  }
  return sent;
}

bool Ws2815SpiDma::transfer(Span<const Rgb> pixels) {
  stop_transfer();
  if (!Ws2815Encoder<BOARD_SPI_HZ>::encode(pixels, Span<std::uint8_t>(tx_buffer, kTxBytes))) {
    return false;
  }
  __DSB();

  DMA1_Stream0->CR &= ~DMA_SxCR_EN;
  wait_dma_idle();
  DMA1->LIFCR = kDmaClearFlags;
  DMA1_Stream0->M0AR = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(tx_buffer));
  DMA1_Stream0->NDTR = static_cast<std::uint32_t>(kTxBytes);
  DMA1_Stream0->CR = kDmaMemoryToPeripheral | DMA_SxCR_EN;

  SPI1->IFCR = kSpiClearFlags;
  SPI1->CR2 = static_cast<std::uint32_t>(kTxBytes);
  SPI1->CR1 |= SPI_CR1_SPE;
  SPI1->CR1 |= SPI_CR1_CSTART;

  if (xSemaphoreTake(done_sem, pdMS_TO_TICKS(kTimeoutMs)) == pdTRUE) {
    stop_transfer();
    return true;
  }

  // The IRQ may have fired after the wait gave up. Drain it before the next frame.
  NVIC_DisableIRQ(SPI1_IRQn);
  __DSB();
  __ISB();
  const bool eot_pending = (SPI1->SR & SPI_SR_EOT) != 0u;
  stop_transfer();
  const BaseType_t late = xSemaphoreTake(done_sem, 0);
  NVIC_ClearPendingIRQ(SPI1_IRQn);
  NVIC_EnableIRQ(SPI1_IRQn);
  return eot_pending || late == pdTRUE;
}

}  // namespace lightworks::h743
