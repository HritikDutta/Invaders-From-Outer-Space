#pragma once

#include "core/types.h"

namespace Binary
{

// Common constants
#define type_data_pack(type, data, size) ((u8)(((type & 0b111u) << 5) | ((data & 0b11u) << 3) | (size & 0b111u)))

constexpr u8 NIL               = type_data_pack(0b000, 0b00, 0b000);

constexpr u8 BOOLEAN_FALSE     = type_data_pack(0b001, 0b00, 0b000);
constexpr u8 BOOLEAN_TRUE      = type_data_pack(0b001, 0b01, 0b000);

constexpr u8 INTEGER_U8        = type_data_pack(0b010, 0b00, 0b000);
constexpr u8 INTEGER_U16       = type_data_pack(0b010, 0b00, 0b001);
constexpr u8 INTEGER_U32       = type_data_pack(0b010, 0b00, 0b010);
constexpr u8 INTEGER_U64       = type_data_pack(0b010, 0b00, 0b011);

constexpr u8 INTEGER_S8        = type_data_pack(0b010, 0b01, 0b000);
constexpr u8 INTEGER_S16       = type_data_pack(0b010, 0b01, 0b001);
constexpr u8 INTEGER_S32       = type_data_pack(0b010, 0b01, 0b010);
constexpr u8 INTEGER_S64       = type_data_pack(0b010, 0b01, 0b011);

constexpr u8 FLOAT_32          = type_data_pack(0b011, 0b00, 0b010);
constexpr u8 FLOAT_64          = type_data_pack(0b011, 0b00, 0b011);

constexpr u8 STRING_1_BYTE     = type_data_pack(0b100, 0b00, 0b000);
constexpr u8 STRING_2_BYTE     = type_data_pack(0b100, 0b00, 0b001);
constexpr u8 STRING_4_BYTE     = type_data_pack(0b100, 0b00, 0b010);
constexpr u8 STRING_8_BYTE     = type_data_pack(0b100, 0b00, 0b011);

constexpr u8 BYTE_ARRAY_1_BYTE = type_data_pack(0b101, 0b00, 0b000);
constexpr u8 BYTE_ARRAY_2_BYTE = type_data_pack(0b101, 0b00, 0b001);
constexpr u8 BYTE_ARRAY_4_BYTE = type_data_pack(0b101, 0b00, 0b010);
constexpr u8 BYTE_ARRAY_8_BYTE = type_data_pack(0b101, 0b00, 0b011);

constexpr u8 ARRAY_1_BYTE      = type_data_pack(0b101, 0b01, 0b000);
constexpr u8 ARRAY_2_BYTE      = type_data_pack(0b101, 0b01, 0b001);
constexpr u8 ARRAY_4_BYTE      = type_data_pack(0b101, 0b01, 0b010);
constexpr u8 ARRAY_8_BYTE      = type_data_pack(0b101, 0b01, 0b011);

constexpr u8 OBJECT_START      = type_data_pack(0b110, 0b00, 0b000);
constexpr u8 OBJECT_END        = type_data_pack(0b110, 0b01, 0b000);

#undef type_data_pack

inline const char* get_type_name(u8 type)
{
    switch (type)
    {
        case NIL : return "nil";

        case BOOLEAN_FALSE: return "boolean";
        case BOOLEAN_TRUE: return  "boolean";

        case INTEGER_U8:  return "8 bit unsigned integer";
        case INTEGER_U16: return "16 bit unsigned integer";
        case INTEGER_U32: return "32 bit unsigned integer";
        case INTEGER_U64: return "64 bit unsigned integer";

        case INTEGER_S8:  return "8 bit signed integer";
        case INTEGER_S16: return "16 bit signed integer";
        case INTEGER_S32: return "32 bit signed integer";
        case INTEGER_S64: return "64 bit signed integer";

        case FLOAT_32: return "32 bit float";
        case FLOAT_64: return "64 bit float";

        case STRING_1_BYTE: return "string with size in 8 bits";
        case STRING_2_BYTE: return "string with size in 16 bits";
        case STRING_4_BYTE: return "string with size in 32 bits";
        case STRING_8_BYTE: return "string with size in 64 bits";
        
        case BYTE_ARRAY_1_BYTE: return "byte array with size in 8 bits";
        case BYTE_ARRAY_2_BYTE: return "byte array with size in 16 bits";
        case BYTE_ARRAY_4_BYTE: return "byte array with size in 32 bits";
        case BYTE_ARRAY_8_BYTE: return "byte array with size in 64 bits";
        
        case ARRAY_1_BYTE: return "array with size in 8 bits";
        case ARRAY_2_BYTE: return "array with size in 16 bits";
        case ARRAY_4_BYTE: return "array with size in 32 bits";
        case ARRAY_8_BYTE: return "array with size in 64 bits";

        case OBJECT_START : return "start of object";
        case OBJECT_END   : return "end of object";
    }

    gn_assert_with_message(false, "invalid type id! (byte value: %)", (u32) type);
    return "";
}

} // namespace Binary