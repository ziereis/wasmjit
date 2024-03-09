#include <optional>
#include <span>
#include <vector>

#include "asmjit/asmjit.h"
#include "asmjit/x86/x86compiler.h"
#include "asmjit/x86/x86operand.h"
#include "compiler.hpp"
#include "parser.hpp"

using namespace asmjit;


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
    std::copy(mainStack.begin(), mainStack.end(),
              std::back_inserter(tempStack));
  }

  void restore() { activeStack = &mainStack; }

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

x86::Gp WasmCompiler::createReg(WasmValueType type) {
  switch (type) {
  case WasmValueType::I32:
    return cc.newInt32();
  case WasmValueType::I64:
    return cc.newInt64();
  // TODO: probably wrong
  case WasmValueType::F32:
    return cc.newInt32();
  case WasmValueType::F64:
    return cc.newInt64();
  default:
    throw std::runtime_error("Invalid type");
  }
}

WasmCompiler::WasmCompiler() : cc(&code) {
  code.init(runtime.environment(), runtime.cpuFeatures());
  code.setLogger(&logger);
}

void WasmCompiler::StartFunction(WasmValueType retType,
                                 std::span<WasmValueType> params) {
  FuncSignature sig;
  sig.setRet(WasmTtoJitT(retType));
  for (auto param : params) {
    sig.addArg(WasmTtoJitT(param));
  }

  auto funcNode = cc.addFunc(sig);
  funcNode->frame().setPreservedFP();
  locals.reserve(params.size());
  for (u32 i = 0; i < params.size(); i++) {
    locals.push_back(createReg(params[i]));
    funcNode->setArg(i, locals.back());
  }
}

void WasmCompiler::AddLocals(std::span<WasmValueType> localTypes) {
  for (auto type : localTypes) {
    locals.push_back(createReg(type));
  }
}

void WasmCompiler::Gts() {

}

void WasmCompiler::EndBlock() {

}

} // namespace wasmjit
