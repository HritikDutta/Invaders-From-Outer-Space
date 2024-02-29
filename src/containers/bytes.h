#pragma once

#include "core/common.h"
#include "core/types.h"
#include "core/logger.h"
#include "platform/platform.h"

struct Bytes
{
    u8* data;
    u64 size;

    u8& operator[](const u64 index)
    {
        gn_assert_with_message(index < size, "Index out of bounds! (index: %, array size: %)", index, size);
        return data[index];
    }
    
    const u8& operator[](const u64 index) const
    {
        gn_assert_with_message(index < size, "Index out of bounds! (index: %, array size: %)", index, size);
        return data[index];
    }
};

inline void free(Bytes& bytes)
{
    platform_free(bytes.data);

    bytes.data = nullptr;
    bytes.size = 0;
}