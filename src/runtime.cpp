#include "lib/compiler.hpp"
#include "lib/parser.hpp"
#include "lib/tz-utils.hpp"
#include "lib/wasm-types.hpp"
#include "lib/wasi.h"
#include "runtime.hpp"
#include <format>
#include <new>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <vector>

namespace wasmjit {

constexpr OpcodeStringTable g_wasmOpcodeStringTable;

static void parselocals(BinaryReader &reader,
                        std::vector<WasmValueType> &locals) {
  u32 numLocals = reader.readIntLeb<u32>();
  for (u32 i = 0; i < numLocals; i++) {
    u32 count = reader.readIntLeb<u32>();
    WasmValueType type = static_cast<WasmValueType>(reader.read<u8>());
    for (u32 j = 0; j < count; j++) {
      locals.push_back(type);
    }
  }
}

int runWasm(std::string_view fileName) {
  auto wasmFile = MappedFile(fileName);
  wasmFile.dump();
  WasmModule wasmModule;
  wasmModule.parseSections(wasmFile.asSpan());
  wasmModule.dump();
  auto code = wasmModule.codeSection.code;
  BinaryReader reader(code.data(), code.size());
  LinearMemory memory;
  memory.init(wasmModule.memorySection.limit->minSize);

  u32 numFuncs = reader.readIntLeb<u32>();
  LOG_DEBUG("numFuncs: {}", numFuncs);

  WasmCompiler compiler(numFuncs);

  std::vector<WasmValueType> localTypes;

  std::vector<value_t> values;
  for (auto &global : wasmModule.globalSection.initExprs) {
    values.push_back(global.value);
  }
  compiler.AddGlobals(wasmModule.globalSection.globals, values);


  for (u32 i = wasmModule.functionSection.numImportedFns; i < numFuncs; i++) {
    localTypes.clear();

    u32 typeIdx = wasmModule.functionSection.functions[i];
    auto &signature = wasmModule.getPrototype(i);
    compiler.StartFunction(i, signature.returnType, signature.paramTypes);

    u32 fnSize = reader.readIntLeb<u32>();
    LOG_DEBUG("fnSize: {}", fnSize);
    parselocals(reader, localTypes);

    compiler.AddLocals(localTypes);

    // handle all operations
    i32 depth = 0;
    while (true) {
      WasmOpcode op = static_cast<WasmOpcode>(reader.read<u8>());
      LOG_DEBUG("op: {}", g_wasmOpcodeStringTable.Get(op));
      switch (op) {
      case WasmOpcode::END: {
        if (depth == 0) {
          goto end;
        } else {
          compiler.EndBlock();
          depth--;
        }
        break;
      }
      case WasmOpcode::BLOCK: {
        u8 res = reader.read<u8>();
        auto type = static_cast<WasmValueType>(res);
        LOG_DEBUG("block type: {}", toString(type));
        compiler.StartBlock(0, 0);
        depth++;
        break;
      }
      case WasmOpcode::LOCAL_GET: {
        u32 localIdx = reader.readIntLeb<u32>();
        compiler.LocalGet(localIdx);
        break;
      }
      case WasmOpcode::GLOBAL_GET: {
        u32 globalIdx = reader.readIntLeb<u32>();

        compiler.GlobalGet(globalIdx);
        break;
      }
      case WasmOpcode::BR_IF: {
        u32 offset = reader.readIntLeb<u32>();
        compiler.BrIf(offset);
        break;
      }
      case WasmOpcode::I32_LOAD: {
        u32 align = reader.readIntLeb<u32>();
        i32 offset = reader.readIntLeb<i32>();
        assert(offset == 0);
        std::ignore = align;
        compiler.I32Load(reinterpret_cast<u64>(memory.mem));
        break;
      }
      case WasmOpcode::I32_STORE: {
        u32 align = reader.readIntLeb<u32>();
        i32 offset = reader.readIntLeb<i32>();
        assert(offset == 0);
        std::ignore = align;
        compiler.I32Store(reinterpret_cast<u64>(memory.mem));
        break;
      }
      case WasmOpcode::LOCAL_SET: {
        u32 localIdx = reader.readIntLeb<u32>();
        compiler.LocalSet(localIdx);
        break;
      }
      case WasmOpcode::I32_CONST: {
        i32 value = reader.readIntLeb<i32>();
        compiler.I32Const(value);
        break;
      }
      case WasmOpcode::CALL: {
        u32 fnIdx = reader.readIntLeb<u32>();
        auto &signature = wasmModule.getPrototype(fnIdx);
        compiler.Call(fnIdx, signature.returnType, signature.paramTypes);
        break;
      }
      case WasmOpcode::I32_ADD: {
        compiler.Add();
        break;
      }
      case WasmOpcode::I32_GT_S: {
        compiler.Gts();
        break;
      }
      case WasmOpcode::RETURN: {
        compiler.Return();
        break;
      }
      case WasmOpcode::IF: {
        // If else statements are a shorthand for block/block/br_if
        auto type = static_cast<WasmValueType>(reader.read<u8>());
        depth += 2;
      }
      case WasmOpcode::ELSE: {
        // else can only exist when prev block was an if, so emit code here to
        // finish the if statement

        // decrease depth so the following branch will target the outer block
        depth--;
        compiler.Br(depth);
        // end the block after the branch so it will
        // correctly bind to the place where the else starts
        compiler.EndBlock();

        // start of the block is already emmited by the if so the else is
        // basically a no-op
        break;
      }
      case WasmOpcode::UNREACHABLE: {
        break;
      }
      default:
        throw std::runtime_error("Invalid opcode");
      }
    }
  end:
    compiler.EndFunction();
  }
  compiler.dump();
  return 0;
}


} // namespace wasmjit
