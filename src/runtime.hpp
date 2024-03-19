#include <limits>
#include <string_view>
#include "lib/tz-utils.hpp"
#include <limits.h>


namespace wasmjit {

struct LinearMemory {

  void init(u32 num_pages) {
    void* result = mmap(nullptr, num_pages * pageSize, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (result == MAP_FAILED) {
      throw std::runtime_error("Failed to allocate memory");
    }
    mem = static_cast<u8*>(result);
  }

  static constexpr u32 pageSize = 1024 * 64;
  u8 *mem = nullptr;

  ~LinearMemory() {
    munmap(mem, pageSize);
  }
};

  int runWasm(std::string_view fileName);
}
