#include "src/runtime.hpp"
#include "doctest.h"

using namespace wasmjit;



TEST_CASE("runtime") {

  auto res = runWasm("../real_examples/exmaple.wasm");
  REQUIRE_EQ(res, 1);
}
