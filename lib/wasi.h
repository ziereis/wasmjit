#pragma once
#include <cstdint>
#include <cstdlib>
#include <string_view>
#include <unordered_map>

namespace wasmjit {
namespace preview1 {

inline void proc_exit(int code) { exit(code); }

inline static std::unordered_map<std::string_view, uintptr_t> linkTable = {
  {"proc_exit", reinterpret_cast<uintptr_t>(&proc_exit)}
};

}
}
