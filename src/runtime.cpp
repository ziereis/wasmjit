#include "compiler.hpp"
#include "parser.hpp"
#include "tz-utils.hpp"
#include "wasm-types.hpp"
#include <format>
#include <new>
#include <string_view>
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

  u32 numFuncs = reader.readIntLeb<u32>();
  LOG_DEBUG("numFuncs: {}", numFuncs);

  std::vector<WasmValueType> localTypes;

  WasmCompiler compiler(numFuncs);

  for (u32 i = 0; i < numFuncs; i++) {
    localTypes.clear();

    u32 typeIdx = wasmModule.functionSection.functions[i];
    auto &signature = wasmModule.getPrototype(i);
    compiler.StartFunction(typeIdx, signature.returnType, signature.paramTypes);

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
          compiler.Return(signature.returnType);
          goto end;
        } else {
          compiler.EndBlock();
          depth--;
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
      case WasmOpcode::IF: {
        // If else statements are a shorthand for block/block/br_if
        auto type = static_cast<WasmValueType>(reader.read<u8>());
        depth += 2;
        compiler.StartBlock(type);
        compiler.StartBlock(type);
        compiler.BrIfnz(depth);
        break;
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
      default:
        throw std::runtime_error("Invalid opcode");
      }
    }
  end:
    compiler.EndFunction();
  }
  auto entry = compiler.finalize();
  compiler.dump();
  return entry();
}

int main() { return runWasm("../wasm_tests/wasm_adder.wasm"); }

} // namespace wasmjit
