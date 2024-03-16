#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include "ArenaAllocator.hpp"
#include "tz-utils.hpp"


using ArenaAllocator = DynamicArenaAllocator<1024 * 4>;

namespace wasmjit {

class BinaryReader {
public:
  BinaryReader(const u8 *data, std::size_t size)
      : data(data), size(size), pos(0) {}

  BinaryReader() : data(nullptr), size(0), pos(0) {}

  template <class T> T read();
  template <typename T> T readIntLeb();
  std::string_view readStr();
  template <class T> T peek() const;

  [[nodiscard]] bool hasMore() const;
  std::span<const u8> readChunk(std::size_t count);

private:
  const uint8_t *data;
  std::size_t size;
  std::size_t pos;
};

template <typename T>
T BinaryReader::readIntLeb() {
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



enum class WasmSection {
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
  SIZE = 12
};

std::string_view toString(WasmSection section);

enum class WasmValueType : u8 {
  I32 = 0x7F,
  I64 = 0x7E,
  F32 = 0x7D,
  F64 = 0x7C,
  NONE = 0,
};

std::string_view toString(WasmValueType type);

enum class ExportType : u8 {
  FUNCTION = 0,
  TABLE = 1,
  MEMORY = 2,
  GLOBAL = 3,
};

std::string_view toString(ExportType type);

enum ImportType : u8 {
  FUNCTION = 0,
  TABLE = 1,
  MEMORY = 2,
  GLOBAL = 3,
};

std::string_view toString(ImportType type);

struct FunctionPrototype : NonCopyable, NonMoveable {
  void parse(ArenaAllocator &alloc, BinaryReader &reader);
  void dump() const;

  std::span<WasmValueType> paramTypes;
  WasmValueType returnType;
};

struct TypeSection : NonCopyable, NonMoveable {
  void parseSection(ArenaAllocator &alloc, BinaryReader &reader);
  void dump() const;

  std::span<FunctionPrototype> types;
};

struct FunctionSection : NonCopyable, NonMoveable {
  // TODO: missing a lot of stuff
  void parseSection(ArenaAllocator &alloc, BinaryReader &reader);
  void dump() const;

  std::span<u32> functions;
};

struct ExportEntity : NonCopyable, NonMoveable {
  void dump() const;

  std::string_view name;
  u32 entityIndex;
  ExportType type;
};

struct ExportSection : NonCopyable, NonMoveable {
  void parseSection(ArenaAllocator &alloc, BinaryReader &reader);
  void dump() const;

  std::span<ExportEntity> exports;
};

struct ImportedName {
  std::string_view l1Name;
  std::string_view l2Name;

  void parse(ArenaAllocator &alloc, BinaryReader &reader);
  void dump() const;
};

struct WasmGlobal {
  void parse(BinaryReader &reader);
  void dump() const;


  WasmValueType type;
  bool isMutable;
};

struct WasmLimit {
  u32 minSize = 0;
  u32 maxSize;

  void parse(BinaryReader &reader);
  void dump() const;
};

struct ImportSection : NonMoveable, NonCopyable {
public:
  using import_t = std::variant<u32, WasmGlobal>;
  void parseSection(ArenaAllocator &alloc, BinaryReader &reader);

  ImportedName getFnName(u32 index) const;
  u32 getFnType(std::string_view name) const;
  ImportedName getGlobalName(u32 index) const;
  WasmGlobal getGlobalType(std::string_view name) const;

  void dump() const;

  u32 numImportedFuncs;
  u32 numImportedGlobals;
  std::span<import_t> imports;
  std::span<ImportedName> importedNames;
  // indices into the imports/names
  std::span<u32> importedFunctions;
  std::span<u32> importedGlobals;
  // limit and idx to name
  std::optional<std::pair<WasmLimit, u32>> importedTableLimit;
  std::optional<std::pair<WasmLimit, u32>> importedMemoryLimit;
};


std::string_view toString(WasmLimit limit);

struct TableSection : NonMoveable, NonCopyable {
  void parseSection(BinaryReader &reader);
  void dump() const;

  std::optional<WasmLimit> limit;
};

struct MemorySection : NonMoveable, NonCopyable {
  void parseSection(BinaryReader &reader);
  void dump() const;

  std::optional<WasmLimit> limit;
};

using value_t = std::variant<u32, i32, u64, i64, f32, f64>;

struct WasmConstExpr {
  void parse(BinaryReader &reader);
  void dump() const;

  bool isInitByGlobal;
  value_t value;
};

struct GlobalSection : NonMoveable, NonCopyable {
  void parseSection(ArenaAllocator& alloc, BinaryReader &reader);
  void dump() const;

  std::span<WasmGlobal> globals;
  std::span<WasmConstExpr> initExprs;
};


struct CodeSection : NonMoveable, NonCopyable {

  void dump() const;

  std::span<const u8> code;
};

struct WasmModule : NonCopyable, NonMoveable {
  void parseSections(std::span<const u8> data);
  FunctionPrototype& getPrototype(u32 index) const;
  void dump() const;

  ArenaAllocator allocator;
  TypeSection typeSection;
  FunctionSection functionSection;
  ExportSection exportSection;
  ImportSection importSection;
  TableSection tableSection;
  MemorySection memorySection;
  GlobalSection globalSection;
  CodeSection codeSection;
};

} // namespace wasmjit
