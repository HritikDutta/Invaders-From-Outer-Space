#pragma once

#include "core/types.h"
#include <limits>

namespace Math
{
    // Exponential
    constexpr f32 E    = 2.71828182845f;    // e

    // Angles
    constexpr f32 PI   = 3.14159265359f;    // pi
    constexpr f32 PI_2 = 1.57079632679f;    // pi/2
    
    constexpr f32 deg_to_rad = 0.01745329251f;
    constexpr f32 rad_to_deg = 57.2957795131f;

    constexpr f32 golden_ratio = 1.61803398875f;

#if defined _MSC_VER
    static const f32 infinity = __builtin_huge_valf();
#elif defined __GNUC__
    constexpr f32 infinity = __builtin_inf();
#endif

} // namespace Math
