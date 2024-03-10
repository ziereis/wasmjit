#pragma once

#include "asmjit/asmjit.h"
#include "asmjit/x86/x86operand.h"
#include "parser.hpp"
#include <vector>

using namespace asmjit;

namespace wasmjit {

class OperandStack {
public:
  OperandStack();

  void push(x86::Gp reg);
  x86::Gp pop();
  x86::Gp &peek();

  void clear();

  bool empty();
  std::size_t size();

private:
  std::vector<x86::Gp> stack;
};

struct BlockState {
  Label label;
  OperandStack stack;
  u32 resultArity;
};

class BlockManager {
public:
  void pushBlock(u32 resultArityd);
  void popBlock();
  BlockState &getActive();
  BlockState &getRelative(i32 depth);
  void initFromRelative(i32 depth);

  bool empty() const;
  void clear();

private:
  i32 activeBlock = -1;
  std::vector<BlockState> blocks;
};

class WasmCompiler {
public:
  using entryFunction = int (*)(void);

  WasmCompiler(u32 funcCount);

  void StartFunction(u32 index, WasmValueType retType,
                     std::span<WasmValueType> params);
  void EndFunction();
  void AddLocals(std::span<WasmValueType> types);
  void EndBlock();
  void Return(WasmValueType type);

  void LocalGet(u32 index);
  void LocalSet(u32 index);
  void I32Const(i32 value);
  void Call(u32 fnIdx, WasmValueType retType, std::span<WasmValueType> params);

  void StartBlock(WasmValueType types);

  void BrIfz(i32 depth);
  void BrIfnz(i32 depth);
  void Br(i32 depth);
  void BindLabel(i32 depth);

  void Add();
  void Gts();

  entryFunction finalize();
  void dump();

private:
  void _I32Add(x86::Gp dst, x86::Gp lhs, x86::Gp rhs);
  x86::Gp createReg(WasmValueType type);

  JitRuntime runtime;
  CodeHolder code;
  x86::Compiler cc;
  StringLogger logger;

  std::vector<x86::Gp> locals;
  std::vector<Label> fnLabels;
  OperandStack ccStack;
  BlockManager blockMngr;
};

} // namespace wasmjit
