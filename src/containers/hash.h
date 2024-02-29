#pragma once

#include <cstdint>
#include "core/types.h"
#include "string.h"
#include "bytes.h"

using Hash = u32;

template<typename T>
struct Hasher
{
    inline Hash operator()(T const& key);
};

constexpr Hash bytes[4] = {
    0x000000FF,
    0x0000FF00,
    0x00FF0000,
    0xFF000000
};

template <>
struct Hasher<f32>
{
    inline Hash operator()(f32 const& key) const
    {
        Hash hash = *(Hash*)(&key);
        Hash shuffled = (hash >> 16) | (hash << 16);

        return shuffled ^ 0xE8BF6CDF;
    }
};

template <>
struct Hasher<s32>
{
    inline Hash operator()(s32 const& key) const
    {
        Hash hash = *(Hash*)(&key);

        // 00 00 00 FF -> 00 FF 00 00   << 16
        // 00 00 FF 00 -> 00 00 00 FF   >> 8
        // 00 FF 00 00 -> FF 00 00 00   << 8
        // FF 00 00 00 -> 00 00 FF 00   >> 16
        
        Hash shuffled = ((hash & bytes[0]) << 16) |
                        ((hash & bytes[1]) >>  8) |
                        ((hash & bytes[2]) <<  8) |
                        ((hash & bytes[3]) >> 16);

        return shuffled;
    }
};

template <>
struct Hasher<u32>
{
    inline Hash operator()(u32 const& key) const
    {
        Hash hash = *(Hash*)(&key);

        // 00 00 00 FF -> 00 FF 00 00   << 16
        // 00 00 FF 00 -> 00 00 00 FF   >> 8
        // 00 FF 00 00 -> FF 00 00 00   << 8
        // FF 00 00 00 -> 00 00 FF 00   >> 16
        
        Hash shuffled = ((hash & bytes[0]) << 16) |
                        ((hash & bytes[1]) >>  8) |
                        ((hash & bytes[2]) <<  8) |
                        ((hash & bytes[3]) >> 16);

        return shuffled;
    }
};

template<>
struct Hasher<u64>
{
    inline Hash operator()(u64 const& key) const
    {
        Hash hash1 = (((Hash*)(&key))[0]);
        Hash hash2 = (((Hash*)(&key))[1]);

        // 00 00 00 FF -> 00 FF 00 00   << 16
        // 00 00 FF 00 -> 00 00 00 FF   >> 8
        // 00 FF 00 00 -> FF 00 00 00   << 8
        // FF 00 00 00 -> 00 00 FF 00   >> 16
        
        Hash shuffled1 = ((hash1 & bytes[0]) << 16) |
                         ((hash1 & bytes[1]) >>  8) |
                         ((hash1 & bytes[2]) <<  8) |
                         ((hash1 & bytes[3]) >> 16);

        Hash shuffled2 = ((hash2 & bytes[0]) << 16) |
                         ((hash2 & bytes[1]) >>  8) |
                         ((hash2 & bytes[2]) <<  8) |
                         ((hash2 & bytes[3]) >> 16);

        return shuffled1 ^ shuffled2;
    }
};

// Hasher for pointers (addresses)
template<>
struct Hasher<void*>
{
    inline Hash operator()(void* const& key) const
    {
        const u64 asU64 = (u64) key;
        return Hasher<u64>()(asU64);
    }
};

static inline Hash HashCharBuffer(char const* buffer, const u64 length)
{
    Hash const* ptr = (Hash*) buffer;
    u64 count = length / 4;
    u32 rem = length % 4;

    Hash hash = 0x8BDC195DF;
    while (count)
    {
        const Hash val = *ptr;
        
        hash = hash + hash * val * val * (count * count + 1);
        hash = ((hash & bytes[0]) << 16) |
               ((hash & bytes[1]) >>  8) |
               ((hash & bytes[2]) <<  8) |
               ((hash & bytes[3]) >> 16);

        count--;
        ptr++;
    }

    {   // Hash the remaining chars
        const u32 shift = (4 - rem) * 8;
        const u32 mask = (shift < 32u) ? (0xFFFFFFFF << shift) : 0u;
        const Hash val = (*ptr) & mask;
        
        hash = hash + hash * val * val;
        hash = ((hash & bytes[0]) << 16) |
               ((hash & bytes[1]) >>  8) |
               ((hash & bytes[2]) <<  8) |
               ((hash & bytes[3]) >> 16);
    }

    return hash;
}

template<>
struct Hasher<String>
{
    inline Hash operator()(String const& key) const
    {
        return HashCharBuffer(key.data, key.size);
    }
};

template<>
struct Hasher<Bytes>
{
    inline Hash operator()(Bytes const& key) const
    {
        const char* as_char = (char*) key.data;
        return HashCharBuffer(as_char, key.size);
    }
};