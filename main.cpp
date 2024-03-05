#include "ArenaAllocator.hpp"
#include "asmjit/core/func.h"
#include "asmjit/core/jitruntime.h"
#include "asmjit/core/logger.h"
#include "asmjit/core/string.h"
#include "asmjit/x86/x86compiler.h"
#include "asmjit/x86/x86operand.h"
#include "wasmOpcodes.hpp"
#include <asmjit/asmjit.h>
#include <cassert>
#include <cstddef>
#include <fcntl.h>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string_view>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include "tzUtils.hpp"

using namespace asmjit;
using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;

using ArenaAllocator = DynamicArenaAllocator<1024 * 16>;

class MappedFile : NonCopyable {
public:
  MappedFile(const std::string &filename) {
    fd = open(filename.c_str(), O_RDONLY);
    if (fd == -1) {
      throw std::runtime_error("Failed to open file: " + filename);
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
      close(fd);
      throw std::runtime_error("Failed to get file size: " + filename);
    }

    length = sb.st_size;

    addr = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
      close(fd);
      throw std::runtime_error("Failed to map file: " + filename);
    }
  }

  MappedFile(MappedFile &&other) {
    fd = other.fd;
    addr = other.addr;
    length = other.length;
    other.fd = -1;
    other.addr = nullptr;
    other.length = 0;
  }

  MappedFile &operator=(MappedFile &&other) {
    if (this != &other) {
      fd = other.fd;
      addr = other.addr;
      length = other.length;
      other.fd = -1;
      other.addr = nullptr;
      other.length = 0;
    }
    return *this;
  }

  ~MappedFile() {
    munmap(addr, length);
    close(fd);
  }

  void dump() {
    for (std::size_t i = 0; i < length; i++) {
      std::cout << std::hex << static_cast<int>(data()[i]) << " ";
    }
    std::cout << std::endl;
  }

  const u8 *data() const { return static_cast<const u8 *>(addr); }

  std::size_t size() const { return length; }

private:
  int fd;
  void *addr;
  std::size_t length;
};

class BinaryReader {
public:
  BinaryReader(const u8 *data, std::size_t size)
      : data(data), size(size), pos(0) {}

  template <class T> T read() {
    if (pos + sizeof(T) > size) {
      throw std::runtime_error("Out of data");
    }
    T value = *reinterpret_cast<const T *>(data + pos);
    pos += sizeof(T);
    return value;
  }

  template <typename T> T readIntLeb() {
    static_assert(std::is_integral<T>::value && !std::is_same<T, bool>::value,
                  "T must be integral");
    using U = typename std::make_unsigned<T>::type;
    uint32_t shift = 0;
    U result = 0;
    while (true) {
      assert(shift < sizeof(T) * 8);
      uint8_t value = *reinterpret_cast<const uint8_t *>(data + pos);
      result |= static_cast<U>(value & 0x7f) << shift;
      shift += 7;
      pos++;
      if ((value & 0x80) == 0) {
        // If the type is signed and the value is negative, do sign extension
        //
        if constexpr (std::is_signed<T>::value) {
          if ((value & 0x40) && shift < sizeof(T) * 8) {
            result |= (~static_cast<U>(0)) << shift;
          }
        }
        break;
      }
    }
    assert(pos <= size);
    return static_cast<T>(result);
  }

  std::string_view readStr() {
    auto length = readIntLeb<u32>();
    if (pos + length > size) {
      throw std::runtime_error("cant read string, out of data");
    }
    auto str =
        std::string_view(reinterpret_cast<const char *>(data + pos), length);
    pos += length;
    return str;
  }

  template <class T> T peek() const {
    if (pos + sizeof(T) > size) {
      throw std::runtime_error("Out of data");
    }
    return *reinterpret_cast<const T *>(data + pos);
  }

  [[nodiscard]] bool hasMore() const { return pos < size; }

private:
  const u8 *data;
  std::size_t size;
  std::size_t pos;
};

enum class WasmSectionId {
  CUSTOM_SECTION = 0,
  TYPE_SECTION = 1,
  IMPORT_SECTION = 2,
  FUNCTION_SECTION = 3,
  TABLE_SECTION = 4,
  MEMORY_SECTION = 5,
  GLOBAL_SECTION = 6,
  EXPORT_SECTION = 7,
  START_SECTION = 8,
  ELEMENT_SECTION = 9,
  CODE_SECTION = 10,
  DATA_SECTION = 11,
  X_END_OF_ENUM = 12
};

void dumpSection(WasmSectionId id) {
  switch (id) {
  case WasmSectionId::CUSTOM_SECTION:
    std::cout << "CUSTOM_SECTION" << std::endl;
    break;
  case WasmSectionId::TYPE_SECTION:
    std::cout << "TYPE_SECTION" << std::endl;
    break;
  case WasmSectionId::IMPORT_SECTION:
    std::cout << "IMPORT_SECTION" << std::endl;
    break;
  case WasmSectionId::FUNCTION_SECTION:
    std::cout << "FUNCTION_SECTION" << std::endl;
    break;
  case WasmSectionId::TABLE_SECTION:
    std::cout << "TABLE_SECTION" << std::endl;
    break;
  case WasmSectionId::MEMORY_SECTION:
    std::cout << "MEMORY_SECTION" << std::endl;
    break;
  case WasmSectionId::GLOBAL_SECTION:
    std::cout << "GLOBAL_SECTION" << std::endl;
    break;
  case WasmSectionId::EXPORT_SECTION:
    std::cout << "EXPORT_SECTION" << std::endl;
    break;
  case WasmSectionId::START_SECTION:
    std::cout << "START_SECTION" << std::endl;
    break;
  case WasmSectionId::ELEMENT_SECTION:
    std::cout << "ELEMENT_SECTION" << std::endl;
    break;
  case WasmSectionId::CODE_SECTION:
    std::cout << "CODE_SECTION" << std::endl;
    break;
  case WasmSectionId::DATA_SECTION:
    std::cout << "DATA_SECTION" << std::endl;
    break;
  case WasmSectionId::X_END_OF_ENUM:
    std::cout << "X_END_OF_ENUM" << std::endl;
    break;
  }
}

#define WASM_VALIDATE(COND, MSG)                                               \
  do {                                                                         \
    if (!(COND)) {                                                             \
      throw std::runtime_error(MSG);                                           \
    }                                                                          \
  } while (0)

enum class ValueType : u8 {
  I32 = 0x7F,
  I64 = 0x7E,
  F32 = 0x7D,
  F64 = 0x7C,
  NONE = 0,
};

std::string asStr(ValueType type) {
  switch (type) {
  case ValueType::I32:
    return "I32";
  case ValueType::I64:
    return "I64";
  case ValueType::F32:
    return "F32";
  case ValueType::F64:
    return "F64";
  case ValueType::NONE:
    return "NONE";
  }
  return "Unknown ValueType";
}

class FunctionPrototype : NonCopyable, NonMoveable {
public:
  void parse(ArenaAllocator &alloc, BinaryReader &reader) {
    u8 fnMagic = reader.read<u8>();
    WASM_VALIDATE(fnMagic == 0x60, "Invalid function prototype magic");
    auto paramCount = reader.readIntLeb<u32>();
    paramTypes = alloc.constructSpan<ValueType[]>(paramCount);
    for (auto &type : paramTypes) {
      type = static_cast<ValueType>(reader.read<u8>());
    }
    auto returnCount = reader.readIntLeb<u32>();
    WASM_VALIDATE(returnCount <= 1, "Invalid return count");
    if (returnCount > 0) {
      returnType = static_cast<ValueType>(reader.read<u8>());
    } else {
      returnType = ValueType::NONE;
    }
  }

  void dump() {
    std::cout << "FunctionPrototype: (";
    for (auto type : paramTypes) {
      std::cout << asStr(type) << " ";
    }
    std::cout << ") -> " << asStr(returnType) << std::endl;
  }

private:
  std::span<ValueType> paramTypes;
  ValueType returnType;
};

class TypeSection : NonCopyable, NonMoveable {
public:
  void parseSection(ArenaAllocator &alloc, BinaryReader &reader) {
    auto count = reader.readIntLeb<u32>();
    types = alloc.constructSpan<FunctionPrototype[]>(count);
    for (auto &type : types) {
      type.parse(alloc, reader);
    }
  }
  void dump() {
    for (auto &type : types) {
      type.dump();
    }
  }

  std::span<FunctionPrototype> types;
private:
};

class FunctionSection: NonCopyable, NonMoveable {
public:
  // TODO: missing a lot of stuff
  void parseSection(ArenaAllocator &alloc, BinaryReader &reader) {
    auto count = reader.readIntLeb<u32>();
    functions = alloc.constructSpan<u32[]>(count);
    for (auto &index : functions) {
      index = reader.readIntLeb<u32>();
    }
  }
  void dump() {
    for (auto index : functions) {
      std::cout << "Function index: " << std::to_string(index) << std::endl;
    }
  }

  std::span<u32> functions;
private:
};

enum class ExportType : u8 {
  FUNCTION = 0,
  TABLE = 1,
  MEMORY = 2,
  GLOBAL = 3,
};

std::string asStr(ExportType type) {
  switch (type) {
  case ExportType::FUNCTION:
    return "FUNCTION";
  case ExportType::TABLE:
    return "TABLE";
  case ExportType::MEMORY:
    return "MEMORY";
  case ExportType::GLOBAL:
    return "GLOBAL";
  }
  return "Unknown ExportType";
}

struct ExportEntity : NonCopyable, NonMoveable{
  std::string_view name;
  u32 entityIndex;
  ExportType type;

  void dump() const {
    std::cout << "ExportEntity: " << name << " -> " << asStr(type) << " "
              << std::to_string(entityIndex) << std::endl;
  }
};

class ExportSection : NonCopyable, NonMoveable {
public:
  void parseSection(ArenaAllocator &alloc, BinaryReader &reader) {
    auto count = reader.readIntLeb<u32>();
    exports = alloc.constructSpan<ExportEntity[]>(count);
    for (auto &entity : exports) {
      auto name = reader.readStr();
      auto strMem = alloc.constructSpan<u8>(name.size());
      std::copy(name.begin(), name.end(), strMem.begin());
      entity.name = std::string_view(
          reinterpret_cast<const char *>(strMem.data()), strMem.size());
      entity.type = static_cast<ExportType>(reader.read<u8>());
      entity.entityIndex = reader.readIntLeb<u32>();
    }
  }

  void dump() const {
    for (auto &entity : exports) {
      entity.dump();
    }
  }

private:
  std::span<ExportEntity> exports;
};

struct ImportedName: NonMoveable, NonCopyable {
  std::string_view l1Name;
  std::string_view l2Name;

  void parse(ArenaAllocator &alloc, BinaryReader &reader) {
    auto name1 = reader.readStr();
    auto name2 = reader.readStr();
    auto strMem1 = alloc.constructSpan<u8>(name1.size());
    auto strMem2 = alloc.constructSpan<u8>(name2.size());
    std::copy(name1.begin(), name1.end(), strMem1.begin());
    std::copy(name2.begin(), name2.end(), strMem2.begin());
    l1Name = std::string_view(reinterpret_cast<const char *>(strMem1.data()),
                              strMem1.size());
    l2Name = std::string_view(reinterpret_cast<const char *>(strMem2.data()),
                              strMem2.size());
  }

  void dump() {
    std::cout << "ImportedName: " << l1Name << " " << l2Name << std::endl;
  }
};

enum ImportType : u8 {
  FUNCTION = 0,
  TABLE = 1,
  MEMORY = 2,
  GLOBAL = 3,
};

class ImportSection: NonMoveable, NonCopyable {
public:
  void parseSection(ArenaAllocator &alloc, BinaryReader &reader) {
    auto count = reader.readIntLeb<u32>();
    importedFunctions = alloc.constructSpan<u32[]>(count);
    importedNames = alloc.constructSpan<ImportedName[]>(count);
    for (u32 i = 0; i < count; i++) {
      importedNames[i].parse(alloc, reader);
      u8 importType = reader.read<u8>();
      WASM_VALIDATE(importType == static_cast<u8>(ImportType::FUNCTION),
                    "Only function imports are supported");
      importedFunctions[i] = reader.readIntLeb<u32>();
    }
  }

  void dump() const {
    for (u32 i = 0; i < importedFunctions.size(); i++) {
      std::cout << "Imported function: " << importedFunctions[i] << " ";
      importedNames[i].dump();
    }
  }

private:
  std::span<u32> importedFunctions;
  std::span<ImportedName> importedNames;
};

class WasmModule : NonCopyable, NonMoveable {
public:
  WasmModule() {
    code.init(runtime.environment());
    code.setLogger(&logger);
  }

  void readCompressedLocals(BinaryReader &reader, std::vector<ValueType> &out) {
    auto count = reader.readIntLeb<u32>();
    for (u32 i = 0; i < count; i++) {
      u32 runLength = reader.readIntLeb<u32>();
      ValueType localtype = static_cast<ValueType>(reader.read<u8>());
      for (u32 j = 0; j < runLength; j++) {
        out.push_back(localtype);
      }
    }
  }

  FuncSignature genSignature(FunctionPrototype &signature) {


  }

  void *genCode(ArenaAllocator &alloc, BinaryReader &reader) {
    u32 numFuncs = reader.readIntLeb<u32>();
    std::cout << "numFuncs: " << std::to_string(numFuncs) << std::endl;
    std::vector<ValueType> localTypes;

    for (u32 i = 0; i < numFuncs; i++) {
      auto typeIdx = fnDeclarationSection.functions[i];
      auto& signature = typeSection.types[typeIdx];
      signature.dump();

      std::cout << "Function: " << std::to_string(i) << std::endl;
      u32 fnSize = reader.readIntLeb<u32>();
      std::cout << "fnSize: " << std::to_string(fnSize) << std::endl;
      readCompressedLocals(reader, localTypes);
      std::cout << "localTypes: " << std::to_string(localTypes.size())
                << std::endl;
    }
    return nullptr;
  }

  void compile(std::span<const u8> wasm_file) {
    ArenaAllocator allocator;
    BinaryReader reader(wasm_file.data(), wasm_file.size());

    auto magic = reader.read<u32>();
    WASM_VALIDATE(magic == 0x6d736100U, "Invalid magic number");
    auto version = reader.read<u32>();
    WASM_VALIDATE(version == 1, "Invalid version");

    while (reader.hasMore()) {
      auto sectionId = reader.read<u8>();
      WASM_VALIDATE(sectionId < static_cast<u8>(WasmSectionId::X_END_OF_ENUM),
                    "Invalid section id");
      WASM_VALIDATE(sectionId != static_cast<u8>(WasmSectionId::CUSTOM_SECTION),
                    "Custom sections are not supported");
      auto sectionSize = reader.readIntLeb<u32>();
      dumpSection(static_cast<WasmSectionId>(sectionId));
      std::cout << std::to_string(sectionSize) << std::endl;
      switch (static_cast<WasmSectionId>(sectionId)) {
      case WasmSectionId::TYPE_SECTION:
        typeSection.parseSection(allocator, reader);
        typeSection.dump();
        break;
      case WasmSectionId::IMPORT_SECTION:
        importSection.parseSection(allocator, reader);
        importSection.dump();
        break;
      case WasmSectionId::FUNCTION_SECTION:
        fnDeclarationSection.parseSection(allocator, reader);
        fnDeclarationSection.dump();
        break;
      case WasmSectionId::TABLE_SECTION:
        break;
      case WasmSectionId::MEMORY_SECTION:
        break;
      case WasmSectionId::GLOBAL_SECTION:
        break;
      case WasmSectionId::EXPORT_SECTION:
        exportSection.parseSection(allocator, reader);
        exportSection.dump();
        break;
      case WasmSectionId::START_SECTION:
        break;
      case WasmSectionId::ELEMENT_SECTION:
        break;
      case WasmSectionId::CODE_SECTION:
        genCode(allocator, reader);
        break;
      case WasmSectionId::DATA_SECTION:
        break;
      case WasmSectionId::X_END_OF_ENUM:
        break;
      }
    }
  }

private:
  TypeSection typeSection;
  FunctionSection fnDeclarationSection;
  ExportSection exportSection;
  ImportSection importSection;
  JitRuntime runtime;
  CodeHolder code;
  StringLogger logger;

};

int main() {
  auto wasm_file = MappedFile("../wasm_adder.wasm");
  wasm_file.dump();
  WasmModule module;
  module.compile({wasm_file.data(), wasm_file.size()});

  JitRuntime rt;

  StringLogger logger;
  CodeHolder code;

  code.init(rt.environment(), rt.cpuFeatures());
  code.setLogger(&logger);

  x86::Compiler cc(&code);
  auto sig = FuncSignature::build<int, int, int>();

  auto funcNode = cc.addFunc(sig);

  funcNode->frame().setPreservedFP();

  x86::Gp a = cc.newInt32("a");
  x86::Gp b = cc.newInt32("b");

  funcNode->setArg(0, a);
  funcNode->setArg(1, b);
  cc.add(a, b);
  cc.ret(a);
  cc.endFunc();
  cc.finalize();

  typedef int (*AddFunc)(int, int);
  AddFunc addFunc;
  Error err = rt.add(&addFunc, &code);
  if (err) {
    printf("Error: %s\n", DebugUtils::errorAsString(err));
    return 1;
  }

  int result = addFunc(40, 2);

  printf("result: %d\n", result);
  printf("logger: %s\n", logger.data());

  return 0;
}
