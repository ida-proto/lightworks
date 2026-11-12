cmake_minimum_required(VERSION 3.20)
project(lightworks_tests LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(ws2815_encode_test ws2815_encode_test.cpp)
target_include_directories(ws2815_encode_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../core/include)
target_compile_options(ws2815_encode_test PRIVATE -Wall -Wextra -Werror -Wconversion -Wshadow)

set(SBUS ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/sculpture-bus)
add_executable(link_test
  link_test.cpp
  ${SBUS}/src/crc16.c
  ${SBUS}/src/cobs.c
  ${SBUS}/src/frame.c
  ${SBUS}/src/decoder.c
  ${SBUS}/src/msg.c
  ${SBUS}/src/names.c
  ${SBUS}/src/payload.c
)
target_include_directories(link_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/include
  ${SBUS}/include
)
target_compile_options(link_test PRIVATE -Wall -Wextra -Werror -Wconversion -Wshadow)
