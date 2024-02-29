#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "core/types.h"
#include "core/common.h"
#include "core/logger.h"
#include "math/common.h"
#include "platform/platform.h"

struct String
{
    char* data;
    u64 size;
    
    char& operator[](const u64 index)
    {
        gn_assert_with_message(index < size, "Index out of bounds! (index: %, array size: %, string: %)", index, size, *this);
        return data[index];
    }

    const char& operator[](const u64 index) const
    {
        gn_assert_with_message(index < size, "Index out of bounds! (index: %, array size: %, string: %)", index, size, *this);
        return data[index];
    }
};

template<>
inline String make(Type<String>, const char* cstr)
{
    String str;

    str.size = strlen(cstr);

    const u64 data_size = (str.size + 1) * sizeof(char);
    str.data = (char*) platform_allocate(data_size);
    gn_assert_with_message(str.data, "Could not allocate data for string!");

    platform_copy_memory(str.data, cstr, data_size);

    return str;
}

template<>
inline String make(Type<String>, const char* cstr, int size)
{
    String str;

    str.size = size;

    const u64 data_size = (str.size + 1) * sizeof(char);
    str.data = (char*) platform_allocate(data_size);
    gn_assert_with_message(str.data, "Could not allocate data for string!");

    platform_copy_memory(str.data, cstr, data_size);

    return str;
}

inline String ref(char* cstr, int size)
{
    return String { cstr, (u64) size };
}

inline String ref(char* cstr, u64 size)
{
    return String { cstr, size };
}

inline String ref(char* cstr)
{
    return String { cstr, strlen(cstr) };
}

inline String copy(const String& other)
{
    String str;

    str.size = other.size;

    const u64 data_size = str.size * sizeof(char);
    str.data = (char*) platform_allocate(data_size);
    gn_assert_with_message(str.data, "Could not allocate data for string!");

    platform_copy_memory(str.data, other.data, data_size);

    return str;
}

inline void free(String& str)
{
    platform_free(str.data);

    str.data = nullptr;
    str.size = 0;
}

// Reallocates
inline void resize(String& str, u64 size)
{
    str.size = size;
    str.data = (char*) platform_reallocate(str.data, str.size * sizeof(char));
    gn_assert_with_message(str.data, "Could not reallocate data for string!");
}

inline void reverse(String& str)
{
    u64 i = 0, j = str.size - 1;
    while (i < j)
    {
        swap(str.data[i], str.data[j]);
        i++;
        j--;
    }
}

inline String get_substring(String src, u64 start = 0Ui64, u64 length = _UI64_MAX)
{
    String str;

    str.data = src.data + start;
    str.size = min(src.size - start, length);

    return str;
}

inline bool operator==(const String str1, const String str2)
{
    if (str1.size != str2.size)
        return false;
    
    {   // Check in chunks of 8 bytes
        const u64 num_iters = str1.size / sizeof(u64);
        const u64* s1 = (const u64*) str1.data;
        const u64* s2 = (const u64*) str2.data;

        for (u64 i = 0; i < num_iters; i++)
        {
            if (s1[i] != s2[i])
                return false;
        }
    }

    {   // Check remaining bytes
        const u64 num_iters = str1.size % sizeof(u64);
        for (u64 i = str1.size - num_iters; i < str1.size; i++)
        {
            if (str1.data[i] != str2.data[i])
                return false;
        }
    }

    return true;
}

inline bool operator!=(const String str1, const String str2)
{
    return !(str1 == str2);
}