#pragma once
#include <cstdint>
#include <string_view>


enum class WasmDefines : uint32_t {
  MAGIC = 0x6D736100,
};

#define FOR_EACH_WASM_OPCODE                                                                                                \
  /*   Name          Encoding       IsSpecial   #Int Consume/#Float Consume/Output           Operand Encoding Type */       \
F( UNREACHABLE,        0x00,           true                                        ,   WasmOpcodeOperandKind::NONE        ) \
F( NOP,                0x01,           true                                        ,   WasmOpcodeOperandKind::NONE        ) \
F( BLOCK,              0x02,           true                                        ,   WasmOpcodeOperandKind::BLOCKTYPE   ) \
F( LOOP,               0x03,           true                                        ,   WasmOpcodeOperandKind::BLOCKTYPE   ) \
F( IF,                 0x04,           true                                        ,   WasmOpcodeOperandKind::BLOCKTYPE   ) \
F( ELSE,               0x05,           true                                        ,   WasmOpcodeOperandKind::NONE        ) \
F( END,                0x0B,           true                                        ,   WasmOpcodeOperandKind::NONE        ) \
F( BR,                 0x0C,           true                                        ,   WasmOpcodeOperandKind::U32         ) \
F( BR_IF,              0x0D,           true                                        ,   WasmOpcodeOperandKind::U32         ) \
F( BR_TABLE,           0x0E,           true                                        ,   WasmOpcodeOperandKind::SPECIAL     ) \
F( RETURN,             0x0F,           true                                        ,   WasmOpcodeOperandKind::NONE        ) \
                                                                                                                            \
F( CALL,               0x10,           true                                        ,   WasmOpcodeOperandKind::U32         ) \
F( CALL_INDIRECT,      0x11,           true                                        ,   WasmOpcodeOperandKind::SPECIAL     ) \
                                                                                                                            \
F( DROP,               0x1A,           true                                        ,   WasmOpcodeOperandKind::NONE        ) \
F( SELECT,             0x1B,           true                                        ,   WasmOpcodeOperandKind::NONE        ) \
                                                                                                                            \
F( LOCAL_GET,          0x20,           true                                        ,   WasmOpcodeOperandKind::U32         ) \
F( LOCAL_SET,          0x21,           true                                        ,   WasmOpcodeOperandKind::U32         ) \
F( LOCAL_TEE,          0x22,           true                                        ,   WasmOpcodeOperandKind::U32         ) \
F( GLOBAL_GET,         0x23,           true                                        ,   WasmOpcodeOperandKind::U32         ) \
F( GLOBAL_SET,         0x24,           true                                        ,   WasmOpcodeOperandKind::U32         ) \
                                                                                                                            \
F( I32_LOAD,           0x28,         false,    1,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::MEM_U32_U32 ) \
F( I64_LOAD,           0x29,         false,    1,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::MEM_U32_U32 ) \
F( F32_LOAD,           0x2A,         false,    1,  0,  WasmValueType::F32          ,   WasmOpcodeOperandKind::MEM_U32_U32 ) \
F( F64_LOAD,           0x2B,         false,    1,  0,  WasmValueType::F64          ,   WasmOpcodeOperandKind::MEM_U32_U32 ) \
                                                                                                                            \
F( I32_LOAD_8S,        0x2C,         false,    1,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::MEM_U32_U32 ) \
F( I32_LOAD_8U,        0x2D,         false,    1,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::MEM_U32_U32 ) \
F( I32_LOAD_16S,       0x2E,         false,    1,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::MEM_U32_U32 ) \
F( I32_LOAD_16U,       0x2F,         false,    1,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::MEM_U32_U32 ) \
                                                                                                                            \
F( I64_LOAD_8S,        0x30,         false,    1,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::MEM_U32_U32 ) \
F( I64_LOAD_8U,        0x31,         false,    1,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::MEM_U32_U32 ) \
F( I64_LOAD_16S,       0x32,         false,    1,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::MEM_U32_U32 ) \
F( I64_LOAD_16U,       0x33,         false,    1,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::MEM_U32_U32 ) \
F( I64_LOAD_32S,       0x34,         false,    1,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::MEM_U32_U32 ) \
F( I64_LOAD_32U,       0x35,         false,    1,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::MEM_U32_U32 ) \
                                                                                                                            \
F( I32_STORE,          0x36,         false,    2,  0,  WasmValueType::X_END_OF_ENUM,   WasmOpcodeOperandKind::MEM_U32_U32 ) \
F( I64_STORE,          0x37,         false,    2,  0,  WasmValueType::X_END_OF_ENUM,   WasmOpcodeOperandKind::MEM_U32_U32 ) \
F( F32_STORE,          0x38,         false,    1,  1,  WasmValueType::X_END_OF_ENUM,   WasmOpcodeOperandKind::MEM_U32_U32 ) \
F( F64_STORE,          0x39,         false,    1,  1,  WasmValueType::X_END_OF_ENUM,   WasmOpcodeOperandKind::MEM_U32_U32 ) \
F( I32_STORE_8,        0x3A,         false,    2,  0,  WasmValueType::X_END_OF_ENUM,   WasmOpcodeOperandKind::MEM_U32_U32 ) \
F( I32_STORE_16,       0x3B,         false,    2,  0,  WasmValueType::X_END_OF_ENUM,   WasmOpcodeOperandKind::MEM_U32_U32 ) \
F( I64_STORE_8,        0x3C,         false,    2,  0,  WasmValueType::X_END_OF_ENUM,   WasmOpcodeOperandKind::MEM_U32_U32 ) \
F( I64_STORE_16,       0x3D,         false,    2,  0,  WasmValueType::X_END_OF_ENUM,   WasmOpcodeOperandKind::MEM_U32_U32 ) \
F( I64_STORE_32,       0x3E,         false,    2,  0,  WasmValueType::X_END_OF_ENUM,   WasmOpcodeOperandKind::MEM_U32_U32 ) \
                                                                                                                            \
F( MEMORY_SIZE,        0x3F,           true                                        ,   WasmOpcodeOperandKind::U32         ) \
F( MEMORY_GROW,        0x40,           true                                        ,   WasmOpcodeOperandKind::U32         ) \
                                                                                                                            \
F( I32_CONST,          0x41,         false,    0,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::CONST       ) \
F( I64_CONST,          0x42,         false,    0,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::CONST       ) \
F( F32_CONST,          0x43,         false,    0,  0,  WasmValueType::F32          ,   WasmOpcodeOperandKind::CONST       ) \
F( F64_CONST,          0x44,         false,    0,  0,  WasmValueType::F64          ,   WasmOpcodeOperandKind::CONST       ) \
                                                                                                                            \
F( I32_EQZ,            0x45,         false,    1,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
                                                                                                                            \
F( I32_EQ,             0x46,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_NE,             0x47,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_LT_S,           0x48,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_LT_U,           0x49,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_GT_S,           0x4A,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_GT_U,           0x4B,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_LE_S,           0x4C,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_LE_U,           0x4D,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_GE_S,           0x4E,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_GE_U,           0x4F,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
                                                                                                                            \
F( I64_EQZ,            0x50,         false,    1,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
                                                                                                                            \
F( I64_EQ,             0x51,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_NE,             0x52,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_LT_S,           0x53,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_LT_U,           0x54,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_GT_S,           0x55,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_GT_U,           0x56,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_LE_S,           0x57,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_LE_U,           0x58,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_GE_S,           0x59,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_GE_U,           0x5A,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
                                                                                                                            \
F( F32_EQ,             0x5B,         false,    0,  2,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( F32_NE,             0x5C,         false,    0,  2,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( F32_LT,             0x5D,         false,    0,  2,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( F32_GT,             0x5E,         false,    0,  2,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( F32_LE,             0x5F,         false,    0,  2,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( F32_GE,             0x60,         false,    0,  2,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
                                                                                                                            \
F( F64_EQ,             0x61,         false,    0,  2,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( F64_NE,             0x62,         false,    0,  2,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( F64_LT,             0x63,         false,    0,  2,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( F64_GT,             0x64,         false,    0,  2,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( F64_LE,             0x65,         false,    0,  2,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( F64_GE,             0x66,         false,    0,  2,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
                                                                                                                            \
F( I32_CLZ,            0x67,         false,    1,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_CTZ,            0x68,         false,    1,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_POPCNT,         0x69,         false,    1,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
                                                                                                                            \
F( I32_ADD,            0x6A,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_SUB,            0x6B,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_MUL,            0x6C,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_DIV_S,          0x6D,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_DIV_U,          0x6E,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_REM_S,          0x6F,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_REM_U,          0x70,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_AND,            0x71,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_OR,             0x72,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_XOR,            0x73,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_SHL,            0x74,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_SHR_S,          0x75,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_SHR_U,          0x76,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_ROTL,           0x77,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_ROTR,           0x78,         false,    2,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
                                                                                                                            \
F( I64_CLZ,            0x79,         false,    1,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_CTZ,            0x7A,         false,    1,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_POPCNT,         0x7B,         false,    1,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::NONE        ) \
                                                                                                                            \
F( I64_ADD,            0x7C,         false,    2,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_SUB,            0x7D,         false,    2,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_MUL,            0x7E,         false,    2,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_DIV_S,          0x7F,         false,    2,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_DIV_U,          0x80,         false,    2,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_REM_S,          0x81,         false,    2,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_REM_U,          0x82,         false,    2,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_AND,            0x83,         false,    2,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_OR,             0x84,         false,    2,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_XOR,            0x85,         false,    2,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_SHL,            0x86,         false,    2,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_SHR_S,          0x87,         false,    2,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_SHR_U,          0x88,         false,    2,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_ROTL,           0x89,         false,    2,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_ROTR,           0x8A,         false,    2,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::NONE        ) \
                                                                                                                            \
F( F32_ABS,            0x8B,         false,    0,  1,  WasmValueType::F32          ,   WasmOpcodeOperandKind::NONE        ) \
F( F32_NEG,            0x8C,         false,    0,  1,  WasmValueType::F32          ,   WasmOpcodeOperandKind::NONE        ) \
F( F32_CEIL,           0x8D,         false,    0,  1,  WasmValueType::F32          ,   WasmOpcodeOperandKind::NONE        ) \
F( F32_FLOOR,          0x8E,         false,    0,  1,  WasmValueType::F32          ,   WasmOpcodeOperandKind::NONE        ) \
F( F32_TRUNC,          0x8F,         false,    0,  1,  WasmValueType::F32          ,   WasmOpcodeOperandKind::NONE        ) \
F( F32_NEAREST,        0x90,         false,    0,  1,  WasmValueType::F32          ,   WasmOpcodeOperandKind::NONE        ) \
F( F32_SQRT,           0x91,         false,    0,  1,  WasmValueType::F32          ,   WasmOpcodeOperandKind::NONE        ) \
                                                                                                                            \
F( F32_ADD,            0x92,         false,    0,  2,  WasmValueType::F32          ,   WasmOpcodeOperandKind::NONE        ) \
F( F32_SUB,            0x93,         false,    0,  2,  WasmValueType::F32          ,   WasmOpcodeOperandKind::NONE        ) \
F( F32_MUL,            0x94,         false,    0,  2,  WasmValueType::F32          ,   WasmOpcodeOperandKind::NONE        ) \
F( F32_DIV,            0x95,         false,    0,  2,  WasmValueType::F32          ,   WasmOpcodeOperandKind::NONE        ) \
F( F32_MIN,            0x96,         false,    0,  2,  WasmValueType::F32          ,   WasmOpcodeOperandKind::NONE        ) \
F( F32_MAX,            0x97,         false,    0,  2,  WasmValueType::F32          ,   WasmOpcodeOperandKind::NONE        ) \
F( F32_COPYSIGN,       0x98,         false,    0,  2,  WasmValueType::F32          ,   WasmOpcodeOperandKind::NONE        ) \
                                                                                                                            \
F( F64_ABS,            0x99,         false,    0,  1,  WasmValueType::F64          ,   WasmOpcodeOperandKind::NONE        ) \
F( F64_NEG,            0x9A,         false,    0,  1,  WasmValueType::F64          ,   WasmOpcodeOperandKind::NONE        ) \
F( F64_CEIL,           0x9B,         false,    0,  1,  WasmValueType::F64          ,   WasmOpcodeOperandKind::NONE        ) \
F( F64_FLOOR,          0x9C,         false,    0,  1,  WasmValueType::F64          ,   WasmOpcodeOperandKind::NONE        ) \
F( F64_TRUNC,          0x9D,         false,    0,  1,  WasmValueType::F64          ,   WasmOpcodeOperandKind::NONE        ) \
F( F64_NEAREST,        0x9E,         false,    0,  1,  WasmValueType::F64          ,   WasmOpcodeOperandKind::NONE        ) \
F( F64_SQRT,           0x9F,         false,    0,  1,  WasmValueType::F64          ,   WasmOpcodeOperandKind::NONE        ) \
                                                                                                                            \
F( F64_ADD,            0xA0,         false,    0,  2,  WasmValueType::F64          ,   WasmOpcodeOperandKind::NONE        ) \
F( F64_SUB,            0xA1,         false,    0,  2,  WasmValueType::F64          ,   WasmOpcodeOperandKind::NONE        ) \
F( F64_MUL,            0xA2,         false,    0,  2,  WasmValueType::F64          ,   WasmOpcodeOperandKind::NONE        ) \
F( F64_DIV,            0xA3,         false,    0,  2,  WasmValueType::F64          ,   WasmOpcodeOperandKind::NONE        ) \
F( F64_MIN,            0xA4,         false,    0,  2,  WasmValueType::F64          ,   WasmOpcodeOperandKind::NONE        ) \
F( F64_MAX,            0xA5,         false,    0,  2,  WasmValueType::F64          ,   WasmOpcodeOperandKind::NONE        ) \
F( F64_COPYSIGN,       0xA6,         false,    0,  2,  WasmValueType::F64          ,   WasmOpcodeOperandKind::NONE        ) \
                                                                                                                            \
F( I32_WRAP_I64,       0xA7,         false,    1,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_TRUNC_F32_S,    0xA8,         false,    0,  1,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_TRUNC_F32_U,    0xA9,         false,    0,  1,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_TRUNC_F64_S,    0xAA,         false,    0,  1,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_TRUNC_F64_U,    0xAB,         false,    0,  1,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
                                                                                                                            \
F( I64_EXTEND_I32_S,   0xAC,         false,    1,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_EXTEND_I32_U,   0xAD,         false,    1,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_TRUNC_F32_S,    0xAE,         false,    0,  1,  WasmValueType::I64          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_TRUNC_F32_U,    0xAF,         false,    0,  1,  WasmValueType::I64          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_TRUNC_F64_S,    0xB0,         false,    0,  1,  WasmValueType::I64          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_TRUNC_F64_U,    0xB1,         false,    0,  1,  WasmValueType::I64          ,   WasmOpcodeOperandKind::NONE        ) \
                                                                                                                            \
F( F32_CONVERT_I32_S,  0xB2,         false,    1,  0,  WasmValueType::F32          ,   WasmOpcodeOperandKind::NONE        ) \
F( F32_CONVERT_I32_U,  0xB3,         false,    1,  0,  WasmValueType::F32          ,   WasmOpcodeOperandKind::NONE        ) \
F( F32_CONVERT_I64_S,  0xB4,         false,    1,  0,  WasmValueType::F32          ,   WasmOpcodeOperandKind::NONE        ) \
F( F32_CONVERT_I64_U,  0xB5,         false,    1,  0,  WasmValueType::F32          ,   WasmOpcodeOperandKind::NONE        ) \
F( F32_DEMOTE_F64,     0xB6,         false,    0,  1,  WasmValueType::F32          ,   WasmOpcodeOperandKind::NONE        ) \
                                                                                                                            \
F( F64_CONVERT_I32_S,  0xB7,         false,    1,  0,  WasmValueType::F64          ,   WasmOpcodeOperandKind::NONE        ) \
F( F64_CONVERT_I32_U,  0xB8,         false,    1,  0,  WasmValueType::F64          ,   WasmOpcodeOperandKind::NONE        ) \
F( F64_CONVERT_I64_S,  0xB9,         false,    1,  0,  WasmValueType::F64          ,   WasmOpcodeOperandKind::NONE        ) \
F( F64_CONVERT_I64_U,  0xBA,         false,    1,  0,  WasmValueType::F64          ,   WasmOpcodeOperandKind::NONE        ) \
F( F64_PROMOTE_F32,    0xBB,         false,    0,  1,  WasmValueType::F64          ,   WasmOpcodeOperandKind::NONE        ) \
                                                                                                                            \
F( I32_BITCAST_F32,    0xBC,         false,    0,  1,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_BITCAST_F64,    0xBD,         false,    0,  1,  WasmValueType::I64          ,   WasmOpcodeOperandKind::NONE        ) \
F( F32_BITCAST_I32,    0xBE,         false,    1,  0,  WasmValueType::F32          ,   WasmOpcodeOperandKind::NONE        ) \
F( F64_BITCAST_I64,    0xBF,         false,    1,  0,  WasmValueType::F64          ,   WasmOpcodeOperandKind::NONE        ) \
                                                                                                                            \
F( I32_EXTEND_8S,      0xC0,         false,    1,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I32_EXTEND_16S,     0xC1,         false,    1,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_EXTEND_8S,      0xC2,         false,    1,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_EXTEND_16S,     0xC3,         false,    1,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::NONE        ) \
F( I64_EXTEND_32S,     0xC4,         false,    1,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::NONE        ) \
                                                                                                                            \
/* HACK: ops invented by us for helper */                                                                                   \
F( XX_SWITCH_SF,       0xD6,         false,    0,  0,  WasmValueType::X_END_OF_ENUM,   WasmOpcodeOperandKind::NONE        ) \
F( XX_I32_FILLPARAM,   0xD7,         false,    1,  0,  WasmValueType::X_END_OF_ENUM,   WasmOpcodeOperandKind::NONE        ) \
F( XX_I64_FILLPARAM,   0xD8,         false,    1,  0,  WasmValueType::X_END_OF_ENUM,   WasmOpcodeOperandKind::NONE        ) \
F( XX_F32_FILLPARAM,   0xD9,         false,    0,  1,  WasmValueType::X_END_OF_ENUM,   WasmOpcodeOperandKind::NONE        ) \
F( XX_F64_FILLPARAM,   0xDA,         false,    0,  1,  WasmValueType::X_END_OF_ENUM,   WasmOpcodeOperandKind::NONE        ) \
                                                                                                                            \
F( XX_I32_RETURN,      0xDB,         false,    1,  0,  WasmValueType::X_END_OF_ENUM,   WasmOpcodeOperandKind::NONE        ) \
F( XX_I64_RETURN,      0xDC,         false,    1,  0,  WasmValueType::X_END_OF_ENUM,   WasmOpcodeOperandKind::NONE        ) \
F( XX_F32_RETURN,      0xDD,         false,    0,  1,  WasmValueType::X_END_OF_ENUM,   WasmOpcodeOperandKind::NONE        ) \
F( XX_F64_RETURN,      0xDE,         false,    0,  1,  WasmValueType::X_END_OF_ENUM,   WasmOpcodeOperandKind::NONE        ) \
F( XX_NONE_RETURN,     0xDF,         false,    0,  0,  WasmValueType::X_END_OF_ENUM,   WasmOpcodeOperandKind::NONE        ) \
                                                                                                                            \
F( XX_I_DROP,          0xE0,         false,    1,  0,  WasmValueType::X_END_OF_ENUM,   WasmOpcodeOperandKind::NONE        ) \
F( XX_F_DROP,          0xE1,         false,    0,  1,  WasmValueType::X_END_OF_ENUM,   WasmOpcodeOperandKind::NONE        ) \
                                                                                                                            \
F( XX_I32_SELECT,      0xE2,         false,    3,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::NONE        ) \
F( XX_I64_SELECT,      0xE3,         false,    3,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::NONE        ) \
F( XX_F32_SELECT,      0xE4,         false,    1,  2,  WasmValueType::F32          ,   WasmOpcodeOperandKind::NONE        ) \
F( XX_F64_SELECT,      0xE5,         false,    1,  2,  WasmValueType::F64          ,   WasmOpcodeOperandKind::NONE        ) \
                                                                                                                            \
F( XX_I32_LOCAL_GET,   0xE6,         false,    0,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::U32         ) \
F( XX_I64_LOCAL_GET,   0xE7,         false,    0,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::U32         ) \
F( XX_F32_LOCAL_GET,   0xE8,         false,    0,  0,  WasmValueType::F32          ,   WasmOpcodeOperandKind::U32         ) \
F( XX_F64_LOCAL_GET,   0xE9,         false,    0,  0,  WasmValueType::F64          ,   WasmOpcodeOperandKind::U32         ) \
                                                                                                                            \
F( XX_I32_LOCAL_SET,   0xEA,         false,    1,  0,  WasmValueType::X_END_OF_ENUM,   WasmOpcodeOperandKind::U32         ) \
F( XX_I64_LOCAL_SET,   0xEB,         false,    1,  0,  WasmValueType::X_END_OF_ENUM,   WasmOpcodeOperandKind::U32         ) \
F( XX_F32_LOCAL_SET,   0xEC,         false,    0,  1,  WasmValueType::X_END_OF_ENUM,   WasmOpcodeOperandKind::U32         ) \
F( XX_F64_LOCAL_SET,   0xED,         false,    0,  1,  WasmValueType::X_END_OF_ENUM,   WasmOpcodeOperandKind::U32         ) \
                                                                                                                            \
F( XX_I32_LOCAL_TEE,   0xEE,         false,    1,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::U32         ) \
F( XX_I64_LOCAL_TEE,   0xEF,         false,    1,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::U32         ) \
F( XX_F32_LOCAL_TEE,   0xF0,         false,    0,  1,  WasmValueType::F32          ,   WasmOpcodeOperandKind::U32         ) \
F( XX_F64_LOCAL_TEE,   0xF1,         false,    0,  1,  WasmValueType::F64          ,   WasmOpcodeOperandKind::U32         ) \
                                                                                                                            \
F( XX_I32_GLOBAL_GET,  0xF2,         false,    0,  0,  WasmValueType::I32          ,   WasmOpcodeOperandKind::U32         ) \
F( XX_I64_GLOBAL_GET,  0xF3,         false,    0,  0,  WasmValueType::I64          ,   WasmOpcodeOperandKind::U32         ) \
F( XX_F32_GLOBAL_GET,  0xF4,         false,    0,  0,  WasmValueType::F32          ,   WasmOpcodeOperandKind::U32         ) \
F( XX_F64_GLOBAL_GET,  0xF5,         false,    0,  0,  WasmValueType::F64          ,   WasmOpcodeOperandKind::U32         ) \
                                                                                                                            \
F( XX_I32_GLOBAL_SET,  0xF6,         false,    1,  0,  WasmValueType::X_END_OF_ENUM,   WasmOpcodeOperandKind::U32         ) \
F( XX_I64_GLOBAL_SET,  0xF7,         false,    1,  0,  WasmValueType::X_END_OF_ENUM,   WasmOpcodeOperandKind::U32         ) \
F( XX_F32_GLOBAL_SET,  0xF8,         false,    0,  1,  WasmValueType::X_END_OF_ENUM,   WasmOpcodeOperandKind::U32         ) \
F( XX_F64_GLOBAL_SET,  0xF9,         false,    0,  1,  WasmValueType::X_END_OF_ENUM,   WasmOpcodeOperandKind::U32         )

enum class WasmOpcode : uint8_t {
#define F(opcodeName, opcodeEncoding, ...) opcodeName = opcodeEncoding,
  FOR_EACH_WASM_OPCODE
#undef F
      X_END_OF_ENUM
};

struct alignas(64) OpcodeStringTable
{
    constexpr OpcodeStringTable()
    {
#define F(opcodeName, opcodeEncoding, ...) m_info[opcodeEncoding] = #opcodeName;
FOR_EACH_WASM_OPCODE
#undef F
    }


    std::string_view Get(WasmOpcode opcode) const
    {
        return m_info[static_cast<uint8_t>(opcode)];
    }

    std::string_view m_info[256];

};
