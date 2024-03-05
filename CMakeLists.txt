cmake_minimum_required(VERSION 3.26)
project(wasm__)

set(CMAKE_CXX_STANDARD 23)

add_executable(wasm__ main.cpp)

target_link_libraries(wasm__ asmjit)
include_directories(vendor/asmjit/src
                    vendor/asmjit/build)
add_subdirectory(vendor/asmjit)
