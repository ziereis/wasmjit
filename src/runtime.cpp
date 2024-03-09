#include "parser.hpp"
#include "compiler.hpp"
#include "tz-utils.hpp"
#include "wasm-types.hpp"
#include <new>
#include <string_view>
#include <format>
#include <vector>



namespace wasmjit {

constexpr OpcodeStringTable g_wasmOpcodeStringTable;


static void parselocals(BinaryReader &reader, std::vector<WasmValueType> &locals) {
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
  WasmModlue module;
  module.parseSections(wasmFile.asSpan());
  module.dump();
  WasmCompiler compiler;
  auto code = module.codeSection.code;
  BinaryReader reader(code.data(), code.size());

  u32 numFuncs = reader.readIntLeb<u32>();
  LOG_DEBUG("numFuncs: {}", numFuncs);

  std::vector<WasmValueType> localTypes;

  for (u32 i = 0; i < numFuncs; i++) {
    localTypes.clear();

    u32 typeIdx = module.functionSection.functions[i];
    auto &signature = module.typeSection.types[typeIdx];
    compiler.StartFunction(signature.returnType, signature.paramTypes);

    u32 fnSize = reader.readIntLeb<u32>();
    LOG_DEBUG("fnSize: {}", fnSize);
    parselocals(reader, localTypes);

    compiler.AddLocals(localTypes);

    // handle all operations
    i32 depth = 1;
    while (true) {
      WasmOpcode op = static_cast<WasmOpcode>(reader.read<u8>());
      LOG_DEBUG("op: {}", g_wasmOpcodeStringTable.Get(op));
      switch (op) {
      case WasmOpcode::END: {
          compiler.EndBlock();
          depth--;
          if (depth == 0) {
            compiler.Return(signature.returnType);
            goto end;
          } else {
            compiler.Br(depth);
          }
          break;
      }
      case WasmOpcode::LOCAL_GET: {
        u32 localIdx = reader.readIntLeb<u32>();
        compiler.LocalGet(localIdx);
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
        compiler.Call(fnIdx);
        break;
      }
      case WasmOpcode::I32_ADD: {
        compiler.Add();
      }
      case WasmOpcode::I32_GT_S: {
        compiler.Gts();
      }
      case WasmOpcode::IF: {
        depth++;
        compiler.BrIfnz(depth);
      }
      case WasmOpcode::ELSE: {
        compiler.Br(depth - 1);
        compiler.BindLabel(depth);
      }
      default:
        throw std::runtime_error("Invalid opcode");
      }
    }
  end:
    compiler.EndFunction();

  }
}

} // namespace wasmjit
