#pragma once

#include <cstdlib>
#include "core/types.h"
#include "darray.h"
#include "string.h"
#include "platform/platform.h"

using StringBuilder = DynamicArray<String>;

inline String build_string(const StringBuilder& builder)
{
    u64 total_size = 0;
    for (u64 i = 0; i < builder.size; i++)
        total_size += builder[i].size;
    
    // Initialize new string
    String str;
    str.size = total_size;
    str.data = (char*) platform_allocate(str.size * sizeof(char));

    // Copy strings to new string
    u64 offset = 0;
    for (u64 i = 0; i < builder.size; i++)
    {
        platform_copy_memory(str.data + offset, builder[i].data, builder[i].size * sizeof(char));
        offset += builder[i].size;
    }

    return str;
}