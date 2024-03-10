cmake_minimum_required(VERSION 3.22)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(wasmjit)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(vendor/asmjit)
include_directories(vendor/asmjit/src)
add_subdirectory(vendor/doctest)
include_directories(vendor/doctest/doctest)


add_library(parser lib/parser.cpp)
add_library(compiler lib/compiler.cpp)
include_directories(.)

add_executable(wasmjit src/runtime.cpp)


target_compile_options(wasmjit PRIVATE -Wall -Wextra -g)
target_link_libraries(wasmjit asmjit parser compiler)

add_executable(test test/main.cpp test/test-parser.cpp test/test-compiler.cpp)
target_compile_options(test PRIVATE -Wall -Wextra -g)

target_link_libraries(test parser compiler doctest::doctest asmjit)
