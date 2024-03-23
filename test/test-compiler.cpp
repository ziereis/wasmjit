//
// Created by ziereis on 10.03.24.
//

#include "asmjit/core/codeholder.h"
#include "asmjit/core/func.h"
#include "asmjit/core/jitruntime.h"
#include "asmjit/core/type.h"
#include "asmjit/x86/x86compiler.h"
#include "doctest.h"
#include "lib/compiler.hpp"
#include "lib/parser.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <iterator>
#include <vector>
#include "src/runtime.hpp"

using namespace wasmjit;

using voidvoidFn = void (*)();
using IntVoidFn = int (*)();
using IntIntFn = int (*)(int);
using IntIntIntFn = int (*)(int, int);

TEST_CASE("asmjit") {
  JitRuntime rt;
  CodeHolder code;
  StringLogger logger;
  code.setLogger(&logger);

  code.init(rt.environment(), rt.cpuFeatures());

  x86::Compiler c(&code);

  FuncSignature sig;
  sig.setRet(TypeId::kInt32);
  sig.addArg(TypeId::kInt32);
  sig.addArg(TypeId::kInt32);
  auto fn = c.addFunc(sig);
  x86::Gp left = c.newInt32("left");
  x86::Gp right = c.newInt32("right");

  fn->setArg(0, left);
  fn->setArg(1, right);
  c.cmp(left, right);

  x86::Gp result = c.newInt32("result");

  c.setg(x86::cl);
  c.movzx(result, x86::cl);

  c.ret(result);

  c.endFunc();

  c.finalize();

  IntIntIntFn fnPtr;

  rt.add(&fnPtr, &code);

  std::cout << logger.data() << std::endl;
}

TEST_CASE("return 1") {
  WasmCompiler cc(1);
  cc.StartFunction(0, WasmValueType::I32, {});
  cc.I32Const(1);
  cc.EndFunction();
  cc.finalize();
  auto fn = cc.getEntry<IntVoidFn>(0);
  std::cout << fn() << std::endl;
  REQUIRE_EQ(fn(), 1);
}

TEST_CASE("return param") {
  WasmCompiler cc(1);
  std::vector<WasmValueType> params = {WasmValueType::I32};
  cc.StartFunction(0, WasmValueType::I32, params);
  cc.LocalGet(0);
  cc.EndFunction();
  cc.finalize();
  auto fn = cc.getEntry<IntIntFn>(0);
  REQUIRE_EQ(fn(42), 42);
}

TEST_CASE("basic add") {
  WasmCompiler cc(1);
  std::vector<WasmValueType> params = {WasmValueType::I32, WasmValueType::I32};
  cc.StartFunction(0, WasmValueType::I32, params);
  cc.LocalGet(0);
  cc.LocalGet(1);
  cc.Add();
  cc.EndFunction();
  cc.finalize();
  auto fn = cc.getEntry<IntIntIntFn>(0);
  REQUIRE_EQ(fn(1, 2), 3);
}

TEST_CASE("empty fn") {
  WasmCompiler cc(1);
  std::vector<WasmValueType> params = {};
  cc.StartFunction(0, WasmValueType::NONE, params);
  cc.EndFunction();
  cc.finalize();
  cc.dumpAsm();
  auto fn = cc.getEntry<voidvoidFn>(0);
  REQUIRE_NOTHROW(fn());
}

TEST_CASE("add and add const") {
  WasmCompiler cc(1);
  std::vector<WasmValueType> params = {WasmValueType::I32, WasmValueType::I32};
  cc.StartFunction(0, WasmValueType::I32, params);
  cc.LocalGet(0);
  cc.I32Const(1);
  cc.Add();
  cc.LocalGet(1);
  cc.Add();
  cc.EndFunction();
  cc.finalize();
  auto fn = cc.getEntry<IntIntIntFn>(0);
  REQUIRE_EQ(fn(1, 2), 4);
}

TEST_CASE("decrement") {
  WasmCompiler cc(1);
  std::vector<WasmValueType> params = {WasmValueType::I32};
  cc.StartFunction(0, WasmValueType::I32, params);
  cc.LocalGet(0);
  cc.I32Const(-1);
  cc.Add();
  cc.EndFunction();
  cc.finalize();
  auto fn = cc.getEntry<IntIntFn>(0);
  REQUIRE_EQ(fn(42), 41);
}

TEST_CASE("function call, callee generated first") {
  WasmCompiler cc(2);
  std::vector<WasmValueType> params = {WasmValueType::I32};
  // increment one
  cc.StartFunction(0, WasmValueType::I32, params);
  cc.LocalGet(0);
  cc.I32Const(1);
  cc.Add();
  cc.EndFunction();
  // call increment one
  cc.StartFunction(1, WasmValueType::I32, {});
  cc.I32Const(41);
  cc.Call(u32{0}, WasmValueType::I32, params);
  cc.EndFunction();
  cc.finalize();
  auto fn = cc.getEntry<IntVoidFn>(1);
  REQUIRE_EQ(fn(), 42);
}

TEST_CASE("function call, caller generated first") {
  WasmCompiler cc(2);
  std::vector<WasmValueType> params = {WasmValueType::I32};

  // call increment one
  cc.StartFunction(1, WasmValueType::I32, {});
  cc.I32Const(41);
  cc.Call(u32{0}, WasmValueType::I32, params);
  cc.EndFunction();
  // increment one
  cc.StartFunction(0, WasmValueType::I32, params);
  cc.LocalGet(0);
  cc.I32Const(1);
  cc.Add();
  cc.EndFunction();
  cc.finalize();
  auto fn = cc.getEntry<IntVoidFn>(1);
  REQUIRE_EQ(fn(), 42);
}

int add(int a, int b) { return a + b; }

TEST_CASE("call external") {
  WasmCompiler cc(1);
  std::vector<WasmValueType> params = {WasmValueType::I32, WasmValueType::I32};
  cc.StartFunction(0, WasmValueType::I32, params);
  cc.LocalGet(0);
  cc.LocalGet(1);
  cc.Call(reinterpret_cast<uintptr_t>(&add), WasmValueType::I32, params);
  cc.EndFunction();
  cc.finalize();
  cc.dumpAsm();
  cc.dumpTrace();
  auto fn = cc.getEntry<IntIntIntFn>(0);
  REQUIRE_EQ(fn(1, 2), 3);
}

TEST_CASE("greater than") {
  WasmCompiler cc(1);
  std::vector<WasmValueType> params = {WasmValueType::I32, WasmValueType::I32};
  cc.StartFunction(0, WasmValueType::I32, params);
  cc.LocalGet(0);
  cc.LocalGet(1);
  cc.Gts();
  cc.EndFunction();
  cc.finalize();
  auto fn = cc.getEntry<IntIntIntFn>(0);
  std::cout << fn(1, 2) << std::endl;
  REQUIRE_EQ(fn(1, 2), 0);
  REQUIRE_EQ(fn(2, 1), 1);
}

TEST_CASE("load") {
  WasmCompiler cc(1);
  u32 *mem = static_cast<u32 *>(malloc(sizeof(u32)));
  *mem = 1337;
  std::vector<WasmValueType> params = {};
  cc.StartFunction(0, WasmValueType::I32, params);
  cc.I32Const(0); // offset
  cc.I32Load(reinterpret_cast<uintptr_t>(mem));
  cc.EndFunction();
  cc.finalize();
  cc.dumpAsm();
  auto fn = cc.getEntry<IntVoidFn>(0);
  auto res = fn();
  REQUIRE_EQ(res, 1337);
}

TEST_CASE("load at offset") {
  WasmCompiler cc(1);
  u32 *mem = static_cast<u32 *>(malloc(sizeof(u32) * 4));
  memset(mem, 0, sizeof(u32) * 4);
  mem[3] = 1337;
  std::vector<WasmValueType> params = {};
  cc.StartFunction(0, WasmValueType::I32, params);
  cc.I32Const(3 * sizeof(u32)); // offset
  cc.I32Load(reinterpret_cast<uintptr_t>(mem));
  cc.EndFunction();
  cc.finalize();
  cc.dumpAsm();
  auto fn = cc.getEntry<IntVoidFn>(0);
  auto res = fn();
  REQUIRE_EQ(res, 1337);
}

TEST_CASE("store") {
  WasmCompiler cc(1);
  u32 *mem = static_cast<u32 *>(malloc(sizeof(u32)));
  std::vector<WasmValueType> params = {};
  cc.StartFunction(0, WasmValueType::NONE, params);
  cc.I32Const(0);
  cc.I32Const(1337);
  cc.I32Store(reinterpret_cast<uintptr_t>(mem));
  cc.EndFunction();
  cc.finalize();
  cc.dumpAsm();
  auto fn = cc.getEntry<voidvoidFn>(0);
  fn();
  REQUIRE_EQ(*mem, 1337);
}

TEST_CASE("store at offset") {
  WasmCompiler cc(1);
  u32 *mem = static_cast<u32 *>(malloc(sizeof(u32) * 4));
  memset(mem, 0, sizeof(u32) * 4);
  std::vector<WasmValueType> params = {};
  cc.StartFunction(0, WasmValueType::NONE, params);
  cc.I32Const(3 * sizeof(u32));
  cc.I32Const(1337);
  cc.I32Store(reinterpret_cast<uintptr_t>(mem));
  cc.EndFunction();
  cc.finalize();
  cc.dumpAsm();
  auto fn = cc.getEntry<voidvoidFn>(0);
  fn();
  REQUIRE_EQ(mem[3], 1337);
}

TEST_CASE("block") {
  WasmCompiler cc(1);
  std::vector<WasmValueType> params = {WasmValueType::I32};
  cc.StartFunction(0, WasmValueType::I32, params);
  cc.StartBlock({}, 1);
  cc.LocalGet(0);
  cc.EndBlock();
  cc.EndFunction();
  cc.finalize();
  auto fn = cc.getEntry<IntIntFn>(0);
  REQUIRE_EQ(fn(42), 42);
}

TEST_CASE("block with input") {
  WasmCompiler cc(1);
  std::vector<WasmValueType> params = {WasmValueType::I32};
  cc.StartFunction(0, WasmValueType::I32, params);
  cc.LocalGet(0);
  cc.StartBlock(1, 1);
  cc.LocalGet(0);
  cc.Add();
  cc.EndBlock();
  cc.EndFunction();
  cc.finalize();
  auto fn = cc.getEntry<IntIntFn>(0);
  REQUIRE_EQ(fn(42), 84);
}

// TEST_CASE("global get non mutable") {
//   WasmCompiler cc(1);
//   WasmGlobal global;
//   global.type = WasmValueType::I32;
//   global.isMutable = false;
//   std::vector<WasmGlobal> globals = {global};
//   std::vector<value_t> values = {42};
//   cc.AddGlobals(globals, values);

//   std::vector<WasmValueType> params = {};
//   cc.StartFunction(0, WasmValueType::I32, params);
//   cc.GlobalGet(0);
//   cc.EndFunction();
//   cc.finalize();
//   cc.dumpAsm();
//   auto fn = cc.getEntry<IntVoidFn>(0);
//   REQUIRE_EQ(fn(), 42);
// }


TEST_CASE("block br_if") {
  WasmCompiler cc(1);
  std::vector<WasmValueType> params = {WasmValueType::I32};
  cc.StartFunction(0, WasmValueType::I32, params);
  /*
   * major problem here:
   * both operand stack slots reference the same register
   * Without deduplication:
   * If the branch is taken it moves uninitialized memory into eax
   * ////////////////////////////
   * test edi, edi
   * jz L3
   * mov eax, 100
   * lea eax, [eax+edi]
   * mov dword ptr [rsp], eax
   * L3:
   * mov eax, dword ptr [rsp]
   * ////////////////////
   * so they shadow each other so i have to do deduplication by
   * moving one stack slot into a new register
   * (probably both stack slots since the local can be used after the block)
   * and then also do a control merge at the end of the block
   * After dedup and merge:
   * //////////////////
   * test edi, edi
   * jz L3
   * mov eax, 100
   * lea eax, [eax+edi]
   * mov edi, eax
   * L3:
   * mov eax, edi
   * //////////////////
   */
  cc.StartBlock({}, 1);
  cc.LocalGet(0);
  cc.LocalGet(0);
  cc.BrIf(0);
  cc.I32Const(100);
  cc.Add();
  cc.EndBlock();
  cc.EndFunction();
  cc.finalize();
  auto fn = cc.getEntry<IntIntFn>(0);
  REQUIRE_EQ(fn(42), 42);
  REQUIRE_EQ(fn(0), 100);
}

TEST_CASE("block br_if end of function") {
  WasmCompiler cc(1);
  std::vector<WasmValueType> params = {WasmValueType::I32};
  cc.StartFunction(0, WasmValueType::I32, params);
  cc.StartBlock({}, 1);
  cc.LocalGet(0);
  cc.LocalGet(0);
  cc.BrIf(1);
  cc.I32Const(100);
  cc.Add();
  cc.EndBlock();
  cc.I32Const(1000);
  cc.Add();
  cc.EndFunction();
  cc.finalize();
  auto fn = cc.getEntry<IntIntFn>(0);
  REQUIRE_EQ(fn(42), 42);
  REQUIRE_EQ(fn(0), 1100);
}

// TEST_CASE("nested block") {
//   WasmCompiler cc(1);
//   std::vector<WasmValueType> params = {WasmValueType::I32};
//   cc.StartFunction(0, WasmValueType::I32, params);
//   cc.StartBlock({}, 1);
//   cc.LocalGet(0);
//   cc.LocalGet(0);
//   cc.
// }





// Trace:
// CC ->AddGlobals: 1
// CC ->StartFunction Index: 1, retType: void, params: 0
// CC ->EndFunction, nest: 2
// CC ->EndBlock
// CC ->StartFunction Index: 2, retType: void, params: 0
// CC ->StartBlock: in: 0, out: 0
// CC ->StartBlock: in: 0, out: 0
// CC ->I32Const: 0
// CC ->I32Load: 127623645365248
// CC ->BrIf: 0
// CC ->I32Const: 0
// CC ->I32Const: 1
// CC ->I32Store: 127623645365248
// CC ->LocalSet: 0
// CC ->LocalGet: 0
// CC ->BrIf: 1
// CC ->Return, type: void
// CC ->EndBlock
// CC ->EndBlock
// CC ->LocalGet: 0
// CC ->EndFunction, nest: 2
// CC ->EndBlock
// CC ->StartFunction Index: 3, retType: i32, params: 0
// CC ->I32Const: 1
// CC ->EndFunction, nest: 2
// CC ->EndBlock
// CC ->StartFunction Index: 4, retType: void, params: 1
// CC ->LocalGet: 0
// CC ->EndFunction, nest: 2
// CC ->EndBlock
// CC ->StartFunction Index: 5, retType: void, params: 0
// CC ->EndFunction, nest: 2
// CC ->EndBlock
// CC ->finalize


TEST_CASE("add locals") {
  std::vector<WasmValueType> params = {};
  std::vector<WasmValueType> locals = {WasmValueType::I32};

  WasmCompiler cc(1);
  cc.StartFunction(0, WasmValueType::I32, params);
  cc.AddLocals(locals);
  cc.I32Const(42);
  cc.LocalSet(0);
  cc.LocalGet(0);
  cc.EndFunction();
  cc.finalize();
  cc.dumpAsm();
  auto fn = cc.getEntry<IntVoidFn>(0);
  auto res = fn();
  REQUIRE_EQ(res, 42);
}

TEST_CASE("trace example 1") {
  std::vector<std::vector<WasmValueType>> params = {{WasmValueType::I32}, {}, {}};
  std::vector<WasmValueType> rets = {WasmValueType::NONE, WasmValueType::NONE, WasmValueType::I32};
  LinearMemory mem;
  mem.init(2);

  WasmCompiler cc(7);

  std::vector<WasmValueType> locals = {WasmValueType::I32};
  cc.StartFunction(1, rets[1], params[1]);
  cc.EndFunction();
  cc.StartFunction(2, rets[1], params[1]);
  cc.AddLocals(locals);
  cc.StartBlock(0, 0);
  cc.StartBlock(0, 0);
  cc.I32Const(0);
  cc.I32Load(reinterpret_cast<uintptr_t>(mem.mem + 1024));
  cc.BrIf(0);
  cc.I32Const(0);
  cc.I32Const(1);
  cc.I32Store(reinterpret_cast<uintptr_t>(mem.mem + 1024));
  cc.Call(u32(1), rets[1], params[1]);
  cc.Call(u32(3), rets[2], params[2]);
  cc.LocalSet(0);
  // cc.Call(u32(6), rets[1], params[1]);
  cc.LocalGet(0);
  cc.BrIf(1);
  // cc.Return();
  cc.EndBlock();
  cc.EndBlock();
  // cc.LocalGet(0);
  // cc.Call(u32(4), rets[0], params[0]);
  cc.EndFunction();
  cc.finalize();
  cc.dumpAsm();
  cc.dumpTrace();
}
