#include <array>
#include <cstdint>
#include <cstdio>
#include <span>
#include <unordered_set>

#include "asmjit/x86/x86operand.h"
#include "compiler.hpp"
#include "lib/tz-utils.hpp"
#include "parser.hpp"

using namespace asmjit;

namespace wasmjit {

OperandStack::OperandStack() : frozenIdx(-1) {}

void OperandStack::push(x86::Gp reg) { stack.push_back(reg); }

x86::Gp OperandStack::pop() {
  assert(!empty() && "OperandStack::pop() called on empty stack");
  auto reg = stack.back();
  stack.pop_back();
  return reg;
}

x86::Gp &OperandStack::peek() {
  assert(!empty() && "OperandStack::peek() called on empty stack");
  return stack.back();
}

void OperandStack::clear() { stack.clear(); }

bool OperandStack::empty() const { return stack.empty(); }

std::size_t OperandStack::size() { return stack.size(); }

void OperandStack::initFrom(OperandStack &other, u32 in) {
  stack.resize(in);
  std::copy(other.stack.end() - in, other.stack.end(), stack.begin());
  other.stack.resize(other.size() - in);
}


void OperandStack::deduplicate(x86::Compiler &cc) {
  std::unordered_set<u32> seen;
  std::vector<x86::Gp> deduped;
  deduped.reserve(stack.size());
  for (auto reg : stack) {
    if (seen.find(reg.id()) == seen.end()) {
      std::cout << reg.id() << std::endl;
      deduped.push_back(reg);
      seen.insert(reg.id());
    } else {
      auto new_reg = cc.newSimilarReg(reg);
      cc.mov(new_reg, reg);
      deduped.push_back(new_reg);
    }
  }
  stack = std::move(deduped);
}

void OperandStack::transferFrom(x86::Compiler &cc, OperandStack &other, u64 count) {
  assert(frozenIdx != -1 && "OperandStack::transferFrom() called on unfrozen stack");
  assert(other.size() >= count);
  auto transferred = stack.size() - frozenIdx;
  auto newSize =  frozenIdx + std::max(count, transferred);

  stack.resize(newSize);
  u32 i = frozenIdx;
  u32 mergeCount = std::min(count, transferred);
  for (; i < frozenIdx + mergeCount; i++) {
    if (stack[i] != other.stack[i]) {
      cc.mov(stack[i], other.stack[i]);
    }
  }
  for (; i < newSize; i++) {
    stack[i] = other.stack[i];
  }
}

void OperandStack::freeze() {
  frozenIdx = stack.size();
}

void OperandStack::unfreeze() {
  frozenIdx = -1;
}


void BlockManager::pushBlock() {
  blocks.push_back({{}, {}, {}, 0});
  activeBlock++;
}
void BlockManager::popBlock() {
  assert(!empty() && "BlockManager::popBlock() called on empty stack");
  blocks.pop_back();
  activeBlock--;
}

BlockState &BlockManager::getActive() {
  assert(!empty() && "BlockManager::getActive() called on empty stack");
  return blocks[activeBlock];
}

BlockState &BlockManager::getParent() {
  assert(activeBlock > 0 &&
         "BlockManager::getParent() called on invalid stack");
  return blocks[activeBlock - 1];
}

BlockState &BlockManager::getRelative(i32 depth) {
  assert(activeBlock >= depth &&
         "BlockManager::getRelative() called with invalid depth");
  return blocks[activeBlock - depth];
}

BlockState &BlockManager::getByDepth(i32 depth) {
  assert(depth >= 0 && depth < blocks.size());
  return blocks[depth];
}

void BlockManager::initFromRelative(i32 depth) {
  assert(activeBlock >= depth &&
         "BlockManager::initFromRelative() called with invalid depth");
  blocks[activeBlock].stack = blocks[activeBlock - depth].stack;
}


bool BlockManager::stackEmpty() const {
  return blocks[activeBlock].stack.empty();
}

bool BlockManager::empty() const { return blocks.empty(); }
std::size_t BlockManager::size() const { return blocks.size(); }

void BlockManager::clear() {
  blocks.clear();
  activeBlock = -1;
}


WasmCompiler::WasmCompiler(u32 funcCount) {
  std::stringstream temp;
  dbg.swap(temp);
  code.init(runtime.environment(), runtime.cpuFeatures());
  code.setLogger(&logger);
  code.attach(&cc);
  fnLabels.reserve(funcCount);
  for (u32 i = 0; i < funcCount; i++) {
    fnLabels.push_back(cc.newLabel());
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

void WasmCompiler::StartFunction(u32 index, WasmValueType retType,
                                 std::span<WasmValueType> params) {
  LOG_DEBUG_CC("StartFunction Index: {}, retType: {}, params: {}", index, toString(retType), params.size());
  // this is for return
  {
    returnType = retType;
    blockMngr.pushBlock();
    auto& block = blockMngr.getActive();
    block.outArity = retType != WasmValueType::NONE ? 1 : 0;
    block.label = cc.newLabel();
    block.stack.freeze();
    //block.stack.push(cc.newInt32());
  }
  ///////////////

  blockMngr.pushBlock();
  auto& block = blockMngr.getActive();
  block.outArity = retType != WasmValueType::NONE ? 1 : 0;
  block.label = cc.newLabel();
  FuncSignature sig;
  sig.setRet(WasmTtoJitT(retType));
  for (auto param : params) {
    sig.addArg(WasmTtoJitT(param));
  }
  cc.bind(fnLabels[index]);
  auto funcNode = cc.addFunc(sig);
  funcNode->frame().setPreservedFP();
  block.locals.reserve(params.size());
  for (u32 i = 0; i < params.size(); i++) {
    block.locals.push_back(createReg(params[i]));
    funcNode->setArg(i, block.locals.back());
  }
}

void WasmCompiler::Return() {
  LOG_DEBUG_CC("Return, type: {}", toString(returnType));
  if (returnType != WasmValueType::NONE) {
    auto& block = blockMngr.getActive();
    auto reg = block.stack.pop();
    cc.ret(reg);
  } else {
    cc.ret();
  }

}

void WasmCompiler::EndFunction() {
  LOG_DEBUG_CC("EndFunction, nest: {}", blockMngr.size());
  EndBlock();
  assert(blockMngr.size() == 1 && "BlockManager not empty at end of function");
  if (returnType != WasmValueType::NONE) {
    auto& block = blockMngr.getActive();
    auto reg = block.stack.pop();
    cc.ret(reg);
  } else {
    cc.ret();
  }
  cc.endFunc();
  blockMngr.clear();
}



void WasmCompiler::AddLocals(std::span<WasmValueType> localTypes) {
  LOG_DEBUG("AddLocals: {}", localTypes.size());
  auto &locals = blockMngr.getActive().locals;
  for (auto type : localTypes) {
    locals.push_back(createReg(type));
  }
}

void WasmCompiler::AddGlobals(std::span<WasmGlobal> _globals,
                              std::span<value_t> values) {
  LOG_DEBUG_CC("AddGlobals: {}", _globals.size());
  for (u32 i = 0; i < _globals.size(); i++) {
    auto &global = _globals[i];
    auto &value = values[i];
    if (!global.isMutable) {
      globals.push_back(cc.newInt32Const(globalPool, std::get<i32>(value)));
    } else {
      globals.push_back(cc.newInt32Const(globalPool, std::get<i32>(value)));
    }

  }

}


void WasmCompiler::StartBlock(u32 in, u32 out) {
  LOG_DEBUG_CC("StartBlock: in: {}, out: {}", in, out);
  blockMngr.pushBlock();
  auto &block = blockMngr.getActive();
  auto &parent = blockMngr.getParent();
  block.label = cc.newLabel();
  block.locals = parent.locals;
  block.outArity = out;
  block.stack.initFrom(parent.stack, in);
  parent.stack.freeze();
  parent.stack.push(cc.newInt32());
}

void WasmCompiler::EndBlock() {
  LOG_DEBUG_CC("EndBlock", 0);
  BlockState &block = blockMngr.getActive();
  assert(blockMngr.size() >= 1 && "EndBlock called on empty block stack");
  BlockState &parent = blockMngr.getParent();
  parent.stack.transferFrom(cc, block.stack, block.outArity);
  parent.stack.unfreeze();
  cc.bind(block.label);
  blockMngr.popBlock();
}

/*
 * br_if specifies a block as branch target relative to the current block
 * so depth 0 means jump to the end of the current block (or to the start for loops)
 * 1 -> one level outwards and so on
 *
 * the stack state (top x elements depending on outArity) after generating the branch
 * has to be transferred to the block at depth + 1
 */
void WasmCompiler::BrIf(i32 depth) {
  LOG_DEBUG_CC("BrIf: {}", depth);
  Label noBreak = cc.newLabel();
  auto& currentBlock = blockMngr.getActive();
  auto reg = currentBlock.stack.pop();
  cc.test(reg, reg);

  cc.jz(noBreak);


 // currentBlock.stack.deduplicate(cc);
  auto& targetBlock = blockMngr.getRelative(depth);
  auto& transferBlock = blockMngr.getRelative(depth + 1);
  assert(targetBlock.label.isValid() && "Invalid target block label");
  transferBlock.stack.transferFrom(cc, currentBlock.stack, currentBlock.outArity);
  cc.jmp(targetBlock.label);
  cc.bind(noBreak);
}

void WasmCompiler::BrIfnz(i32 depth) {

  assert (false && "Not implemented");

}

void WasmCompiler::Br(i32 depth) {
  LOG_DEBUG_CC("Br: {}", depth);
  BlockState &block = blockMngr.getRelative(depth - 1);
  cc.jmp(block.label);
}

void WasmCompiler::I32Const(i32 value) {
  LOG_DEBUG_CC("I32Const: {}", value);
  auto& block = blockMngr.getActive();
  auto reg = createReg(WasmValueType::I32);
  cc.mov(reg, value);
  block.stack.push(reg);
}

void WasmCompiler::_I32Add(x86::Gp dst, x86::Gp lhs, x86::Gp rhs) {
  if (dst != lhs) {
    cc.lea(dst, x86::ptr(lhs, rhs, 0, 0));
  } else {
    cc.add(dst, rhs);
  }
}

void WasmCompiler::Add() {
  LOG_DEBUG_CC("Add", 0);
  auto& block = blockMngr.getActive();
  x86::Gp dst = createReg(WasmValueType::I32);
  x86::Gp rhs = block.stack.pop();
  x86::Gp lhs = block.stack.pop();
  _I32Add(dst, lhs, rhs);
  block.stack.push(dst);
}

void WasmCompiler::I32Load(i64 base) {
  LOG_DEBUG_CC("I32Load: {}", base);
  auto& block = blockMngr.getActive();
  auto result = createReg(WasmValueType::I32);
  auto offset = block.stack.pop();
  auto baseReg = createReg(WasmValueType::I64);
  cc.mov(baseReg, base);
  cc.mov(result, x86::ptr_32(baseReg, offset));
  block.stack.push(result);
}

void WasmCompiler::I32Store(i64 base) {
  LOG_DEBUG_CC("I32Store: {}", base);
  auto& block = blockMngr.getActive();
  auto value = block.stack.pop();
  auto offset = block.stack.pop();
  auto baseReg = createReg(WasmValueType::I64);
  cc.mov(baseReg, base);
  cc.mov(x86::ptr_32(baseReg, offset), value);
}

void WasmCompiler::LocalGet(u32 index) {
  LOG_DEBUG_CC("LocalGet: {}", index);
  auto &block = blockMngr.getActive();
  block.stack.push(block.locals[index]);
}
void WasmCompiler::GlobalGet(u32 index) {
  LOG_DEBUG_CC("GlobalGet: {}", index);
  auto &block = blockMngr.getActive();
  auto reg = createReg(WasmValueType::I32);
  cc.mov(reg, globals[index]);
  block.stack.push(reg);
}

void WasmCompiler::LocalSet(u32 index) {
  LOG_DEBUG_CC("LocalSet: {}", index);
  auto &block = blockMngr.getActive();
  auto reg = block.stack.pop();
  cc.mov(block.locals[index], reg);
}

void WasmCompiler::Gts() {
  LOG_DEBUG_CC("Gts", 0);
  auto& block = blockMngr.getActive();
  x86::Gp rhs = block.stack.pop();
  x86::Gp lhs = block.stack.pop();
  cc.cmp(lhs, rhs);
  x86::Gp dst = cc.newInt32();
  cc.setg(x86::cl);
  cc.movzx(dst, x86::cl);
  block.stack.push(dst);
}

void WasmCompiler::finalize() {
  LOG_DEBUG_CC("finalize", 0);
  cc.finalize();
  Error err = runtime.add(&entry, &code);
  if (err) {
    printf("Error: %s\n", DebugUtils::errorAsString(err));
  }
}

void WasmCompiler::dumpAsm() { std::cout << logger.data() << std::endl; }
void WasmCompiler::dumpTrace() {
  std::cout << dbg.str() << std::endl;
}

} // namespace wasmjit
