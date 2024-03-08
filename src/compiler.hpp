#pragma once

#include "tz-utils.hpp"
#include <span>

namespace wasmjit {

class WasmCompiler {
public:
  using entryFunction = int (*)(void);

  entryFunction compile(std::span<const u8> wasmCode);

private:

};

} // namespace wasmjit
