#pragma once

#include <xmmintrin.h>
#include <smmintrin.h>

#include "core/types.h"
#include "core/compiler_utils.h"
#include "../sse_masks.h"
#include "../common.h"

// Vector3 uses 128 bit SSE registers only. The 4th value is ignored.
// This should be okay for small projects though.
union Vector3
{
    struct { f32 x, y, z; };
    struct { f32 r, b, g; };
    f32 data[4];                // For alignment
    __m128 _sse;

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
    Vector3() = default;
    
    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
    Vector3(f32 val)
    : _sse(_mm_setr_ps(val, val, val, 0.0f))
    {
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
    Vector3(f32 x, f32 y, f32 z)
    : _sse(_mm_setr_ps(x, y, z, 0.0f))
    {
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
    Vector3(__m128 sse)
    : _sse(sse)
    {
    }

    static const Vector3 up;
    static const Vector3 down;
    static const Vector3 left;
    static const Vector3 right;
    static const Vector3 forward;
    static const Vector3 back;
};

// Vector functions
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
f32 length(const Vector3& vec)
{
    return _mm_cvtss_f32(_mm_sqrt_ps(_mm_dp_ps(vec._sse, vec._sse, SSE::DP_SCALAR_MASK_V3)));
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
f32 sqr_length(const Vector3& vec)
{
    return _mm_cvtss_f32(_mm_dp_ps(vec._sse, vec._sse, SSE::DP_SCALAR_MASK_V3));
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector3 normalize(const Vector3& vec)
{
    const f32 len = length(vec);

    // Return original vector if length is 0
    if (len == 0.0f)
        return vec;
    
    return _mm_div_ps(vec._sse, _mm_setr_ps(len, len, len, 1.0f));
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
f32 dot(const Vector3& lhs, const Vector3& rhs)
{
    return _mm_cvtss_f32(_mm_dp_ps(lhs._sse, rhs._sse, SSE::DP_SCALAR_MASK_V3));
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector3 dotv(const Vector3& lhs, const Vector3& rhs)
{
    return _mm_dp_ps(lhs._sse, rhs._sse, SSE::DP_VECTOR_MASK_V3);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector3 cross(const Vector3& lhs, const Vector3& rhs)
{
    return _mm_sub_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(lhs._sse, lhs._sse, 0b11001001),
            _mm_shuffle_ps(rhs._sse, rhs._sse, 0b11010010)
        ),
        _mm_mul_ps(
            _mm_shuffle_ps(lhs._sse, lhs._sse, 0b11010010),
            _mm_shuffle_ps(rhs._sse, rhs._sse, 0b11001001)
        )
    );
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector3 lerp(const Vector3& a, const Vector3& b, f32 t)
{
    return _mm_add_ps(
        _mm_mul_ps(a._sse, _mm_set1_ps(1.0f - t)),
        _mm_mul_ps(b._sse, _mm_set1_ps(t))
    );
}

// Operators

// Comparative Operators
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
bool operator==(const Vector3& lhs, const Vector3& rhs)
{
    return (_mm_movemask_ps(_mm_cmpeq_ps(lhs._sse, rhs._sse)) == 0x7);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
bool operator!=(const Vector3& lhs, const Vector3& rhs)
{
    return (_mm_movemask_ps(_mm_cmpeq_ps(lhs._sse, rhs._sse)) != 0x7);
}

// Unary Operator(s?)
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector3 operator-(const Vector3& vec)
{
    return _mm_xor_ps(vec._sse, SSE::SIGN_MASK_V3);
}

// Arithmetic Operators
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector3 operator+(const Vector3& lhs, const Vector3& rhs)
{
    return _mm_add_ps(lhs._sse, rhs._sse);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector3 operator-(const Vector3& lhs, const Vector3& rhs)
{
    return _mm_sub_ps(lhs._sse, rhs._sse);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector3 operator*(const Vector3& lhs, const Vector3& rhs)
{
    return _mm_mul_ps(lhs._sse, rhs._sse);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector3 operator/(const Vector3& lhs, const Vector3& rhs)
{
    return _mm_div_ps(lhs._sse, rhs._sse);
}

// op= Operators
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector3& operator+=(Vector3& lhs, const Vector3& rhs)
{
    lhs._sse = _mm_add_ps(lhs._sse, rhs._sse);
    return lhs;
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector3& operator-=(Vector3& lhs, const Vector3& rhs)
{
    lhs._sse = _mm_sub_ps(lhs._sse, rhs._sse);
    return lhs;
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector3& operator*=(Vector3& lhs, const Vector3& rhs)
{
    lhs._sse = _mm_mul_ps(lhs._sse, rhs._sse);
    return lhs;
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector3& operator/=(Vector3& lhs, const Vector3& rhs)
{
    lhs._sse = _mm_div_ps(lhs._sse, rhs._sse);
    return lhs;
}

// Arithmetic Operators with a Scalar
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector3 operator*(const Vector3& lhs, f32 rhs)
{
    return _mm_mul_ps(lhs._sse, _mm_set1_ps(rhs));
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector3 operator*(f32 lhs, const Vector3& rhs)
{
    return _mm_mul_ps(_mm_set1_ps(lhs), rhs._sse);
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector3 operator/(const Vector3& lhs, f32 rhs)
{
    return _mm_div_ps(lhs._sse, _mm_set1_ps(rhs));
}

// op= Operators with a Scalar
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector3& operator*=(Vector3& lhs, f32 rhs)
{
    lhs._sse = _mm_mul_ps(lhs._sse, _mm_set1_ps(rhs));
    return lhs;
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
Vector3& operator/=(Vector3& lhs, f32 rhs)
{
    lhs._sse = _mm_div_ps(lhs._sse, _mm_set1_ps(rhs));
    return lhs;
}