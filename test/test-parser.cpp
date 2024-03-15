#include "doctest.h"
#include "lib/parser.hpp"
#include "lib/tz-utils.hpp"

#include <vector>
#include <string>
#include <filesystem>

using namespace wasmjit;

// TEST_CASE("parser") {

//   MappedFile file("../wasm_examples/add.wasm");

//   WasmModule mod;
//   mod.parseSections(file.asSpan());

//   REQUIRE(mod.functionSection.functions.size() == 2);
// }

TEST_CASE("parser real") {
  MappedFile file("../real_examples/exmaple.wasm");

  WasmModule mod;
  mod.parseSections(file.asSpan());

}
