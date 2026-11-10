cmake_minimum_required(VERSION 3.20)
project(lightworks_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(ws2815_encode_test ws2815_encode_test.cpp)
target_include_directories(ws2815_encode_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../core/include)
target_compile_options(ws2815_encode_test PRIVATE -Wall -Wextra -Werror -Wconversion -Wshadow)
