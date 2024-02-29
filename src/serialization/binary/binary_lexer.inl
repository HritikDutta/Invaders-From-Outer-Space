#include "binary_lexer.h"

#include "binary_types.h"
#include "binary_utils.h"
#include "core/types.h"
#include "core/logger.h"
#include "containers/string.h"
#include "containers/bytes.h"

namespace Binary
{

template<>
inline bool get(const Bytes& bytes, u64& offset)
{
    gn_assert_with_message(offset < bytes.size, "Given offset exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

    switch (bytes[offset])
    {
        case BOOLEAN_FALSE: offset++; return false;
        case BOOLEAN_TRUE:  offset++; return true;
    }

    gn_assert_with_message(false, "Given byte doesn't correspond to a boolean! (byte type: %, offset: %)", get_type_name(bytes[offset]), offset);
    return false;
}

template<>
inline u8 get(const Bytes& bytes, u64& offset)
{
    gn_assert_with_message(offset < bytes.size, "Given offset exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

    switch (bytes[offset])
    {
        case INTEGER_U8:
        {
            gn_assert_with_message(offset + 1 < bytes.size, "Data for u8 exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            u8 value = *(u8*)(bytes.data + offset + 1);
            offset += 1 + 1; // type + size
            return (u8) value;
        }
    }

    gn_assert_with_message(false, "Given byte doesn't correspond to an 8 bit unsigned integer! (byte type: %, offset: %)", get_type_name(bytes[offset]), offset);
    return false;
}

template<>
inline u16 get(const Bytes& bytes, u64& offset)
{
    gn_assert_with_message(offset < bytes.size, "Given offset exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

    switch (bytes[offset])
    {
        case INTEGER_U16:
        {
            gn_assert_with_message(offset + 2 < bytes.size, "Data for u16 exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            u16 value = *(u16*)(bytes.data + offset + 1);
            offset += 1 + 2; // type + size
            return (u16) value;
        }
        
        case INTEGER_U8:
        {
            gn_assert_with_message(offset + 1 < bytes.size, "Data for u8 exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            u8 value = *(u8*)(bytes.data + offset + 1);
            offset += 1 + 1; // type + size
            return (u16) value;
        }
    }

    gn_assert_with_message(false, "Given byte doesn't correspond to a 16 bit unsigned integer! (byte type: %, offset: %)", get_type_name(bytes[offset]), offset);
    return 0;
}

template<>
inline u32 get(const Bytes& bytes, u64& offset)
{
    gn_assert_with_message(offset < bytes.size, "Given offset exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

    switch (bytes[offset])
    {
        case INTEGER_U32:
        {
            gn_assert_with_message(offset + 4 < bytes.size, "Data for u32 exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            u32 value = *(u32*)(bytes.data + offset + 1);
            offset += 1 + 4; // type + size
            return (u32) value;
        }

        case INTEGER_U16:
        {
            gn_assert_with_message(offset + 2 < bytes.size, "Data for u16 exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            u16 value = *(u16*)(bytes.data + offset + 1);
            offset += 1 + 2; // type + size
            return (u32) value;
        }
        
        case INTEGER_U8:
        {
            gn_assert_with_message(offset + 1 < bytes.size, "Data for u8 exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            u8 value = *(u8*)(bytes.data + offset + 1);
            offset += 1 + 1; // type + size
            return (u32) value;
        }
    }

    gn_assert_with_message(false, "Given byte doesn't correspond to a 32 bit unsigned integer! (byte type: %, offset: %)", get_type_name(bytes[offset]), offset);
    return 0;
}

template<>
inline u64 get(const Bytes& bytes, u64& offset)
{
    gn_assert_with_message(offset < bytes.size, "Given offset exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

    switch (bytes[offset])
    {
        case INTEGER_U64:
        {
            gn_assert_with_message(offset + 8 < bytes.size, "Data for u64 exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            u64 value = *(u64*)(bytes.data + offset + 1);
            offset += 1 + 8; // type + size
            return (u64) value;
        } break;

        case INTEGER_U32:
        {
            gn_assert_with_message(offset + 4 < bytes.size, "Data for u32 exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            u32 value = *(u32*)(bytes.data + offset + 1);
            offset += 1 + 4; // type + size
            return (u64) value;
        }

        case INTEGER_U16:
        {
            gn_assert_with_message(offset + 2 < bytes.size, "Data for u16 exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            u16 value = *(u16*)(bytes.data + offset + 1);
            offset += 1 + 2; // type + size
            return (u64) value;
        }
        
        case INTEGER_U8:
        {
            gn_assert_with_message(offset + 1 < bytes.size, "Data for u8 exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            u8 value = *(u8*)(bytes.data + offset + 1);
            offset += 1 + 1; // type + size
            return (u64) value;
        }
    }

    gn_assert_with_message(false, "Given byte doesn't correspond to a 64 bit unsigned integer! (byte type: %, offset: %)", get_type_name(bytes[offset]), offset);
    return 0;
}

template<>
inline s8 get(const Bytes& bytes, u64& offset)
{
    gn_assert_with_message(offset < bytes.size, "Given offset exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

    switch (bytes[offset])
    {
        case INTEGER_S8:
        {
            gn_assert_with_message(offset + 1 < bytes.size, "Data for s8 exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            s8 value = *(s8*)(bytes.data + offset + 1);
            offset += 1 + 1; // type + size
            return (s8) value;
        }
    }

    gn_assert_with_message(false, "Given byte doesn't correspond to an 8 bit signed integer! (byte type: %, offset: %)", get_type_name(bytes[offset]), offset);
    return false;
}

template<>
inline s16 get(const Bytes& bytes, u64& offset)
{
    gn_assert_with_message(offset < bytes.size, "Given offset exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

    switch (bytes[offset])
    {
        case INTEGER_S16:
        {
            gn_assert_with_message(offset + 2 < bytes.size, "Data for s16 exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            s16 value = *(s16*)(bytes.data + offset + 1);
            offset += 1 + 2; // type + size
            return (s16) value;
        }
        
        case INTEGER_S8:
        {
            gn_assert_with_message(offset + 1 < bytes.size, "Data for s8 exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            s8 value = *(s8*)(bytes.data + offset + 1);
            offset += 1 + 1; // type + size
            return (s16) value;
        }
    }

    gn_assert_with_message(false, "Given byte doesn't correspond to a 16 bit signed integer! (byte type: %, offset: %)", get_type_name(bytes[offset]), offset);
    return 0;
}

template<>
inline s32 get(const Bytes& bytes, u64& offset)
{
    gn_assert_with_message(offset < bytes.size, "Given offset exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

    switch (bytes[offset])
    {
        case INTEGER_S32:
        {
            gn_assert_with_message(offset + 4 < bytes.size, "Data for s32 exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            s32 value = *(s32*)(bytes.data + offset + 1);
            offset += 1 + 4; // type + size
            return (s32) value;
        }

        case INTEGER_S16:
        {
            gn_assert_with_message(offset + 2 < bytes.size, "Data for s16 exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            s16 value = *(s16*)(bytes.data + offset + 1);
            offset += 1 + 2; // type + size
            return (s32) value;
        }
        
        case INTEGER_S8:
        {
            gn_assert_with_message(offset + 1 < bytes.size, "Data for s8 exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            s8 value = *(s8*)(bytes.data + offset + 1);
            offset += 1 + 1; // type + size
            return (s32) value;
        }
    }

    gn_assert_with_message(false, "Given byte doesn't correspond to a 32 bit signed integer! (byte type: %, offset: %)", get_type_name(bytes[offset]), offset);
    return 0;
}

template<>
inline s64 get(const Bytes& bytes, u64& offset)
{
    gn_assert_with_message(offset < bytes.size, "Given offset exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

    switch (bytes[offset])
    {
        case INTEGER_S64:
        {
            gn_assert_with_message(offset + 8 < bytes.size, "Data for s64 exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            s64 value = *(s64*)(bytes.data + offset + 1);
            offset += 1 + 8; // type + size
            return (s64) value;
        }

        case INTEGER_S32:
        {
            gn_assert_with_message(offset + 4 < bytes.size, "Data for s32 exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            s32 value = *(s32*)(bytes.data + offset + 1);
            offset += 1 + 4; // type + size
            return (s64) value;
        }

        case INTEGER_S16:
        {
            gn_assert_with_message(offset + 2 < bytes.size, "Data for s16 exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            s16 value = *(s16*)(bytes.data + offset + 1);
            offset += 1 + 2; // type + size
            return (s64) value;
        }
        
        case INTEGER_S8:
        {
            gn_assert_with_message(offset + 1 < bytes.size, "Data for s8 exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            s8 value = *(s8*)(bytes.data + offset + 1);
            offset += 1 + 1; // type + size
            return (s64) value;
        }
    }

    gn_assert_with_message(false, "Given byte doesn't correspond to a 64 bit signed integer! (byte type: %, offset: %)", get_type_name(bytes[offset]), offset);
    return 0;
}

template<>
inline f32 get(const Bytes& bytes, u64& offset)
{
    gn_assert_with_message(offset < bytes.size, "Given offset exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

    switch (bytes[offset])
    {
        case FLOAT_32:
        {
            gn_assert_with_message(offset + 4 < bytes.size, "Data for f32 exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            f32 value = *(f32*)(bytes.data + offset + 1);
            offset += 1 + 4; // type + size
            return (f32) value;
        }
    }

    gn_assert_with_message(false, "Given byte doesn't correspond to a 32 bit float! (byte type: %, offset: %)", get_type_name(bytes[offset]), offset);
    return 0.0f;
}

template<>
inline f64 get(const Bytes& bytes, u64& offset)
{
    gn_assert_with_message(offset < bytes.size, "Given offset exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

    switch (bytes[offset])
    {
        case FLOAT_64:
        {
            gn_assert_with_message(offset + 8 < bytes.size, "Data for f64 exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            f64 value = *(f64*)(bytes.data + offset + 1);
            offset += 1 + 8; // type + size
            return (f64) value;
        }

        case FLOAT_32:
        {
            gn_assert_with_message(offset + 4 < bytes.size, "Data for f32 exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            f32 value = *(f32*)(bytes.data + offset + 1);
            offset += 1 + 4; // type + size
            return (f64) value;
        }
    }

    gn_assert_with_message(false, "Given byte doesn't correspond to a 64 bit float! (byte type: %, offset: %)", get_type_name(bytes[offset]), offset);
    return 0.0f;
}

template<>
inline String get(const Bytes& bytes, u64& offset)
{
    gn_assert_with_message(offset < bytes.size, "Given offset exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

    switch (bytes[offset])
    {
        case STRING_1_BYTE:
        {
            gn_assert_with_message(offset + 1 < bytes.size, "String length not encoded! (offset: %, array size: %)", offset, bytes.size);

            u8 size = *(bytes.data + offset + 1);
            gn_assert_with_message(offset + 1 + size < bytes.size, "String data exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            String str;
            str.data = (char*) (bytes.data + offset + 2);
            str.size = (u64) size;

            offset += 1 + 1 + size; // type + size + string_size
            return str;
        }

        case STRING_2_BYTE:
        {
            gn_assert_with_message(offset + 2 < bytes.size, "String length not encoded! (offset: %, array size: %)", offset, bytes.size);

            u16 size = *(u16*)(bytes.data + offset + 1);
            gn_assert_with_message(offset + 2 + size < bytes.size, "String data exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            String str;
            str.data = (char*) (bytes.data + offset + 3);
            str.size = (u64) size;
            
            offset += 1 + 2 + size; // type + size + string_size
            return str;
        }

        case STRING_4_BYTE:
        {
            gn_assert_with_message(offset + 4 < bytes.size, "String length not encoded! (offset: %, array size: %)", offset, bytes.size);

            u32 size = *(u32*)(bytes.data + offset + 1);
            gn_assert_with_message(offset + 4 + size < bytes.size, "String data exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            String str;
            str.data = (char*) (bytes.data + offset + 5);
            str.size = (u64) size;
            
            offset += 1 + 4 + size; // type + size + string_size
            return str;
        }

        case STRING_8_BYTE:
        {
            gn_assert_with_message(offset + 8 < bytes.size, "String length not encoded! (offset: %, array size: %)", offset, bytes.size);

            u64 size = *(u64*)(bytes.data + offset + 1);
            gn_assert_with_message(offset + 8 + size < bytes.size, "String data exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            String str;
            str.data = (char*) (bytes.data + offset + 9);
            str.size = (u64) size;

            offset += 1 + 8 + size; // type + size + string_size
            return str;
        }
    }

    gn_assert_with_message(false, "Given byte doesn't correspond to a string! (byte type: %, offset: %)", get_type_name(bytes[offset]), offset);
    return String {};
}

template<>
inline Bytes get(const Bytes& bytes, u64& offset)
{
    gn_assert_with_message(offset < bytes.size, "Given offset exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

    switch (bytes[offset])
    {
        case BYTE_ARRAY_1_BYTE:
        {
            gn_assert_with_message(offset + 1 < bytes.size, "Byte array length not encoded! (offset: %, array size: %)", offset, bytes.size);

            u8 size = *(bytes.data + offset + 1);
            gn_assert_with_message(offset + 1 + size < bytes.size, "Byte array data exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            Bytes out_bytes;
            out_bytes.data = (u8*) (bytes.data + offset + 2);
            out_bytes.size = (u64) size;

            offset += 1 + 1 + size; // type + size + array_size
            return out_bytes;
        }

        case BYTE_ARRAY_2_BYTE:
        {
            gn_assert_with_message(offset + 2 < bytes.size, "Byte array length not encoded! (offset: %, array size: %)", offset, bytes.size);

            u16 size = *(u16*)(bytes.data + offset + 1);
            gn_assert_with_message(offset + 2 + size < bytes.size, "Byte array data exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);
            
            Bytes out_bytes;
            out_bytes.data = (u8*) (bytes.data + offset + 3);
            out_bytes.size = (u64) size;
            
            offset += 1 + 2 + size; // type + size + array_size
            return out_bytes;
        }

        case BYTE_ARRAY_4_BYTE:
        {
            gn_assert_with_message(offset + 4 < bytes.size, "Byte array length not encoded! (offset: %, array size: %)", offset, bytes.size);

            u32 size = *(u32*)(bytes.data + offset + 1);
            gn_assert_with_message(offset + 4 + size < bytes.size, "Byte array data exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            Bytes out_bytes;
            out_bytes.data = (u8*) (bytes.data + offset + 5);
            out_bytes.size = (u64) size;

            offset += 1 + 4 + size; // type + size + array_size
            return out_bytes;
        }

        case BYTE_ARRAY_8_BYTE:
        {
            gn_assert_with_message(offset + 8 < bytes.size, "Byte array length not encoded! (offset: %, array size: %)", offset, bytes.size);

            u64 size = *(u64*)(bytes.data + offset + 1);
            gn_assert_with_message(offset + 8 + size < bytes.size, "Byte array data exceeds the size of byte array! (offset: %, array size: %)", offset, bytes.size);

            Bytes out_bytes;
            out_bytes.data = (u8*) (bytes.data + offset + 9);
            out_bytes.size = (u64) size;

            offset += 1 + 8 + size; // type + size + array_size
            return out_bytes;
        }
    }

    gn_assert_with_message(false, "Given byte doesn't correspond to a byte array! (byte type: %, offset: %)", get_type_name(bytes[offset]), offset);
    return Bytes {};
}

}