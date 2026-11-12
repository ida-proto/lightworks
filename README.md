# Lightworks

FreeRTOS lighting core for addressable LEDs. The first board is the NUCLEO-H743ZI2 driving a 144-pixel WS2815 strip. The pixel types, the double buffer, and the WS2815 timing do not mention an MCU. A later port supplies its own output driver and the app names that concrete type. There is no virtual driver.

C++17. Exceptions and RTTI are off. FreeRTOS uses static allocation only. The steady-state path does not allocate.

The first blink experiment is written up in `initial_explanation.md`. Those sources are gone. This tree is the build.

## Layout

| Path | Role |
|---|---|
| `core/include/lightworks/` | `Rgb`, `Span`, `FrameBuffer`, `Ws2815Encoder`, `Control`, `Link` |
| `third_party/sculpture-bus` | Shared sbus protocol, submodule |
| `ports/stm32h743/` | Clock, vectors, FreeRTOS, SPI+DMA driver |
| `apps/nucleo_h743zi2/` | Dim green frame at 30 Hz, LD2 heartbeat |
| `tests/` | Host check of the encoder |
| `third_party/FreeRTOS-Kernel` | FreeRTOS V11.3.3, submodule |
| `third_party/cmsis_device_h7` | STM32H7 CMSIS device headers, submodule |
| `third_party/cmsis` | Vendored CMSIS Core 5.9.0 headers |

`Control` is the seam for serial, Wi-Fi, sensors, and OTA. Nothing is wired to it yet, so the effect keeps the idle color.

## Frame path

The effect task fills the back buffer, waits until the strip is idle, publishes that buffer, and gives a semaphore. The output task has the higher priority, so it runs immediately, expands the front buffer, and starts DMA. SPI1 MOSI is clocked at 3 MHz. Eight SPI bits encode one WS2815 bit (0 is `0x80`, 1 is `0xC0`), MSB first, green then red then blue, followed by a low reset longer than 280 µs. The SPI end-of-transfer interrupt, not a DMA half-done flag, wakes the task after the last reset byte has left the shifter.

The RTOS does not bit-bang. RMT and PIO belong to other MCUs and would be another port behind the same pixel buffer.

## Clock and memory

The Nucleo clock is the 8 MHz ST-LINK MCO into PH0, so startup enables HSE bypass. PLL1 locks a 480 MHz VCO. The CPU runs at 240 MHz, the AHB at 120 MHz, and SPI1's kernel is PLL1Q at 48 MHz divided by 16.

Caches stay off. The transmit buffer is 32-byte aligned in D2 SRAM1 (`0x30000000`) so a later MPU region can mark it non-cacheable without moving it. The main stack is DTCM. Task stacks, the pixel buffers, and `.bss` are AXI SRAM. Startup does not zero SRAM1. The encoder writes every transmit byte.

## Bench

Board: NUCLEO-H743ZI2, MB1364, STM32H743ZIT6.

Data pin: Arduino D11, CN7 pin 14, PB5, SPI1_MOSI, AF5. Factory solder bridges are SB33 open and SB35 closed. Leave them that way. The other SPI1 MOSI is PA7, and closing SB33 hands that pin to Ethernet.

The MCU pin is 3.3 V push-pull into the level shifter. A 74AHCT125 powered from 5 V is the shifter to use. The WS2815 minimum high input is 2.7 V, so a bare 3.3 V pin can be enough, and the shifter is still the setup this image expects.

Power the strip from an external 12 V supply. Do not take strip power from the Nucleo. Connect the supply ground, the shifter ground, and a Nucleo ground together. Tie the first pixel's BI pin to ground.

The demo color is RGB `{0, 40, 0}`. That is a dim green, not full-scale white. Size the 12 V supply for the brightness you intend to run later.

LD2, the yellow LED on PE1, blinks at 1 Hz while the scheduler runs. LD3, the red LED on PB14, stays on if a transfer does not finish or a task overflows its stack.

## Build

Arm GNU toolchain 14.2 is the one this was built with. The toolchain file uses `ARM_GNU_TOOLCHAIN` when it is set, and otherwise `~/.local/toolchains/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi`.

```sh
git submodule update --init
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake
cmake --build build
```

Outputs are `build/firmware.elf` and `build/firmware.bin`.

The encoder test uses the desktop compiler:

```sh
cmake -S tests -B build/host
cmake --build build/host
./build/host/ws2815_encode_test
```

## Programming the board

Configuring and building does not flash the Nucleo. After the strip is wired:

```sh
st-flash write build/firmware.bin 0x08000000
```

OpenOCD can do the same job:

```sh
openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
  -c "program build/firmware.elf verify reset exit"
```

## Roadmap

The addressable LED type will become a choice. This board drives a WS2815 today. WS2812B, SK6812 RGBW, and APA102 (and the families that show up after those) each need their own encoder in front of the same pixel buffer. The effect task should keep filling `Rgb` samples and leave the wire format to the port.

`feat/sculpture-bus` is the first cut of the shared bus on this node. It vendors the `sculpture-bus` protocol as a submodule, adds `Link` (COBS frames over whatever byte pipe the app names), and brings up USART3 on the ST-LINK virtual COM port: PD8 TX, PD9 RX, AF7, 115200 8N1. One heartbeat goes out that port at boot. The baud assumes APB1 is 120 MHz. That path is a skeleton and has not been scoped. `main` stays the LED demo until this branch is ready to merge.
