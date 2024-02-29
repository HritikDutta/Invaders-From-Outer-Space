#pragma once

#include "core/types.h"
#include "core/compiler_utils.h"
#include "../common.h"

union Vector2
{
    struct { f32 x, y; };
    struct { f32 u, v; };
    f32 data[2];

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
    Vector2() = default;

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
    Vector2(f32 val)
    : x(val), y(val)
    {
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
    Vector2(f32 x, f32 y)
    : x(x), y(y)
    {
    }
};

// Vector Functions
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
f32 length(const Vector2& vec)
{
    return Math::sqrt(vec.x * vec.x + vec.y * vec.y);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
f32 sqr_length(const Vector2& vec)
{
    return (vec.x * vec.x + vec.y * vec.y);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2 normalize(const Vector2& vec)
{
    const f32 len = length(vec);

    // Return original vector if length is 0
    if (len == 0.0f)
        return vec;
    
    return Vector2 { vec.x / len, vec.y / len };
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
f32 dot(const Vector2& lhs, const Vector2& rhs)
{
    return lhs.x * rhs.x + lhs.y * rhs.y;
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
f32 cross(const Vector2& lhs, const Vector2& rhs)
{
    return lhs.x * rhs.y - lhs.y * rhs.x;
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2 lerp(const Vector2& a, const Vector2& b, f32 t)
{
    const f32 one_minus_t = 1.0f - t;
    return Vector2 { one_minus_t * a.x + t * b.x, one_minus_t * a.y + t * b.y };
}

// Operators

// Comparative Operators
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
bool operator==(const Vector2& lhs, const Vector2& rhs)
{
    return (lhs.x == rhs.x) && (lhs.y == rhs.y);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
bool operator!=(const Vector2& lhs, const Vector2& rhs)
{
    return (lhs.x != rhs.x) || (lhs.y != rhs.y);
}

// Unary Operator(s?)
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2 operator-(const Vector2& vec)
{
    return Vector2 { -vec.x, -vec.y };
}

// Arithmetic Operators
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2 operator+(const Vector2& lhs, const Vector2& rhs)
{
    return Vector2 { lhs.x + rhs.x, lhs.y + rhs.y };
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2 operator-(const Vector2& lhs, const Vector2& rhs)
{
    return Vector2 { lhs.x - rhs.x, lhs.y - rhs.y };
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2 operator*(const Vector2& lhs, const Vector2& rhs)
{
    return Vector2 { lhs.x * rhs.x, lhs.y * rhs.y };
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2 operator/(const Vector2& lhs, const Vector2& rhs)
{
    return Vector2 { lhs.x / rhs.x, lhs.y / rhs.y };
}

// op= Operators
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2& operator+=(Vector2& lhs, const Vector2& rhs)
{
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    return lhs;
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2& operator-=(Vector2& lhs, const Vector2& rhs)
{
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    return lhs;
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2& operator*=(Vector2& lhs, const Vector2& rhs)
{
    lhs.x *= rhs.x;
    lhs.y *= rhs.y;
    return lhs;
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2& operator/=(Vector2& lhs, const Vector2& rhs)
{
    lhs.x /= rhs.x;
    lhs.y /= rhs.y;
    return lhs;
}

// Arithmetic Operators with a Scalar
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2 operator*(const Vector2& lhs, f32 rhs)
{
    return Vector2 { lhs.x * rhs, lhs.y * rhs };
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2 operator*(f32 lhs, const Vector2& rhs)
{
    return Vector2 { lhs * rhs.x, lhs * rhs.y };
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2 operator/(const Vector2& lhs, f32 rhs)
{
    return Vector2 { lhs.x / rhs, lhs.y / rhs };
}

// op= Operators with a Scalar
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2& operator*=(Vector2& lhs, f32 rhs)
{
    lhs.x *= rhs;
    lhs.y *= rhs;
    return lhs;
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2& operator/=(Vector2& lhs, f32 rhs)
{
    lhs.x /= rhs;
    lhs.y /= rhs;
    return lhs;
}