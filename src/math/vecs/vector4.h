#pragma once

#include <xmmintrin.h>
#include <smmintrin.h>

#include "core/types.h"
#include "core/compiler_utils.h"
#include "../sse_masks.h"
#include "../common.h"

union Vector4
{
    struct { f32 x, y, z, w; };
    struct { f32 r, g, b, a; };
    struct { f32 s, t, u, v; };
    f32 data[4];
    __m128 _sse;

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
    Vector4() = default;
    
    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
    Vector4(f32 val)
    : _sse(_mm_set1_ps(val))
    {
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
    Vector4(f32 x, f32 y, f32 z, f32 w)     
    : _sse(_mm_setr_ps(x, y, z, w))
    {
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
    Vector4(__m128 sse)
    : _sse(sse)
    {
    }
};

// Vector functions
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
f32 length(const Vector4& vec)
{
    return _mm_cvtss_f32(_mm_sqrt_ps(_mm_dp_ps(vec._sse, vec._sse, SSE::DP_SCALAR_MASK_V4)));
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
f32 sqr_length(const Vector4& vec)
{
    return _mm_cvtss_f32(_mm_dp_ps(vec._sse, vec._sse, SSE::DP_SCALAR_MASK_V4));
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector4 normalize(const Vector4& vec)
{
    const f32 len = length(vec);

    // Return original vector if length is 0
    if (len == 0.0f)
        return vec;
     
    return _mm_div_ps(vec._sse, _mm_set1_ps(len));
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
f32 dot(const Vector4& lhs, const Vector4& rhs)
{
    return _mm_cvtss_f32(_mm_dp_ps(lhs._sse, rhs._sse, SSE::DP_SCALAR_MASK_V4));
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector4 dotv(const Vector4& lhs, const Vector4& rhs)
{
    return _mm_dp_ps(lhs._sse, rhs._sse, SSE::DP_VECTOR_MASK_V4);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector4 lerp(const Vector4& a, const Vector4& b, f32 t)
{
    return _mm_add_ps(
        _mm_mul_ps(a._sse, _mm_set1_ps(1.0f - t)),
        _mm_mul_ps(b._sse, _mm_set1_ps(t))
    );
}

// Operators

// Comparative Operators
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
bool operator==(const Vector4& lhs, const Vector4& rhs)
{
    return (_mm_movemask_ps(_mm_cmpeq_ps(lhs._sse, rhs._sse)) == 0xF);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
bool operator!=(const Vector4& lhs, const Vector4& rhs)
{
    return (_mm_movemask_ps(_mm_cmpeq_ps(lhs._sse, rhs._sse)) != 0xF);
}

// Unary Operator(s?)
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector4 operator-(const Vector4& vec)
{
    return _mm_xor_ps(vec._sse, SSE::SIGN_MASK_V4);
}

// Arithmetic Operators
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector4 operator+(const Vector4& lhs, const Vector4& rhs)
{
    return _mm_add_ps(lhs._sse, rhs._sse);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector4 operator-(const Vector4& lhs, const Vector4& rhs)
{
    return _mm_sub_ps(lhs._sse, rhs._sse);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector4 operator*(const Vector4& lhs, const Vector4& rhs)
{
    return _mm_mul_ps(lhs._sse, rhs._sse);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector4 operator/(const Vector4& lhs, const Vector4& rhs)
{
    return _mm_div_ps(lhs._sse, rhs._sse);
}

// op= Operators
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector4& operator+=(Vector4& lhs, const Vector4& rhs)
{
    lhs._sse = _mm_add_ps(lhs._sse, rhs._sse);
    return lhs;
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector4& operator-=(Vector4& lhs, const Vector4& rhs)
{
    lhs._sse = _mm_sub_ps(lhs._sse, rhs._sse);
    return lhs;
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector4& operator*=(Vector4& lhs, const Vector4& rhs)
{
    lhs._sse = _mm_mul_ps(lhs._sse, rhs._sse);
    return lhs;
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector4& operator/=(Vector4& lhs, const Vector4& rhs)
{
    lhs._sse = _mm_div_ps(lhs._sse, rhs._sse);
    return lhs;
}

// Arithmetic Operators with a Scalar
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector4 operator*(const Vector4& lhs, f32 rhs)
{
    return _mm_mul_ps(lhs._sse, _mm_set1_ps(rhs));
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector4 operator*(f32 lhs, const Vector4& rhs)
{
    return _mm_mul_ps(_mm_set1_ps(lhs), rhs._sse);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector4 operator/(const Vector4& lhs, f32 rhs)
{
    return _mm_div_ps(lhs._sse, _mm_set1_ps(rhs));
}

// op= Operators with a Scalar
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector4& operator*=(Vector4& lhs, f32 rhs)
{
    lhs._sse = _mm_mul_ps(lhs._sse, _mm_set1_ps(rhs));
    return lhs;
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector4& operator/=(Vector4& lhs, f32 rhs)
{
    lhs._sse = _mm_div_ps(lhs._sse, _mm_set1_ps(rhs));
    return lhs;
}