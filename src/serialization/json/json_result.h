#pragma once

#include "core/types.h"
#include "containers/string.h"

namespace Json
{

inline u64 line_number(const String str, u64 index)
{
    u64 count = 1;

    for (u64 i = 0; i < index; i++)
        count += (str[i] == '\n');

    return count;
}

} // namespace Json

#ifdef GN_DEBUG
#include "core/logger.h"
#define log_error(fmt, ...) print_error("Json Error: "fmt"\n", __VA_ARGS__)
#else
#define log_error(fmt, ...)
#endif // GN_DEBUG