cmake_minimum_required(VERSION 3.26)
project(wasmjit)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(vendor/asmjit)

add_executable(wasmjit src/main.cpp)
target_compile_options(wasmjit PRIVATE -Wall -Wextra -g)
target_link_libraries(wasmjit asmjit)
include_directories(vendor/asmjit/src)
