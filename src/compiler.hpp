#pragma once


#include "asmjit/x86/x86operand.h"
#include "parser.hpp"
#include "asmjit/asmjit.h"
#include <vector>

using namespace asmjit;


namespace wasmjit {

class WasmCompiler {
public:
  using entryFunction = int (*)(void);

  WasmCompiler();

  void StartFunction(WasmValueType retType, std::span<WasmValueType> params);
  void EndFunction();
  void AddLocals(std::span<WasmValueType> types);
  void EndBlock();
  void Return(WasmValueType type);

  void LocalGet(u32 index);
  void LocalSet(u32 index);
  void I32Const(i32 value);
  void Call(u32 fnIdx);

  void BrIfz(i32 depth);
  void BrIfnz(i32 depth);
  void Br(i32 depth);
  void BindLabel(i32 depth);

  void Add();
  void Gts();



private:
  x86::Gp createReg(WasmValueType type);

  JitRuntime runtime;
  CodeHolder code;
  x86::Compiler cc;
  StringLogger logger;

  std::vector<x86::Gp> locals;
};

} // namespace wasmjit
