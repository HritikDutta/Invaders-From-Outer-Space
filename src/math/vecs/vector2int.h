#pragma once

#include "core/types.h"
#include "core/compiler_utils.h"
#include "../common.h"

union Vector2Int
{
    struct { s32 x, y; };
    struct { s32 u, v; };
    s32 data[2];

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
    Vector2Int() = default;

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
    Vector2Int(s32 val)
    : x(val), y(val)
    {
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
    Vector2Int(s32 x, s32 y)
    : x(x), y(y)
    {
    }
};

// Vector Functions
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
f32 length(const Vector2Int& vec)
{
    return Math::sqrt(vec.x * vec.x + vec.y * vec.y);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
s32 sqr_length(const Vector2Int& vec)
{
    return (vec.x * vec.x + vec.y * vec.y);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
s32 dot(const Vector2Int& lhs, const Vector2Int& rhs)
{
    return lhs.x * rhs.x + lhs.y * rhs.y;
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
s32 cross(const Vector2Int& lhs, const Vector2Int& rhs)
{
    return lhs.x * rhs.y - lhs.y * rhs.x;
}

// Operators

// Comparative Operators
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
bool operator==(const Vector2Int& lhs, const Vector2Int& rhs)
{
    return (lhs.x == rhs.x) && (lhs.y == rhs.y);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
bool operator!=(const Vector2Int& lhs, const Vector2Int& rhs)
{
    return (lhs.x != rhs.x) || (lhs.y != rhs.y);
}

// Unary Operator(s?)
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2Int operator-(const Vector2Int& vec)
{
    return Vector2Int { -vec.x, -vec.y };
}

// Arithmetic Operators
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2Int operator+(const Vector2Int& lhs, const Vector2Int& rhs)
{
    return Vector2Int { lhs.x + rhs.x, lhs.y + rhs.y };
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2Int operator-(const Vector2Int& lhs, const Vector2Int& rhs)
{
    return Vector2Int { lhs.x - rhs.x, lhs.y - rhs.y };
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2Int operator*(const Vector2Int& lhs, const Vector2Int& rhs)
{
    return Vector2Int { lhs.x * rhs.x, lhs.y * rhs.y };
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2Int operator/(const Vector2Int& lhs, const Vector2Int& rhs)
{
    return Vector2Int { lhs.x / rhs.x, lhs.y / rhs.y };
}

// op= Operators
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2Int& operator+=(Vector2Int& lhs, const Vector2Int& rhs)
{
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    return lhs;
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2Int& operator-=(Vector2Int& lhs, const Vector2Int& rhs)
{
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    return lhs;
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2Int& operator*=(Vector2Int& lhs, const Vector2Int& rhs)
{
    lhs.x *= rhs.x;
    lhs.y *= rhs.y;
    return lhs;
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2Int& operator/=(Vector2Int& lhs, const Vector2Int& rhs)
{
    lhs.x /= rhs.x;
    lhs.y /= rhs.y;
    return lhs;
}

// Arithmetic Operators with a Scalar
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2Int operator*(const Vector2Int& lhs, s32 rhs)
{
    return Vector2Int { lhs.x * rhs, lhs.y * rhs };
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2Int operator*(s32 lhs, const Vector2Int& rhs)
{
    return Vector2Int { lhs * rhs.x, lhs * rhs.y };
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2Int operator/(const Vector2Int& lhs, s32 rhs)
{
    return Vector2Int { lhs.x / rhs, lhs.y / rhs };
}

// op= Operators with a Scalar
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2Int& operator*=(Vector2Int& lhs, s32 rhs)
{
    lhs.x *= rhs;
    lhs.y *= rhs;
    return lhs;
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector2Int& operator/=(Vector2Int& lhs, s32 rhs)
{
    lhs.x /= rhs;
    lhs.y /= rhs;
    return lhs;
}