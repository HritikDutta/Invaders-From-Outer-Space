#pragma once

#include "core/types.h"

namespace Json
{

enum struct Type : u8
{
    NONE,
    BOOLEAN,
    INTEGER,
    FLOAT,
    STRING,
    ARRAY,
    OBJECT,

    NUM_TYPES
};

inline const char* get_enum_name(Type type)
{
    const char* names[] = {
        "NONE",
        "BOOLEAN",
        "INTEGER",
        "FLOAT",
        "STRING",
        "ARRAY",
        "OBJECT"
    };

    return names[(int) type];
}

} // namespace Jso
