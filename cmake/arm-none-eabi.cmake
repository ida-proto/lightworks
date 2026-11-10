# Arm GNU toolchain file for the STM32H743 (Cortex-M7, hard float).
# Override the install location with the ARM_GNU_TOOLCHAIN environment variable
# or -DARM_GNU_TOOLCHAIN=/path/to/toolchain.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR cortex-m7)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

if(DEFINED ARM_GNU_TOOLCHAIN)
  set(TOOLCHAIN_ROOT "${ARM_GNU_TOOLCHAIN}")
elseif(DEFINED ENV{ARM_GNU_TOOLCHAIN})
  set(TOOLCHAIN_ROOT "$ENV{ARM_GNU_TOOLCHAIN}")
else()
  set(TOOLCHAIN_ROOT "/home/van/.local/toolchains/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi")
endif()

set(CMAKE_C_COMPILER "${TOOLCHAIN_ROOT}/bin/arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_ROOT}/bin/arm-none-eabi-g++")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_ROOT}/bin/arm-none-eabi-gcc")
set(CMAKE_AR "${TOOLCHAIN_ROOT}/bin/arm-none-eabi-ar")
set(CMAKE_RANLIB "${TOOLCHAIN_ROOT}/bin/arm-none-eabi-ranlib")
set(CMAKE_OBJCOPY "${TOOLCHAIN_ROOT}/bin/arm-none-eabi-objcopy")
set(CMAKE_SIZE "${TOOLCHAIN_ROOT}/bin/arm-none-eabi-size")

set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)
set(CMAKE_ASM_COMPILER_WORKS 1)

set(MCU_FLAGS "-mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard")
set(CMAKE_C_FLAGS_INIT "${MCU_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${MCU_FLAGS}")
set(CMAKE_ASM_FLAGS_INIT "${MCU_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${MCU_FLAGS}")

set(CMAKE_FIND_ROOT_PATH "${TOOLCHAIN_ROOT}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
