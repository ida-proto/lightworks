#include "board.h"

#include "stm32h7xx.h"

#include <cstdint>

// ExitRun0Mode and SystemInit run from Reset_Handler before .data is copied
// and before .bss is cleared. They use only registers and the stack.

namespace {

constexpr std::uint32_t kSpinLimit = 2000000u;

[[noreturn]] void halt() {
  for (;;) {
  }
}

void spin_until(const volatile std::uint32_t& reg, std::uint32_t mask, std::uint32_t expect) {
  for (std::uint32_t spin = 0; spin < kSpinLimit; ++spin) {
    if ((reg & mask) == expect) {
      return;
    }
  }
  halt();
}

}  // namespace

static_assert(BOARD_HSE_HZ / BOARD_PLL_M >= 2000000u, "PLL1 input is below the 2-4 MHz range");
static_assert(BOARD_HSE_HZ / BOARD_PLL_M <= 4000000u, "PLL1 input is above the 2-4 MHz range");
static_assert((BOARD_HSE_HZ / BOARD_PLL_M) * BOARD_PLL_N >= 192000000u, "VCO is below the wide range");
static_assert((BOARD_HSE_HZ / BOARD_PLL_M) * BOARD_PLL_N <= 836000000u, "VCO is above the wide range");
static_assert(BOARD_CPU_HZ == (BOARD_HSE_HZ / BOARD_PLL_M) * BOARD_PLL_N / BOARD_PLL_P, "CPU clock");
static_assert(BOARD_HCLK_HZ == BOARD_CPU_HZ / 2u, "AHB clock");
static_assert(BOARD_PLLQ_HZ == (BOARD_HSE_HZ / BOARD_PLL_M) * BOARD_PLL_N / BOARD_PLL_Q, "PLL1Q clock");
static_assert(BOARD_SPI_HZ == BOARD_PLLQ_HZ / BOARD_SPI_PRESCALER, "SPI clock");

extern "C" void ExitRun0Mode(void) {
  // Nucleo-H743ZI2 is the LDO part. This header has no SMPS enable bit.
  PWR->CR3 = (PWR->CR3 | PWR_CR3_LDOEN) & ~PWR_CR3_BYPASS;
  spin_until(PWR->CSR1, PWR_CSR1_ACTVOSRDY, PWR_CSR1_ACTVOSRDY);
}

extern "C" void SystemInit(void) {
  // VOS1 before the clock leaves the 64 MHz HSI. Both VOS bits select scale 1.
  PWR->D3CR = (PWR->D3CR & ~PWR_D3CR_VOS) | PWR_D3CR_VOS_0 | PWR_D3CR_VOS_1;
  spin_until(PWR->D3CR, PWR_D3CR_VOSRDY, PWR_D3CR_VOSRDY);
  spin_until(PWR->CSR1, PWR_CSR1_ACTVOSRDY, PWR_CSR1_ACTVOSRDY);

  // 2 wait states is inside the VOS1 limit at 120 MHz HCLK. Raise it first.
  FLASH->ACR = (FLASH->ACR & ~(FLASH_ACR_LATENCY | FLASH_ACR_WRHIGHFREQ)) | FLASH_ACR_LATENCY_2WS |
               FLASH_ACR_WRHIGHFREQ_1;
  spin_until(FLASH->ACR, FLASH_ACR_LATENCY, FLASH_ACR_LATENCY_2WS);

  // HSEBYP is writable only while HSEON is clear. The Nucleo clock is MCO, not a crystal.
  RCC->CR |= RCC_CR_HSEBYP;
  RCC->CR |= RCC_CR_HSEON;
  spin_until(RCC->CR, RCC_CR_HSERDY, RCC_CR_HSERDY);

  std::uint32_t clock_select = RCC->PLLCKSELR;
  clock_select &= ~(RCC_PLLCKSELR_PLLSRC | RCC_PLLCKSELR_DIVM1);
  clock_select |= RCC_PLLCKSELR_PLLSRC_HSE;
  clock_select |= (BOARD_PLL_M << RCC_PLLCKSELR_DIVM1_Pos);
  RCC->PLLCKSELR = clock_select;

  // Divider fields are stored as N-1. VCOSEL stays 0 (wide VCO, 480 MHz).
  RCC->PLL1DIVR = ((BOARD_PLL_N - 1u) << RCC_PLL1DIVR_N1_Pos) | ((BOARD_PLL_P - 1u) << RCC_PLL1DIVR_P1_Pos) |
                  ((BOARD_PLL_Q - 1u) << RCC_PLL1DIVR_Q1_Pos) | ((BOARD_PLL_R - 1u) << RCC_PLL1DIVR_R1_Pos);
  // PLL1RGE = 01 is the 2-4 MHz input range. Q must be enabled or SPI1 has no kernel clock.
  RCC->PLLCFGR = RCC_PLLCFGR_PLL1RGE_1 | RCC_PLLCFGR_DIVP1EN | RCC_PLLCFGR_DIVQ1EN | RCC_PLLCFGR_DIVR1EN;

  RCC->CR |= RCC_CR_PLL1ON;
  spin_until(RCC->CR, RCC_CR_PLL1RDY, RCC_CR_PLL1RDY);

  // AHB /2 before the switch, so the bus never sees 240 MHz. APB stays at /1 (120 MHz).
  RCC->D1CFGR = (RCC->D1CFGR & ~RCC_D1CFGR_HPRE) | RCC_D1CFGR_HPRE_DIV2;
  // Reset SPI123SEL is already pll1_q. Write it so a later boot path cannot leave it elsewhere.
  RCC->D2CCIP1R &= ~RCC_D2CCIP1R_SPI123SEL;

  RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL1;
  spin_until(RCC->CFGR, RCC_CFGR_SWS, RCC_CFGR_SWS_PLL1);

  RCC->AHB2ENR |= RCC_AHB2ENR_D2SRAM1EN;
  (void)RCC->AHB2ENR;

  // Caches stay off. The DMA buffer is still placed for a later non-cacheable MPU region.
  SCB->VTOR = FLASH_BASE;
  __DSB();
  __ISB();
}
