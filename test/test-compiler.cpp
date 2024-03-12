//
// Created by ziereis on 10.03.24.
//

#include "doctest.h"
#include "lib/compiler.hpp"
#include <iostream>


using namespace wasmjit;

using IntVoidFn = int (*)();
using IntIntFn = int (*)(int);
using IntIntIntFn = int (*)(int, int);




TEST_CASE("return 1") {
  WasmCompiler cc(1);
  cc.StartFunction(0, WasmValueType::I32, {});
  cc.I32Const(1);
  cc.Return(WasmValueType::I32);
  cc.EndFunction();
  cc.finalize();
  cc.dump();
  auto fn = cc.getEntry<IntVoidFn>(0);
  std::cout << fn() << std::endl;
  REQUIRE_EQ(fn(), 1);
}

TEST_CASE("return param") {
  WasmCompiler cc(1);
  std::vector<WasmValueType> params = {WasmValueType::I32};
  cc.StartFunction(0, WasmValueType::I32, params);
  cc.LocalGet(0);
  cc.Return(WasmValueType::I32);
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
  cc.Return(WasmValueType::I32);
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
  cc.Return(WasmValueType::I32);
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
  cc.Return(WasmValueType::I32);
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
  cc.Return(WasmValueType::I32);
  cc.EndFunction();
  // call increment one
  cc.StartFunction(1, WasmValueType::I32, {});
  cc.I32Const(41);
  cc.Call(0, WasmValueType::I32, params);
  cc.Return(WasmValueType::I32);
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
  cc.Return(WasmValueType::I32);
  cc.EndFunction();
  // increment one
  cc.StartFunction(0, WasmValueType::I32, params);
  cc.LocalGet(0);
  cc.I32Const(1);
  cc.Add();
  cc.Return(WasmValueType::I32);
  cc.EndFunction();
  cc.finalize();
  auto fn = cc.getEntry<IntVoidFn>(1);
  REQUIRE_EQ(fn(), 42);
}

TEST_CASE("block") {
  WasmCompiler cc(1);
  std::vector<WasmValueType> params = {WasmValueType::I32};
  cc.StartFunction(0, WasmValueType::I32, params);
  cc.StartBlock({}, 1);
  cc.LocalGet(0);
  cc.EndBlock();
  cc.Return(WasmValueType::I32);
  cc.EndFunction();
  cc.finalize();
  cc.dump();
  auto fn = cc.getEntry<IntIntFn>(0);
  std::cout << fn(42) << std::endl;
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
  cc.Return(WasmValueType::I32);
  cc.EndFunction();
  cc.finalize();
  auto fn = cc.getEntry<IntIntFn>(0);
  std::cout << fn(42) << std::endl;
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
   * TODO: deduplication and merge
   */
  cc.StartBlock({}, 1);
  cc.LocalGet(0);
  cc.LocalGet(0);
  cc.BrIfz(1);
  cc.I32Const(100);
  cc.Add();
  cc.EndBlock();
  cc.Return(WasmValueType::I32);
  cc.EndFunction();
  cc.finalize();
  cc.dump();
  auto fn = cc.getEntry<IntIntFn>(0);
  REQUIRE_EQ(fn(0), 0);
  REQUIRE_EQ(fn(42), 142);
}
