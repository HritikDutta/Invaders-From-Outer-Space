#pragma once

#include "core/types.h"
#include "containers/darray.h"
#include "containers/string.h"

namespace Json
{

struct Token
{
    enum struct Type : char
    {
        // Literals (multiple characters)
        IDENTIFIER,     // true, false, null
        INTEGER,
        FLOAT,
        STRING,

        // Punctuations (single character)
        BRACKET_OPEN  = '[',
        BRACKET_CLOSE = ']',
        BRACE_OPEN    = '{',
        BRACE_CLOSE   = '}',
        COLON         = ':',
        COMMA         = ',',

        NUM_TYPES
    };

    Type type;
    u64 index;
    String value;   // Not owned
};

bool lex(const String content, DynamicArray<Token>& tokens);

} // namespace Json