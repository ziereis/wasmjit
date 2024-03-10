#include <cassert>
#include <string_view>
#include <iostream>

#include "parser.hpp"
using namespace std::literals;


namespace wasmjit {

template <class T>
T BinaryReader::read() {
  if (pos + sizeof(T) > size) {
    throw std::runtime_error("Out of data");
  }
  T value = *reinterpret_cast<const T *>(data + pos);
  pos += sizeof(T);
  return value;
}

std::string_view BinaryReader::readStr() {
  auto length = readIntLeb<uint32_t>();
  if (pos + length > size) {
    throw std::runtime_error("cant read string, out of data");
  }
  auto str =
      std::string_view(reinterpret_cast<const char *>(data + pos), length);
  pos += length;
  return str;
}

template <class T>
T BinaryReader::peek() const {
  if (pos + sizeof(T) > size) {
    throw std::runtime_error("Out of data");
  }
  return *reinterpret_cast<const T *>(data + pos);
}

bool BinaryReader::hasMore() const { return pos < size; }

std::span<const u8> BinaryReader::readChunk(std::size_t count) {
  if (pos + count > size) {
    throw std::runtime_error("Out of data");
  }
  pos += count;
  return std::span<const u8>(data + pos, count);
}


#define WASM_VALIDATE(COND, MSG)                                               \
  do {                                                                         \
    if (!(COND)) {                                                             \
      throw std::runtime_error(MSG);                                           \
    }                                                                          \
  } while (0)


std::string_view toString(WasmSection section) {
  switch (section) {
  case WasmSection::CUSTOM_SECTION:
    return "CUSTOM_SECTION";
  case WasmSection::TYPE_SECTION:
    return "TYPE_SECTION";
  case WasmSection::IMPORT_SECTION:
    return "IMPORT_SECTION";
  case WasmSection::FUNCTION_SECTION:
    return "FUNCTION_SECTION";
  case WasmSection::TABLE_SECTION:
    return "TABLE_SECTION";
  case WasmSection::MEMORY_SECTION:
    return "MEMORY_SECTION";
  case WasmSection::GLOBAL_SECTION:
    return "GLOBAL_SECTION";
  case WasmSection::EXPORT_SECTION:
    return "EXPORT_SECTION";
  case WasmSection::START_SECTION:
    return "START_SECTION";
  case WasmSection::ELEMENT_SECTION:
    return "ELEMENT_SECTION";
  case WasmSection::CODE_SECTION:
    return "CODE_SECTION";
  case WasmSection::DATA_SECTION:
    return "DATA_SECTION";
  case WasmSection::SIZE:
    assert(false && "Invalid section");
    break;
  }
  assert(false);
  return ""sv;
}

std::string_view toString(WasmValueType type) {
  switch (type) {
  case WasmValueType::I32:
    return "i32";
  case WasmValueType::I64:
    return "i64";
  case WasmValueType::F32:
    return "f32";
  case WasmValueType::F64:
    return "f64";
  case WasmValueType::NONE:
    return "void";
  }
  assert(false);
  return "";
}

std::string_view toString(ExportType type) {
  switch (type) {
  case ExportType::FUNCTION:
    return "EXPORT.FUNCTION";
  case ExportType::TABLE:
    return "EXPRORT.TABLE";
  case ExportType::MEMORY:
    return "EXPRORT.MEMORY";
  case ExportType::GLOBAL:
    return "EXPRORT.GLOBAL";
  }
  assert(false);
  return "";
}

std::string_view toString(ImportType type) {
  switch (type) {
  case ImportType::FUNCTION:
    return "IMPORT.FUNCTION";
  case ImportType::TABLE:
    return "IMPORT.TABLE";
  case ImportType::MEMORY:
    return "IMPORT.MEMORY";
  case ImportType::GLOBAL:
    return "IMPORT.GLOBAL";
  }
  assert(false);
  return "";
}

void FunctionPrototype::parse(ArenaAllocator &alloc, BinaryReader &reader) {
  u8 fnMagic = reader.read<u8>();
  WASM_VALIDATE(fnMagic == 0x60, "Invalid function prototype magic");
  auto paramCount = reader.readIntLeb<u32>();
  paramTypes = alloc.constructSpan<WasmValueType>(paramCount);

  for (auto &type : paramTypes) {
    type = static_cast<WasmValueType>(reader.read<u8>());
  }

  auto returnCount = reader.readIntLeb<u32>();
  WASM_VALIDATE(returnCount <= 1, "Invalid return count");

  if (returnCount > 0) {
    returnType = static_cast<WasmValueType>(reader.read<u8>());
  } else {
    returnType = WasmValueType::NONE;
  }
}

void TypeSection::parseSection(ArenaAllocator &alloc, BinaryReader &reader) {
  auto count = reader.readIntLeb<u32>();
  types = alloc.constructSpan<FunctionPrototype[]>(count);
  for (auto &type : types) {
    type.parse(alloc, reader);
  }
}

void FunctionSection::parseSection(ArenaAllocator &alloc, BinaryReader &reader) {
  auto count = reader.readIntLeb<u32>();
  functions = alloc.constructSpan<u32[]>(count);
  for (auto &index : functions) {
    index = reader.readIntLeb<u32>();
  }
}

void ExportSection::parseSection(ArenaAllocator &alloc, BinaryReader &reader) {
  auto count = reader.readIntLeb<u32>();
  exports = alloc.constructSpan<ExportEntity>(count);
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

void ImportedName::parse(ArenaAllocator &alloc, BinaryReader &reader) {
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

void ImportSection::parseSection(ArenaAllocator &alloc, BinaryReader &reader) {
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


void FunctionPrototype::dump() const {
  std::cout << "FunctionPrototype: (";
  for (auto type : paramTypes) {
    std::cout << toString(type) << " ";
  }
  std::cout << ") -> " << toString(returnType) << std::endl;
}

void TypeSection::dump() const {
  for (auto &type : types) {
    type.dump();
  }
}

void FunctionSection::dump() const {
  for (auto index : functions) {
    std::cout << "Function index: " << std::to_string(index) << std::endl;
  }
}

void ExportEntity::dump() const {
  std::cout << "ExportEntity: " << name << " -> " << toString(type) << " "
            << std::to_string(entityIndex) << std::endl;
}

void ExportSection::dump() const {
  for (auto &entity : exports) {
    entity.dump();
  }
}

void ImportedName::dump() const {
  std::cout << "ImportedName: " << l1Name << " " << l2Name << std::endl;
}

void ImportSection::dump() const {
  for (u32 i = 0; i < importedFunctions.size(); i++) {
    std::cout << "Imported function: " << importedFunctions[i] << " ";
    importedNames[i].dump();
  }
}

FunctionPrototype &WasmModule::getPrototype(u32 index) const {
  u32 typeIdx = functionSection.functions[index];
  return typeSection.types[typeIdx];
}

void WasmModule::parseSections(std::span<const u8> wasmFile) {
  BinaryReader reader(wasmFile.data(), wasmFile.size());

  auto magic = reader.read<u32>();
  WASM_VALIDATE(magic == 0x6d736100U, "Invalid magic number");
  auto version = reader.read<u32>();
  WASM_VALIDATE(version == 1, "Invalid version");

  while (reader.hasMore()) {
    auto sectionId = reader.read<u8>();
    WASM_VALIDATE(sectionId < static_cast<u8>(WasmSection::SIZE),
                  "Invalid section id");
    WASM_VALIDATE(sectionId != static_cast<u8>(WasmSection::CUSTOM_SECTION),
                  "Custom sections are not supported");
    auto sectionSize = reader.readIntLeb<u32>();
    // std::cout << "Section: " << toString(static_cast<WasmSection>(sectionId))
    //           << " size: " << std::to_string(sectionSize) << std::endl;
    switch (static_cast<WasmSection>(sectionId)) {
    case WasmSection::TYPE_SECTION:
      typeSection.parseSection(allocator, reader);
      typeSection.dump();
      break;
    case WasmSection::IMPORT_SECTION:
      importSection.parseSection(allocator, reader);
      importSection.dump();
      break;
    case WasmSection::FUNCTION_SECTION:
      functionSection.parseSection(allocator, reader);
      functionSection.dump();
      break;
    case WasmSection::TABLE_SECTION:
      break;
    case WasmSection::MEMORY_SECTION:
      break;
    case WasmSection::GLOBAL_SECTION:
      break;
    case WasmSection::EXPORT_SECTION:
      exportSection.parseSection(allocator, reader);
      exportSection.dump();
      break;
    case WasmSection::START_SECTION:
      break;
    case WasmSection::ELEMENT_SECTION:
      break;
    case WasmSection::CODE_SECTION:
      codeSection.code = reader.readChunk(sectionSize);
      break;
    case WasmSection::DATA_SECTION:
      break;
    default:
      throw std::runtime_error("Invalid section id");
    }
  }
}

void WasmModule::dump() const {
  typeSection.dump();
  importSection.dump();
  functionSection.dump();
  exportSection.dump();
}

} // namespace wasmjit
