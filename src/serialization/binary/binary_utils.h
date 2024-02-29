#pragma once

#include "core/types.h"
#include "containers/darray.h"
#include "containers/string.h"
#include "binary_types.h"

namespace Binary
{

static inline void append_integer(DynamicArray<u8>& bytes, const s8 val)
{
    const u8* as_bytes = (u8*) &val;
    append(bytes, as_bytes[0]);
}

static inline void append_integer(DynamicArray<u8>& bytes, const s16 val)
{
    // TODO: Think about endianness (right now it's only little endian)
    const u8* as_bytes = (u8*) &val;
    append(bytes, as_bytes[0]);
    append(bytes, as_bytes[1]);
}

static inline void append_integer(DynamicArray<u8>& bytes, const s32 val)
{
    // TODO: Think about endianness (right now it's only little endian)
    const u8* as_bytes = (u8*) &val;
    append(bytes, as_bytes[0]);
    append(bytes, as_bytes[1]);
    append(bytes, as_bytes[2]);
    append(bytes, as_bytes[3]);
}

static inline void append_integer(DynamicArray<u8>& bytes, const s64 val)
{
    // TODO: Think about endianness (right now it's only little endian)
    const u8* as_bytes = (u8*) &val;
    append(bytes, as_bytes[0]);
    append(bytes, as_bytes[1]);
    append(bytes, as_bytes[2]);
    append(bytes, as_bytes[3]);
    append(bytes, as_bytes[4]);
    append(bytes, as_bytes[5]);
    append(bytes, as_bytes[6]);
    append(bytes, as_bytes[7]);
}

static inline void append_integer(DynamicArray<u8>& bytes, const u8 val)
{
    const u8* as_bytes = (u8*) &val;
    append(bytes, as_bytes[0]);
}

static inline void append_integer(DynamicArray<u8>& bytes, const u16 val)
{
    // TODO: Think about endianness (right now it's only little endian)
    const u8* as_bytes = (u8*) &val;
    append(bytes, as_bytes[0]);
    append(bytes, as_bytes[1]);
}

static inline void append_integer(DynamicArray<u8>& bytes, const u32 val)
{
    // TODO: Think about endianness (right now it's only little endian)
    const u8* as_bytes = (u8*) &val;
    append(bytes, as_bytes[0]);
    append(bytes, as_bytes[1]);
    append(bytes, as_bytes[2]);
    append(bytes, as_bytes[3]);
}

static inline void append_integer(DynamicArray<u8>& bytes, const u64 val)
{
    // TODO: Think about endianness (right now it's only little endian)
    const u8* as_bytes = (u8*) &val;
    append(bytes, as_bytes[0]);
    append(bytes, as_bytes[1]);
    append(bytes, as_bytes[2]);
    append(bytes, as_bytes[3]);
    append(bytes, as_bytes[4]);
    append(bytes, as_bytes[5]);
    append(bytes, as_bytes[6]);
    append(bytes, as_bytes[7]);
}

static inline void append_float(DynamicArray<u8>& bytes, const f32 val)
{
    // TODO: Think about endianness (right now it's only little endian)
    const u8* as_bytes = (u8*) &val;
    append(bytes, as_bytes[0]);
    append(bytes, as_bytes[1]);
    append(bytes, as_bytes[2]);
    append(bytes, as_bytes[3]);
}

static inline void append_float(DynamicArray<u8>& bytes, const f64 val)
{
    // TODO: Think about endianness (right now it's only little endian)
    const u8* as_bytes = (u8*) &val;
    append(bytes, as_bytes[0]);
    append(bytes, as_bytes[1]);
    append(bytes, as_bytes[2]);
    append(bytes, as_bytes[3]);
    append(bytes, as_bytes[4]);
    append(bytes, as_bytes[5]);
    append(bytes, as_bytes[6]);
    append(bytes, as_bytes[7]);
}

static inline void append_string(DynamicArray<u8>& bytes, const String str)
{
    // encode string length
    if (str.size <= 0xffu)
    {
        append(bytes, STRING_1_BYTE);
        append_integer(bytes, (u8) str.size);
    }
    else if (str.size <= 0xffffu)
    {
        append(bytes, STRING_2_BYTE);
        append_integer(bytes, (u16) str.size);
    }
    else if (str.size <= 0xffffffffu)
    {
        append(bytes, STRING_4_BYTE);
        append_integer(bytes, (u32) str.size);
    }
    else // (str.size <= 0xffffffffffffffffu) // can't go bigger than this anyways
    {
        append(bytes, STRING_8_BYTE);
        append_integer(bytes, (u64) str.size);
    }

    // encode string data
    append_many(bytes, (u8*) str.data, str.size);
}

static inline void append_bytes(DynamicArray<u8>& bytes, const u8* raw_bytes, const u64 size)
{
    // encode array length
    if (size <= 0xffUi64)
    {
        append(bytes, Binary::BYTE_ARRAY_1_BYTE);
        Binary::append_integer(bytes, (u8) size);
    }
    else if (size <= 0xffffUi64)
    {
        append(bytes, Binary::BYTE_ARRAY_2_BYTE);
        Binary::append_integer(bytes, (u16) size);
    }
    else if (size <= 0xffffffffUi64)
    {
        append(bytes, Binary::BYTE_ARRAY_4_BYTE);
        Binary::append_integer(bytes, (u32) size);
    }
    else
    {
        append(bytes, Binary::BYTE_ARRAY_8_BYTE);
        Binary::append_integer(bytes, size);
    }

    // encode array data
    append_many(bytes, raw_bytes, size);
}

static inline void append_image(DynamicArray<u8>& bytes, const String name, const u8* pixels, const s32 width, const s32 height, const s32 bytes_pp)
{
    {   // Encode meta data
        append(bytes, Binary::INTEGER_S32);
        append_integer(bytes, width);

        append(bytes, Binary::INTEGER_S32);
        append_integer(bytes, height);

        append(bytes, Binary::INTEGER_S32);
        append_integer(bytes, bytes_pp);

        append_string(bytes, name);
    }

    {   // Encode pixels
        u64 data_size = width * height * bytes_pp;
        append_bytes(bytes, pixels, data_size);
    }
}

// Get next number as an unsigned int irrespective of integer signdness
static inline u64 get_next_uint(const Bytes& bytes, u64& offset)
{
    u8 byte = bytes[offset];
    u8 size = byte & 0b111;

    switch (size)
    {
        case 0b000:
        {
            u8 value = *(u8*)(bytes.data + offset + 1);
            offset += 1 + 1;
            return (u64) value;
        }

        case 0b001:
        {
            u16 value = *(u16*)(bytes.data + offset + 1);
            offset += 1 + 2;
            return (u64) value;
        }

        case 0b010:
        {
            u32 value = *(u32*)(bytes.data + offset + 1);
            offset += 1 + 4;
            return (u64) value;
        }

        case 0b011:
        {
            u64 value = *(u64*)(bytes.data + offset + 1);
            offset += 1 + 8;
            return (u64) value;
        }
    }

    gn_assert_with_message(false, "Incorrect size id for integer! (size id: %, offset: %)", size, offset);
    return 0;
}

}