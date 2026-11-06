# Initial explanation

The project is now freestanding C++. It still toggles PE1, and the startup code now does the three things a C++ program requires before `main`: copy `.data`, clear `.bss`, and run constructors. The build with `arm-none-eabi-g++` succeeded. `firmware.bin` is 360 bytes. Nothing was flashed to a board.

`main.c` and the empty `stm32743xx.h` are gone. The sources are `Makefile`, `linker.ld`, `startup.s`, `stm32h743.hpp`, and `main.cpp`. The makefile compiles at `-O0` so each C++ operation is still visible as its own instructions.

## What the chip does, from reset to the LED

This is an STM32H743: a Cortex-M7 with 2 MB of flash at `0x08000000` and 512 KB of AXI SRAM at `0x24000000`. After reset the CPU clock is the internal 64 MHz HSI oscillator. The FPU, caches, and MPU are off. Peripheral interrupt enables in the NVIC are clear. The CPU itself comes out of reset allowed to take exceptions, but no peripheral is requesting one.

When the boot pins select user flash, address `0x00000000` is an alias of `0x08000000`. The Cortex-M7 reset vector table register still points at `0x00000000`, so these two words are what reset reads:

| Address | Bytes in the image | Value | Meaning |
|---|---|---|---|
| `0x08000000` | `00 00 08 24` | `0x24080000` | Initial stack pointer, top of AXI SRAM |
| `0x08000004` | `09 00 00 08` | `0x08000009` | Reset handler at `0x08000008`, with the Thumb bit set |

Reset is hardware, before any instruction of ours runs. The processor loads the main stack pointer from the first word, loads the program counter from the second, and starts in Thumb state, privileged, using that stack. `0x08000009` has bit 0 set because `.thumb_func` marked `Reset_Handler` as a Thumb function. Cortex-M7 only executes Thumb. That bit selects the instruction set. It is not part of the address.

Then our startup runs, in this order:

1. Set the stack pointer to `0x24080000` again.
2. Copy `.data` from flash into RAM. This program has no initialized globals, so the copy loop compares `0x24000000` with `0x24000000` and takes the branch out.
3. Write zeros through `.bss`. That is one word at `0x24000000`. The `Led` object lives there. It has no members, so the zero is just the empty object.
4. Call the one function pointer in `.init_array`. That calls `Led::Led()`, which enables the GPIOE clock and makes PE1 an output.
5. Call `main`. `main` toggles PE1 and busy-waits, forever.

The stack grows downward from `0x24080000`. The only RAM variable sits at `0x24000000`. They do not meet.

## Makefile

`arm-none-eabi-g++` compiles `startup.s` and `main.cpp` together and links them with `linker.ld`.

The flags that define this program:

- `-mcpu=cortex-m7 -mthumb` selects the Cortex-M7 Thumb-2 instruction set.
- `-O0 -g` leaves calls, the frame pointer, and local variables in place, and keeps debug line info in the ELF.
- `-ffreestanding -nostdlib -nostartfiles` means there is no C library and no vendor `crt0`. The reset handler in `startup.s` is the entry point.
- `-fno-exceptions -fno-rtti` omits unwind tables and `typeinfo`.
- `-fno-use-cxa-atexit` keeps a future global destructor out of `__cxa_atexit`, which we do not have. `Led` has a trivial destructor, so the compiler emitted none.
- `-fno-threadsafe-statics` omits the guard variables a local `static` would otherwise use.
- `-lgcc` is the compiler helper library (integer division, and similar). The link used none of it. `nm -u firmware.elf` prints no undefined symbols.

`firmware.elf` is the linked image plus symbols and debug info. `firmware.bin` is the raw flash image: bytes from `0x08000000` through the end of `.init_array`, 360 bytes. `.bss` is not in that file. Startup creates it by writing zeros.

## linker.ld

This file is the memory map of the program.

```
FLASH  0x08000000  2048 KB   executable, read-only
RAM    0x24000000   512 KB   the AXI SRAM in the D1 domain
```

`ENTRY(Reset_Handler)` tells the linker which symbol is the start of the program. The CPU still starts at the vector table. The entry symbol is there for the tools.

Sections, in the order they were linked:

| Section | Runtime address | What is in it |
|---|---|---|
| `.isr_vector` | `0x08000000` | The two vector words. An `ASSERT` checks that this landed at the start of flash. |
| `.text` | `0x08000008` | Instructions, and the constant pools those instructions load. 348 bytes. |
| `.init_array` | `0x08000164` | One pointer: `0x080000CD`, the static constructor stub. |
| `.data` | RAM `0x24000000`, load address `0x08000168` | Initialized globals. Size 0. The load address is where the initial bytes would sit in flash. |
| `.bss` | `0x24000000`, 4 bytes | The `Led` object. `NOLOAD` means it occupies RAM and is absent from `firmware.bin`. |

`_estack` is `0x24000000 + 512 KB = 0x24080000`.

`.data` and `.bss` currently start at the same RAM address because `.data` is empty. A global with an initializer would extend `.data`, and `.bss` would begin after it. `_sidata` is the flash address of those initializers. `_sdata` and `_edata` bracket them in RAM. `_sbss` and `_ebss` bracket the zeroed region.

## startup.s

This is the only code that runs before C++ exists. The assembler listed it like this. Addresses are in the flash.

```
08000008  ldr   r0, [pc, #64]     @ r0 = 0x24080000   (_estack)
0800000a  mov   sp, r0
0800000c  ldr   r0, [pc, #64]     @ r0 = 0x24000000   (_sdata)
0800000e  ldr   r1, [pc, #68]     @ r1 = 0x24000000   (_edata)
08000010  ldr   r2, [pc, #68]     @ r2 = 0x08000168   (_sidata)

copy_data:
08000012  cmp   r0, r1
08000014  bcs.n zero_bss_setup    @ taken immediately: nothing to copy
08000016  ldr.w r3, [r2], #4      @ load a word from flash, then r2 += 4
0800001a  str.w r3, [r0], #4      @ store it to RAM, then r0 += 4
0800001e  b.n   copy_data

zero_bss:
08000020  ldr   r0, =0x24000000   @ _sbss
08000022  ldr   r1, =0x24000004   @ _ebss
08000024  movs  r2, #0
08000026  cmp   r0, r1
08000028  bcs.n call_constructors
0800002a  str.w r2, [r0], #4      @ one store: the Led object becomes 0
0800002e  b.n   zero_bss

init_loop:
08000030  ldr   r4, =0x08000164   @ __init_array_start
08000032  ldr   r5, =0x08000168   @ __init_array_end
08000034  cmp   r4, r5
08000036  bcs.n enter_main
08000038  ldr.w r3, [r4], #4      @ r3 = 0x080000CD, r4 advances to the end
0800003c  push  {r0, r4, r5, lr}
0800003e  blx   r3                @ call the constructor stub
08000040  pop   {r0, r4, r5, lr}
08000044  b.n   init_loop

enter_main:
08000046  bl    main              @ 0x080000A0
0800004a  b.n   0800004a          @ if main ever returned, stay here
```

The words at `0x0800004C` through `0x08000068` are not instructions. They are the literal pool. `ldr r0, [pc, #64]` means: take the address of this instruction plus 4, add 64, and load the word stored there. That is how an immediate larger than the instruction can hold gets into a register. The pool sits after `b.n 0800004a`, so the CPU reaches it only if that branch is overwritten.

`blx r3` is the indirect call. Bit 0 of `0x080000CD` is the Thumb bit. `blx` uses it to stay in Thumb and jumps to `0x080000CC`. `bl` on a direct label writes the return address into `lr`.

`push {r0, r4, r5, lr}` is four registers, 16 bytes. The call standard (AAPCS) wants the stack 8-byte aligned at a call. `r4` and `r5` hold the constructor-loop cursors, and the called function is allowed to destroy `r0`–`r3` and `lr`, so those cursors are saved across `blx`. The stack is full descending: `push` subtracts from `sp` first, then stores.

## stm32h743.hpp

A peripheral register is a word in the address space. `reg()` turns an integer address into a `volatile uint32_t` reference:

```cpp
inline volatile std::uint32_t& reg(std::uintptr_t address) {
  return *reinterpret_cast<volatile std::uint32_t*>(address);
}
```

`volatile` makes the compiler emit each read and write. Without it, a repeated read of a register the program itself just wrote can be deleted, and a write whose value is never read in C++ can be deleted too. The hardware still needs those transactions.

On the bus, addresses `0x40000000`–`0x5FFFFFFF` are Device memory in the Cortex-M7 default map. Device accesses stay in program order with each other and are not cached. SRAM at `0x24000000` is Normal memory. The `Led` object is Normal memory. GPIO and RCC are Device memory.

The addresses used here, from the STM32H743 reference manual:

| Name | Address | Role |
|---|---|---|
| `RCC_AHB4ENR` | `0x580244E0` | Clock enables for GPIO ports. Bit 4 is GPIOE. Reset value `0`. |
| `GPIOE_MODER` | `0x58021000` | Two mode bits per pin. Reset value `0xFFFFFFFF`, every pin in analog mode. |
| `GPIOE_ODR` | `0x58021014` | Output data. Bit N drives pin N. Reset value `0`. |

The enumerators (`kAhb4enr`, `kGpioeEn`, and the rest) are compile-time numbers. They have no address of their own. The instruction stream holds the finished constants `0x580244E0`, `#16`, `#12`, `#4`, and `#2`.

`reg()` compiled to this. A returned reference is the address in `r0`. The function does not touch the peripheral. The caller does.

```
080000d8  push  {r7}
080000da  sub   sp, #12
080000dc  add   r7, sp, #0
080000de  str   r0, [r7, #4]    @ save the address argument
080000e0  ldr   r3, [r7, #4]
080000e2  mov   r0, r3          @ return it
080000e4  add   r7, #12
080000e6  mov   sp, r7
080000e8  pop   {r7}
080000ea  bx    lr
```

`r7` is the frame pointer. At `-O0` every function builds a frame this way: push `r7` (and `lr` if the function calls anything), point `r7` at the locals, and on the way out restore `sp` from `r7` and return. `bx lr` returns to the address in `lr`.

## main.cpp

### The object

```cpp
static Led led;
```

`led` is a function-scope-free object with a constructor, so it is initialized before `main`. `nm` shows it as one byte at `0x24000000`. A class with no data members and no virtual functions still has size 1, so that two different objects have different addresses. There is no vtable pointer, because nothing is virtual. The linker then rounds `.bss` out to 4 bytes, which is why `_ebss` is `0x24000004`.

The constructor's only effect is on the peripherals. The object has nowhere to store state. The call still passes `&led` in `r0`, because a member function's first argument is `this`.

### The constructor call chain

`.init_array` holds `0x080000CD`. That enters three tiny functions:

```
_GLOBAL__sub_I__Z5delayv          @ the pointer in .init_array
  bl  __static_initialization_and_destruction_0

__static_initialization_and_destruction_0
  ldr r0, =0x24000000             @ this = &led
  bl  Led::Led()
```

The stub is named after `delay` because that is the first function in the file. It constructs `led`, not `delay`.

`Led::Led()` appears twice in the symbol table, as `_ZN3LedC1Ev` and `_ZN3LedC2Ev`, both at `0x080000EC`. The C++ ABI emits a complete-object constructor and a base-object constructor. With no base class they are the same code.

The mangled names are the Itanium scheme, which ARM C++ uses:

| Symbol | Decodes to |
|---|---|
| `_Z5delayv` | `delay()` |
| `_ZN3LedC1Ev` | `Led::Led()`, complete object |
| `_ZN3Led6toggleEv` | `Led::toggle()` |
| `_ZN5stm323regEj` | `stm32::reg(unsigned int)` |
| `main` | `main` |

`main` keeps the spelling `main`. The linker resolves `bl main` in the assembly file to `0x080000A0`.

### `Led::Led()` on the bus

PE1's mode is bits 3 and 2 of `GPIOE_MODER`. `00` is input, `01` is general-purpose output, `10` is alternate function, `11` is analog. Reset leaves `11`. The constructor does three read-modify-writes:

```
080000f2  str   r0, [r7, #4]          @ save this

@ AHB4ENR |= (1 << 4)
080000f4  ldr   r0, =0x580244E0
080000f6  bl    stm32::reg
080000fe  ldr   r3, [r3, #0]          @ read  RCC_AHB4ENR
08000100  orr.w r2, r3, #16           @ set bit 4
08000106  str   r2, [r3, #0]          @ write RCC_AHB4ENR

@ clock_on = AHB4ENR
0800010a  ldr   r3, [r3, #0]          @ read it back
0800010c  str   r3, [r7, #16]

@ MODER = (MODER & ~(3 << 2)) | (1 << 2)
0800010e  ldr   r0, =0x58021000
08000110  bl    stm32::reg
08000118  ldr   r3, [r3, #0]          @ read  GPIOE_MODER
0800011a  bic.w r2, r3, #12           @ clear bits 3:2
08000120  str   r2, [r3, #0]          @ write GPIOE_MODER
08000124  ldr   r3, [r3, #0]          @ read  again
08000126  orr.w r2, r3, #4            @ set bit 2 → mode 01
0800012c  str   r2, [r3, #0]          @ write GPIOE_MODER
```

Until bit 4 of `RCC_AHB4ENR` is set, the GPIOE block has no clock and ignores accesses. The extra read of `AHB4ENR` is the delay between turning the clock on and using the port: a later Device read does not complete until the earlier Device write has reached RCC.

`bic` clears the bits set in its immediate. `#12` is `0b1100`, bits 3 and 2. `orr ... #4` sets bit 2. The pair of writes leaves bits 3:2 equal to `01`. Output type, speed, and pull-up stay at their resets: push-pull, low speed, no pull. `ODR` is still 0, so PE1 drives low when it becomes an output.

On a Nucleo-H743ZI, PE1 is the yellow user LED, LD2, lit when the pin is high.

### `main` and `toggle`

```
080000a0  push  {r7, lr}
080000a4  ldr   r0, =0x24000000       @ this = &led
080000a6  bl    Led::toggle()
080000aa  bl    delay()
080000b0  b.n   080000a4
```

`toggle` is a member call, so `this` goes in `r0` even though the function never reads it back:

```
08000146  str   r0, [r7, #4]          @ saved this, unused after this
08000148  ldr   r0, =0x58021014       @ GPIOE_ODR
0800014a  bl    stm32::reg
08000150  ldr   r2, [r3, #0]          @ read ODR
08000152  eor.w r2, r2, #2            @ flip bit 1
08000156  str   r2, [r3, #0]          @ write ODR
```

Each toggle is a read of the whole port, a flip of bit 1 in a register, and a write of the whole port. That is the instruction sequence for `^=` on a volatile word.

### `delay`

The counter is a local `volatile unsigned` on the stack, so each of the million iterations is a real load, add, and store. The compiler turned `i < 1000000` into `i <= 999999`. `999999` is the pool constant `0x000F423F`.

```
08000072  movs  r3, #0
08000074  str   r3, [r7, #4]          @ i = 0
08000076  b.n   0800007e              @ jump to the test

08000078  ldr   r3, [r7, #4]          @ body: load i
0800007a  adds  r3, #1
0800007c  str   r3, [r7, #4]

0800007e  ldr   r3, [r7, #4]          @ test
08000080  ldr   r2, =999999
08000082  cmp   r3, r2
08000084  ite   ls
08000086  movls r3, #1                @ i <= 999999
08000088  movhi r3, #0
0800008a  uxtb  r3, r3
0800008c  cmp   r3, #0
0800008e  bne.n 08000078
0800009a  bx    lr
```

`cmp` sets the flags from a subtraction and throws the numeric result away. `ls` means unsigned lower-or-same. `ite ls` executes the next instruction if that condition is true and the one after it if the condition is false. The loop runs `i` from 0 through 999999, which is one million trips.

This is a busy-wait, not a timer. At the 64 MHz reset clock, a million trips of that body is a visible pause. Building the same source at `-O2` would inline `reg`, `toggle`, and the constructor and shorten the loop, so the LED would blink faster. The registers written would be the same ones.

## What is deliberately absent

The vector table has two entries. A HardFault, or any other exception, would fetch a vector from whatever bytes follow, which are `Reset_Handler` instructions, and then fault again. The rest of the table is the usual next piece: a handler that simply branches to itself, placed in every unused slot.

No PLL, no flash wait-state change, no SysTick, no interrupt. The program runs on the clock and the stack it was given at reset.
