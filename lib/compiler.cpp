#include <cstdio>
#include <span>

#include "compiler.hpp"
#include "parser.hpp"

using namespace asmjit;

namespace wasmjit {

OperandStack::OperandStack() {}

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

void OperandStack::merge(OperandStack &other, u32 count) {
  stack.insert(stack.end(), other.stack.end() - count, other.stack.end());
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

WasmCompiler::WasmCompiler(u32 funcCount) {
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
  blockMngr.pushBlock();
  auto& block = blockMngr.getActive();
  block.outArity = 1;
  FuncSignature sig;
  sig.setRet(WasmTtoJitT(retType));
  for (auto param : params) {
    sig.addArg(WasmTtoJitT(param));
  }

  auto funcNode = cc.addFunc(sig);
  funcNode->frame().setPreservedFP();
  cc.bind(fnLabels[index]);
  block.locals.reserve(params.size());
  for (u32 i = 0; i < params.size(); i++) {
    block.locals.push_back(createReg(params[i]));
    funcNode->setArg(i, block.locals.back());
  }
}

void WasmCompiler::EndFunction() {
  cc.endFunc();
  assert(blockMngr.stackEmpty() && "OperandStack not empty at end of function");
  assert(blockMngr.size() == 1 && "BlockManager not empty at end of function");
  blockMngr.clear();
}

void WasmCompiler::Call(u32 index, WasmValueType retType,
                        std::span<WasmValueType> params) {
  // TODO: maybe cache the sig if its already generated
  FuncSignature calleeSig;
  calleeSig.setRet(WasmTtoJitT(retType));
  for (auto param : params) {
    calleeSig.addArg(WasmTtoJitT(param));
  }
  InvokeNode *invokeNode;
  cc.invoke(&invokeNode, fnLabels[index], calleeSig);

  auto& block = blockMngr.getActive();
  for (u32 i = 0; i < params.size(); i++) {
    invokeNode->setArg(i, block.stack.pop());
  }
  x86::Gp retReg = createReg(retType);

  invokeNode->setRet(0, retReg);
  block.stack.push(retReg);
}

void WasmCompiler::AddLocals(std::span<WasmValueType> localTypes) {
  auto &locals = blockMngr.getActive().locals;
  for (auto type : localTypes) {
    locals.push_back(createReg(type));
  }
}

void WasmCompiler::Return(WasmValueType type) {
  auto& block = blockMngr.getActive();
  if (type != WasmValueType::NONE) {
    cc.ret(block.stack.pop());
  } else {
    cc.ret();
  }
}

void WasmCompiler::StartBlock(u32 in, u32 out) {
  blockMngr.pushBlock();
  auto &block = blockMngr.getActive();
  auto &parent = blockMngr.getParent();
  block.label = cc.newLabel();
  block.locals = parent.locals;
  block.outArity = out;
  block.stack.initFrom(parent.stack, in);
}

void WasmCompiler::EndBlock() {
  BlockState &block = blockMngr.getActive();
  BlockState &parent = blockMngr.getParent();
  parent.stack.merge(block.stack, block.outArity);
  cc.bind(block.label);
  blockMngr.popBlock();
}

void WasmCompiler::BrIfz(i32 depth) {
  auto& block = blockMngr.getActive();
  auto reg = block.stack.pop();
  cc.test(reg, reg);

  BlockState &parent = blockMngr.getRelative(depth - 1);
  cc.jz(parent.label);
}

void WasmCompiler::BrIfnz(i32 depth) {
  auto& block = blockMngr.getActive();
  auto reg = block.stack.pop();
  cc.test(reg, reg);

  BlockState &parent = blockMngr.getRelative(depth - 1);
  cc.jnz(block.label);
}

void WasmCompiler::Br(i32 depth) {
  BlockState &block = blockMngr.getRelative(depth - 1);
  cc.jmp(block.label);
}

void WasmCompiler::I32Const(i32 value) {
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
  auto& block = blockMngr.getActive();
  x86::Gp dst = createReg(WasmValueType::I32);
  x86::Gp lhs = block.stack.pop();
  x86::Gp rhs = block.stack.pop();
  _I32Add(dst, lhs, rhs);
  block.stack.push(dst);
}

void WasmCompiler::LocalGet(u32 index) {
  auto &block = blockMngr.getActive();
  block.stack.push(block.locals[index]);
}

void WasmCompiler::LocalSet(u32 index) {
  auto &block = blockMngr.getActive();
  auto reg = block.stack.pop();
  cc.mov(block.locals[index], reg);
}

void WasmCompiler::Gts() {
  auto& block = blockMngr.getActive();
  x86::Gp dst = cc.newInt32();
  x86::Gp lhs = block.stack.pop();
  x86::Gp rhs = block.stack.pop();
  cc.cmp(lhs, rhs);
  cc.setg(dst.r8());
  cc.movzx(dst, dst.r8());
  block.stack.push(dst);
}

void WasmCompiler::finalize() {
  cc.finalize();
  Error err = runtime.add(&entry, &code);
  if (err) {
    printf("Error: %s\n", DebugUtils::errorAsString(err));
  }
}

void WasmCompiler::dump() { std::cout << logger.data() << std::endl; }

} // namespace wasmjit
