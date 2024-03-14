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
#include <functional>
#include <iostream>
#include <vector>


using namespace wasmjit;

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
  cc.Call(0, WasmValueType::I32, params);
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
  cc.Call(0, WasmValueType::I32, params);
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

TEST_CASE("greater than") {
  WasmCompiler cc(1);
  std::vector<WasmValueType> params = {WasmValueType::I32, WasmValueType::I32};
  cc.StartFunction(0, WasmValueType::I32, params);
  cc.LocalGet(0);
  cc.LocalGet(1);
  cc.Gts();
  cc.EndFunction();
  cc.finalize();
  cc.dump();
  auto fn = cc.getEntry<IntIntIntFn>(0);
  std::cout << fn(1, 2) << std::endl;
  REQUIRE_EQ(fn(1, 2), 0);
  REQUIRE_EQ(fn(2, 1), 1);

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
  cc.BrIfz(0);
  cc.I32Const(100);
  cc.Add();
  cc.EndBlock();
  cc.EndFunction();
  cc.finalize();
  auto fn = cc.getEntry<IntIntFn>(0);
  REQUIRE_EQ(fn(0), 0);
  REQUIRE_EQ(fn(42), 142);
}

TEST_CASE("block br_if end of function") {
  WasmCompiler cc(1);
  std::vector<WasmValueType> params = {WasmValueType::I32};
  cc.StartFunction(0, WasmValueType::I32, params);
  cc.StartBlock({}, 1);
  cc.LocalGet(0);
  cc.LocalGet(0);
  cc.BrIfz(1);
  cc.I32Const(100);
  cc.Add();
  cc.EndBlock();
  cc.I32Const(1000);
  cc.Add();
  cc.EndFunction();
  cc.finalize();
  auto fn = cc.getEntry<IntIntFn>(0);
  REQUIRE_EQ(fn(0), 0);
  REQUIRE_EQ(fn(42), 1142);
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
