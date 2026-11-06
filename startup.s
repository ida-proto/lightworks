.syntax unified
.cpu cortex-m7
.thumb

.global Reset_Handler

.section .isr_vector, "a", %progbits
.word _estack
.word Reset_Handler
/* Remaining exception entries are not filled in. A fault will not land
   in a handler until those words are added. */

.section .text
.thumb_func
.type Reset_Handler, %function
Reset_Handler:
  /* Hardware already loaded MSP from the first vector word. Set it again
     so the very next instructions have an obvious stack. */
  ldr r0, =_estack
  mov sp, r0

  /* Copy initial values of global objects from flash (LMA) into RAM (VMA). */
  ldr r0, =_sdata
  ldr r1, =_edata
  ldr r2, =_sidata
copy_data:
  cmp r0, r1
  bhs zero_bss_setup
  ldr r3, [r2], #4
  str r3, [r0], #4
  b copy_data

zero_bss_setup:
  /* Static-duration objects with no explicit initializer must start at 0. */
  ldr r0, =_sbss
  ldr r1, =_ebss
  movs r2, #0
zero_bss:
  cmp r0, r1
  bhs call_constructors
  str r2, [r0], #4
  b zero_bss

call_constructors:
  /* Each .init_array word is a Thumb function pointer (LSB set). */
  ldr r4, =__init_array_start
  ldr r5, =__init_array_end
init_loop:
  cmp r4, r5
  bhs enter_main
  ldr r3, [r4], #4
  /* Four registers keeps SP 8-byte aligned across the call (AAPCS). */
  push {r0, r4, r5, lr}
  blx r3
  pop {r0, r4, r5, lr}
  b init_loop

enter_main:
  bl main
  b .
