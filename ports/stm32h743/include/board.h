#ifndef LIGHTWORKS_BOARD_H
#define LIGHTWORKS_BOARD_H

/* NUCLEO-H743ZI2 (MB1364, STM32H743ZIT6).
 *
 * HSE is the 8 MHz ST-LINK MCO into PH0, so the clock bring-up uses HSE bypass.
 * PLL1: 8 MHz / 4 * 240 = 480 MHz VCO, /2 = 240 MHz CPU, /2 AHB = 120 MHz,
 * /10 = 48 MHz pll1_q for SPI1. SPI /16 = 3.000 MHz.
 *
 * Arduino D11 (CN7 pin 14) is PB5, SPI1_MOSI, AF5.
 * Factory solder bridges: SB33 open, SB35 closed. Leave them there.
 * PA7 is the other SPI1_MOSI and is Ethernet RMII CRS_DV when SB33 is closed.
 *
 * LD2 (yellow) is PE1, active high. LD3 (red) is PB14, active high.
 */

#define BOARD_HSE_HZ 8000000u
#define BOARD_PLL_M 4u
#define BOARD_PLL_N 240u
#define BOARD_PLL_P 2u
#define BOARD_PLL_Q 10u
#define BOARD_PLL_R 2u

#define BOARD_CPU_HZ 240000000u
#define BOARD_HCLK_HZ 120000000u
/* clock.cpp leaves D2PPRE at reset (/1), so APB1 matches HCLK. USART3 uses that. */
#define BOARD_APB1_HZ BOARD_HCLK_HZ
#define BOARD_PLLQ_HZ 48000000u
#define BOARD_SPI_PRESCALER 16u
#define BOARD_SPI_HZ 3000000u

#define BOARD_PIXEL_COUNT 144u

/* RM0433 Table 121. DMAMUX1 request spi1_tx_dma, wired to DMA1 Stream 0. */
#define BOARD_SPI1_TX_DMA_REQUEST 38u

#endif /* LIGHTWORKS_BOARD_H */
