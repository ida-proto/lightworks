/* STM32H743ZI memory map.
 * Flash 2 MB at 0x08000000.
 * DTCM holds the main stack. DMA cannot see DTCM.
 * AXI SRAM holds .data, .bss, and the FreeRTOS task stacks.
 * D2 SRAM1 holds the SPI DMA buffer. The startup zero-fill does not touch it:
 * that clock is off until SystemInit, and the encoder writes every byte.
 */

ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 2048K
  DTCM (rw)  : ORIGIN = 0x20000000, LENGTH = 128K
  RAM (rw)   : ORIGIN = 0x24000000, LENGTH = 512K
  SRAM1 (rw) : ORIGIN = 0x30000000, LENGTH = 128K
}

_estack = ORIGIN(DTCM) + LENGTH(DTCM);

SECTIONS
{
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } > FLASH

  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.rodata)
    *(.rodata*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)
    KEEP(*(.init))
    KEEP(*(.fini))
    . = ALIGN(4);
  } > FLASH

  .init_array :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array))
    PROVIDE_HIDDEN(__init_array_end = .);
    . = ALIGN(4);
  } > FLASH

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } > RAM AT> FLASH

  _sidata = LOADADDR(.data);

  .bss :
  {
    . = ALIGN(4);
    _sbss = .;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
  } > RAM

  .dma_buffers (NOLOAD) :
  {
    . = ALIGN(32);
    _sdma = .;
    *(.dma_buffers)
    *(.dma_buffers*)
    . = ALIGN(32);
    _edma = .;
  } > SRAM1

  ASSERT(_edma > _sdma, "DMA buffer section is empty")

  /DISCARD/ :
  {
    *(.ARM.exidx*)
    *(.ARM.extab*)
  }
}
