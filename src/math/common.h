#pragma once

#include <cmath>
#include <ctime>
#include "core/types.h"
#include "core/compiler_utils.h"

template<typename T>
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
constexpr T abs(const T a)
{
    return (a >= 0) ? a : -a;
}

// Max and Min Functions

template<typename T>
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
constexpr T min(const T a, const T b)
{
    return ((a < b) ? a : b);
}

template<typename T>
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
constexpr T max(const T a, const T b)
{
    return ((a > b) ? a : b);
}

template<typename T>
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
T clamp(const T x, const T min_value, const T max_value)
{
    return min(max(x, min_value), max_value);
}

template<typename T>
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
T wrap(const T x, const T min, const T max)
{
    T diff = x - min;
    T range = max - min;

    return (range + diff % range) % range + min;
}

template<>
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
f32 wrap(const f32 x, const f32 min, const f32 max)
{
    f32 diff = x - min;
    f32 range = max - min;

    return fmodf(range + fmodf(diff, range), range) + min;
}

template<>
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
f64 wrap(const f64 x, const f64 min, const f64 max)
{
    f64 diff = x - min;
    f64 range = max - min;

    return fmod(range + fmod(diff, range), range) + min;
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
f32 lerp(const f32 a, const f32 b, const f32 t)
{
    return (1.0f - t) * a + t * b;
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
f32 inv_lerp(const f32 val, const f32 a, const f32 b)
{
    return (val - a) / (b - a);
}

namespace Math
{

// Convenience Functions

template<typename T>
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
constexpr T abs(const T val)
{
    return (val < 0) ? -val : val;
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
f32 almost_equals(f32 a, f32 b, f32 epsilon = 0.001f)
{
    return abs(a - b) <= epsilon;
}

// Math Functions

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
f32 sign(f32 t)
{
    return __signbitvaluef(t);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
f32 sin(f32 t)
{
    return sinf(t);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
f32 cos(f32 t)
{
    return cosf(t);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
f32 tan(f32 t)
{
    return tanf(t);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
f32 exp(f32 x)
{
    return expf(x);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
f32 log(f32 x)
{
    return logf(x);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
f32 floor(f32 x)
{
    return floorf(x);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
f32 ceil(f32 x)
{
    return ceilf(x);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
f32 sqrt(f32 x)
{
    return sqrtf(x);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
f32 pow(f32 x, f32 exponent)
{
    return powf(x, exponent);
}

// Extra Functions

// Gives a random float in the range [0, 1)
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
f32 random()
{
    return rand() / (f32) RAND_MAX;
}

} // namespace Math