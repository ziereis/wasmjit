#include <vector>
#include <optional>

#include "asmjit/asmjit.h"
#include "wasm-types.hpp"
#include "compiler.hpp"
#include "parser.hpp"

using namespace asmjit;

constexpr OpcodeStringTable g_wasmOpcodeStringTable;

namespace wasmjit {

class OperandStack {
public:
  OperandStack() { activeStack = &mainStack; }

  void push(x86::Gp reg) { activeStack->push_back(reg); }

  x86::Gp pop() {
    auto reg = activeStack->back();
    activeStack->pop_back();
    return reg;
  }

  x86::Gp &peek() { return activeStack->back(); }

  void snapshot() {
    assert(activeStack == &mainStack);
    activeStack = &tempStack;
    tempStack.clear();
    std::copy(mainStack.begin(), mainStack.end(), std::back_inserter(tempStack));
  }

  void restore() {
    activeStack = &mainStack;
  }

  void clear() { mainStack.clear(); }

  bool empty() { return activeStack->empty(); }
  std::size_t size() { return activeStack->size(); }

private:
  std::vector<x86::Gp> *activeStack;
  std::vector<x86::Gp> mainStack;
  std::vector<x86::Gp> tempStack;
};

struct BlockInfo {
  Label label;
  std::optional<x86::Gp> mergeReg;
};

static TypeId WasmTtoJitT(WasmValueType type) {
    switch (type) {
    case WasmValueType::I32:
      return TypeId::kInt32;
    case WasmValueType::I64:
      return TypeId::kInt64;
    case WasmValueType::F32:
      return TypeId::kInt32;
    case WasmValueType::F64:
      return TypeId::kInt64;
    case WasmValueType::NONE:
      return TypeId::kVoid;
    default:
      throw std::runtime_error("Invalid type");
    }
  }

} // namespace wasmjit
