#pragma once

#include "core/types.h"
#include "core/logger.h"
#include "containers/bytes.h"
#include "containers/string.h"

namespace Binary
{

template <typename T>
inline T get(const Bytes& bytes, u64& offset)
{
    gn_assert_not_implemented();
}

void pretty_print(const Bytes& bytes);

} // namespace Binary

#include "binary_lexer.inl"