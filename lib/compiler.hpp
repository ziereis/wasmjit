#pragma once

#include "asmjit/asmjit.h"
#include "asmjit/core/constpool.h"
#include "asmjit/x86/x86opcode_p.h"
#include "asmjit/x86/x86operand.h"
#include "lib/tz-utils.hpp"
#include "parser.hpp"
#include <span>
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

  bool empty() const;
  std::size_t size();
  void deduplicate(x86::Compiler &cc);
  void initFrom(OperandStack &other, u32 in);
  void transferFrom(x86::Compiler &cc, OperandStack &other, u64 count);
  void freeze();
  void unfreeze();

private:
  std::vector<x86::Gp> stack;
  i64 frozenIdx;
};

struct BlockState {
  Label label;
  OperandStack stack;
  std::vector<x86::Gp> locals;
  u32 outArity;
};

class BlockManager {
public:
  bool stackEmpty() const;
  void pushBlock();
  void popBlock();
  BlockState &getActive();
  BlockState &getParent();
  BlockState &getRelative(i32 depth);
  BlockState &getByDepth(i32 depth);
  void initFromRelative(i32 depth);

  bool empty() const;
  std::size_t size() const;
  void clear();

private:
  i32 activeBlock = -1;
  std::vector<BlockState> blocks;
};

class WasmCompiler {
public:
  WasmCompiler(u32 funcCount);

  void StartFunction(u32 index, WasmValueType retType,
                     std::span<WasmValueType> params);
  void EndFunction();
  void AddLocals(std::span<WasmValueType> types);
  void AddGlobals(std::span<WasmGlobal> globals, std::span<value_t> values);
  void EndBlock();
  void Return();

  void LocalGet(u32 index);
  void GlobalGet(u32 index);
  void LocalSet(u32 index);
  void I32Const(i32 value);
  void Call(u32 fnIdx, WasmValueType retType, std::span<WasmValueType> params);

  void StartBlock(u32 in, u32 out);

  void I32Load(i64 addr);
  void I32Store(i64 addr);

  void BrIf(i32 depth);
  void BrIfnz(i32 depth);
  void Br(i32 depth);

  void Add();
  void Gts();
  void Eq();

  void finalize();
  template <typename T> T getEntry(u32 fnIdx);
  void dump();

private:
  void _I32Add(x86::Gp dst, x86::Gp lhs, x86::Gp rhs);
  x86::Gp createReg(WasmValueType type);
  WasmValueType returnType;


  std::vector<x86::Mem> globals;

  ConstPoolScope globalPool;

  JitRuntime runtime;
  CodeHolder code;
  x86::Compiler cc;
  StringLogger logger;
  u8 *entry;

  std::vector<Label> fnLabels;
  BlockManager blockMngr;
};

template <class T> T WasmCompiler::getEntry(u32 fnIdx) {
  assert(fnLabels[fnIdx].isValid());
  auto offset = code.labelOffsetFromBase(fnLabels[fnIdx]);
  return reinterpret_cast<T>(entry + offset);
}

} // namespace wasmjit
