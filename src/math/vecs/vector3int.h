#pragma once

#include <xmmintrin.h>
#include <smmintrin.h>
#include <immintrin.h>

#include "core/types.h"
#include "core/compiler_utils.h"
#include "../sse_masks.h"
#include "../common.h"

// Vector3Int uses 128 bit SSE registers only. The 4th value is ignored.
// This should be okay for small projects though.
union Vector3Int
{
    struct { s32 x, y, z; };
    struct { s32 r, b, g; };
    s32 data[4];                // For alignment
    __m128i _sse;

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
    Vector3Int() = default;
    
    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
    Vector3Int(s32 val)
    : _sse(_mm_setr_epi32(val, val, val, 0.0f))
    {
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
    Vector3Int(s32 x, s32 y, s32 z)
    : _sse(_mm_setr_epi32(x, y, z, 0.0f))
    {
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
    Vector3Int(__m128i sse)
    : _sse(sse)
    {
    }

    static const Vector3Int up;
    static const Vector3Int down;
    static const Vector3Int left;
    static const Vector3Int right;
    static const Vector3Int forward;
    static const Vector3Int back;
};