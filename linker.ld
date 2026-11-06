MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K
  RAM   (rwx) : ORIGIN = 0x24000000, LENGTH = 512K
}

ENTRY(Reset_Handler)

SECTIONS
{
  .isr_vector : {
    __vector_start = .;
    KEEP(*(.isr_vector))
  } > FLASH
  ASSERT(__vector_start == ORIGIN(FLASH), "vector table is not at the start of FLASH")

  .text : {
    *(.text*)
    *(.rodata*)
  } > FLASH

  .init_array : ALIGN(4) {
    __init_array_start = .;
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array))
    __init_array_end = .;
  } > FLASH

  .data : ALIGN(4) {
    _sdata = .;
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } > RAM AT > FLASH
  _sidata = LOADADDR(.data);

  .bss (NOLOAD) : ALIGN(4) {
    _sbss = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
  } > RAM

  /* Stack grows downward from the top of AXI SRAM. Full descending. */
  _estack = ORIGIN(RAM) + LENGTH(RAM);
}
