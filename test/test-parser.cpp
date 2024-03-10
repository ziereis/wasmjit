#include "doctest.h"
#include "lib/parser.hpp"
#include "lib/tz-utils.hpp"

using namespace wasmjit;


TEST_CASE("parser") {

  MappedFile file("../wasm_examples/add.wasm");

  WasmModule mod;
  mod.parseSections(file.asSpan());

  REQUIRE(mod.functionSection.functions.size() == 2);
}
