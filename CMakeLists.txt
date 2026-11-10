cmake_minimum_required(VERSION 3.20)

# Firmware only. Configure with the Arm toolchain file:
#   cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake
#   cmake --build build
# The host encoder test is a separate project in tests/.

project(lightworks LANGUAGES C CXX ASM)

if(NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "cortex-m7")
  message(FATAL_ERROR "Firmware build needs cmake/arm-none-eabi.cmake (processor cortex-m7).")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(firmware
  apps/nucleo_h743zi2/main.cpp
  apps/nucleo_h743zi2/app.cpp
  ports/stm32h743/src/clock.cpp
  ports/stm32h743/src/runtime.cpp
  ports/stm32h743/src/status_led.cpp
  ports/stm32h743/src/ws2815_spi_dma.cpp
  ports/stm32h743/src/freertos_hooks.cpp
  ports/stm32h743/startup/startup_stm32h743xx.s
  third_party/FreeRTOS-Kernel/tasks.c
  third_party/FreeRTOS-Kernel/queue.c
  third_party/FreeRTOS-Kernel/list.c
  third_party/FreeRTOS-Kernel/portable/GCC/ARM_CM4F/port.c
)

target_include_directories(firmware PRIVATE
  core/include
  ports/stm32h743/include
  ports/stm32h743/src
  apps/nucleo_h743zi2
)

target_include_directories(firmware SYSTEM PRIVATE
  third_party/cmsis/Core/Include
  third_party/cmsis_device_h7/Include
  third_party/FreeRTOS-Kernel/include
  third_party/FreeRTOS-Kernel/portable/GCC/ARM_CM4F
)

target_compile_definitions(firmware PRIVATE STM32H743xx)

target_compile_options(firmware PRIVATE
  $<$<COMPILE_LANGUAGE:C,CXX>:-O2 -g -ffunction-sections -fdata-sections -ffreestanding -fno-common>
  $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Werror -Wconversion -Wshadow -fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-threadsafe-statics>
  $<$<COMPILE_LANGUAGE:C>:-Wall -Wextra>
)

target_link_options(firmware PRIVATE
  -T${CMAKE_CURRENT_SOURCE_DIR}/ports/stm32h743/linker/stm32h743zi.ld
  -nostartfiles
  -nostdlib
  -Wl,--gc-sections
  -Wl,-Map=$<TARGET_FILE_DIR:firmware>/firmware.map
)

target_link_libraries(firmware PRIVATE gcc)

set_target_properties(firmware PROPERTIES OUTPUT_NAME firmware SUFFIX ".elf")

add_custom_command(TARGET firmware POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:firmware> $<TARGET_FILE_DIR:firmware>/firmware.bin
  COMMAND ${CMAKE_SIZE} $<TARGET_FILE:firmware>
  VERBATIM
)
