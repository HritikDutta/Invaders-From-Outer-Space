#pragma once

#include <xmmintrin.h>
#include <cmath>

#include "core/types.h"
#include "core/compiler_utils.h"
#include "../sse_masks.h"
#include "../common.h"
#include "../vecs/vector3.h"
#include "../mats/matrix4.h"

// Basically a Vector4 with additional functionality
// The coordinates are represented as w + xi + yj + zk
union Quaternion
{
    struct { f32 w, x, y, z; };
    f32 data[4];
    __m128 _sse;

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Quaternion()
    :   _sse(_mm_setzero_ps())
    {
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Quaternion(f32 w, f32 x, f32 y, f32 z)
    :   _sse(_mm_setr_ps(w, x, y, z))
    {
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Quaternion(__m128 sse)
    :   _sse(sse)
    {
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Quaternion(const Vector3& axis, f32 angle)
    {
        f32 halfAngle = 0.5f * angle;
        Vector3 xyz = normalize(axis) * sinf(halfAngle);
        _sse = _mm_setr_ps(cosf(halfAngle), xyz.x, xyz.y, xyz.z);
    }

    // Quaternion Functions
    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE f32 Length() const
    {
        return _mm_cvtss_f32(_mm_sqrt_ps(_mm_dp_ps(_sse, _sse, SSE::DP_SCALAR_MASK_V4)));
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE f32 SqrLength() const
    {
        return _mm_cvtss_f32(_mm_dp_ps(_sse, _sse, SSE::DP_SCALAR_MASK_V4));
    }

    // Returns normalized quaternion without changing the original
    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Quaternion Normalized() const
    {
        return _mm_div_ps(_sse, _mm_sqrt_ps(_mm_dp_ps(_sse, _sse, 0xFF)));
    }

    // Changes the original quaternion
    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Quaternion& Normalize()
    {
        _sse = _mm_div_ps(_sse, _mm_sqrt_ps(_mm_dp_ps(_sse, _sse, 0xFF)));
        return *this;
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Quaternion Conjugate() const
    {
        return _mm_setr_ps(w, -x, -y, -z);
    }

    // Returns inverse of the quaternion without changing the original
    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Quaternion Inverse() const
    {
        __m128 conjugate = _mm_setr_ps(w, -x, -y, -z);
        return _mm_div_ps(conjugate, _mm_dp_ps(_sse, _sse, 0xFF));
    }

    // Changes the original quaternion
    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Quaternion& Invert()
    {
        __m128 conjugate = _mm_setr_ps(w, -x, -y, -z);
        _sse = _mm_div_ps(conjugate, _mm_dp_ps(_sse, _sse, 0xFF));
        return *this;
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Matrix4 GetMatrix4() const
    {
        return (Matrix4) *this;
    }

    // Comparative Operators
    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE bool operator==(const Quaternion& rhs) const
    {
        return (_mm_movemask_ps(_mm_cmpeq_ps(_sse, rhs._sse)) == 0xF);
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE bool operator!=(const Quaternion& rhs) const
    {
        return (_mm_movemask_ps(_mm_cmpeq_ps(_sse, rhs._sse)) != 0xF);
    }

    // Unary Operator(s?)
    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Quaternion operator-() const
    {
        return _mm_xor_ps(_sse, SSE::SIGN_MASK_V4);
    }

    // Arithmetic Operators
    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Quaternion operator+(const Quaternion& rhs) const
    {
        return _mm_add_ps(_sse, rhs._sse);
    }
    
    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Quaternion operator-(const Quaternion& rhs) const
    {
        return _mm_sub_ps(_sse, rhs._sse);
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Quaternion operator*(const Quaternion& rhs) const
    {

        /* Hamiltonian Product

        q1 = w + x1i + y1j + z1k
        q2 = w + x2i + y2j + z2k

        Solving that way ->
        q1 * q2 = (w1 * w2 - x1 * x2 - y1 * y2 - z1 * z2)       // new w
                + (w1 * x2 + x1 * w2 + y1 * z2 - z1 * y2) i     // new x
                + (w1 * y2 - x1 * z2 + y1 * w2 + z1 * x2) j     // new y
                + (w1 * z2 + x1 * y2 - y1 * x2 + z1 * w2) k     // new z
                ;

        Remember when shuffling, the __m128 registers hold data
        in the reverse order.

        */
        __m128 signs = _mm_setr_ps(0.0f, 0.0f, -0.0f, -0.0f);

        __m128 p1 = _mm_mul_ps(
            _mm_shuffle_ps(_sse, _sse, 0b00000000),
            _mm_shuffle_ps(rhs._sse, rhs._sse, 0b11100100)
        );

        p1 = _mm_add_ps(
            p1,
            _mm_xor_ps(
                _mm_shuffle_ps(signs, signs, 0b00110011),
                _mm_mul_ps(
                    _mm_shuffle_ps(_sse, _sse, 0b01010101),
                    _mm_shuffle_ps(rhs._sse, rhs._sse, 0b10110001)
                )
            )
        );

        p1 = _mm_add_ps(
            p1,
            _mm_xor_ps(
                _mm_shuffle_ps(signs, signs, 0b11000011),
                _mm_mul_ps(
                    _mm_shuffle_ps(_sse, _sse, 0b10101010),
                    _mm_shuffle_ps(rhs._sse, rhs._sse, 0b01001110)
                )
            )
        );

        p1 = _mm_add_ps(
            p1,
            _mm_xor_ps(
                _mm_shuffle_ps(signs, signs, 0b00001111),
                _mm_mul_ps(
                    _mm_shuffle_ps(_sse, _sse, 0b11111111),
                    _mm_shuffle_ps(rhs._sse, rhs._sse, 0b00011011)
                )
            )
        );

        return p1;
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Vector3 operator*(const Vector3& v)
    {
        Vector3 quatVector = Vector3 { x, y, z };
        Vector3 uv  = cross(quatVector, v);
        Vector3 uuv = cross(quatVector, uv);

        return v + ((uv * w) + uuv) * 2.0f;
    }

    // op= Operators
    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Quaternion& operator+=(const Quaternion& rhs)
    {
        _sse = _mm_add_ps(_sse, rhs._sse);
        return *this;
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Quaternion& operator-=(const Quaternion& rhs)
    {
        _sse = _mm_sub_ps(_sse, rhs._sse);
        return *this;
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Quaternion& operator*=(const Quaternion& rhs)
    {
        _sse = rhs * *this;
        return *this;
    }

    // Arithmetic Operators with a Scalar
    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Quaternion operator*(f32 scalar) const
    {
        return _mm_mul_ps(_sse, _mm_set1_ps(scalar));
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Quaternion operator/(f32 scalar) const
    {
        return _mm_div_ps(_sse, _mm_set1_ps(scalar));
    }

    // op= Operators with a Scalar
    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Quaternion& operator*=(f32 scalar)
    {
        _sse = _mm_mul_ps(_sse, _mm_set1_ps(scalar));
        return *this;
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Quaternion& operator/=(f32 scalar)
    {
        _sse = _mm_div_ps(_sse, _mm_set1_ps(scalar));
        return *this;
    }

    // Conversion Operators
    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE operator __m128() const
    {
        return _sse;
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE operator __m128()
    {
        return _sse;
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE operator Matrix4() const
    {
        Quaternion normalized = Normalized();

        f32 xx, yy, zz,
            xy, xz, yz,
            wx, wy, wz;

        xx = normalized.x * normalized.x;
        yy = normalized.y * normalized.y;
        zz = normalized.z * normalized.z;

        xy = normalized.x * normalized.y;
        xz = normalized.x * normalized.z;
        yz = normalized.y * normalized.z;

        wx = normalized.w * normalized.x;
        wy = normalized.w * normalized.y;
        wz = normalized.w * normalized.z;

        __m128 c0 = _mm_setr_ps(1.0f - 2.0f * (yy + zz), 2.0f * (xy + wz), 2.0f * (xz - wy), 0.0f);
        __m128 c1 = _mm_setr_ps(2.0f * (xy - wz), 1.0f - 2.0f * (xx + zz), 2.0f * (yz + wx), 0.0f);
        __m128 c2 = _mm_setr_ps(2.0f * (xz + wy), 2.0f * (yz - wx), 1.0f - 2.0f * (xx + yy), 0.0f);
        __m128 c3 = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f);

        return Matrix4(c0, c1, c2, c3);
    }

    // Performs Rotation in the XYZ order
    static GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Quaternion FromEuler(f32 x, f32 y, f32 z)
    {
        f32 cx = Math::cos(0.5f * x);
        f32 sx = Math::sin(0.5f * x);
        f32 cy = Math::cos(0.5f * y);
        f32 sy = Math::sin(0.5f * y);
        f32 cz = Math::cos(0.5f * z);
        f32 sz = Math::sin(0.5f * z);

        return _mm_add_ps(
            _mm_mul_ps(
                _mm_setr_ps(cx, sx, cx, cx),
                _mm_mul_ps(
                    _mm_setr_ps(cy, cy, sy, cy),
                    _mm_setr_ps(cz, cz, cz, sz)
                )
            ),
            _mm_mul_ps(
                _mm_setr_ps(-sx, cx, -sx, sx),
                _mm_mul_ps(
                    _mm_setr_ps(sy, sy, cy, sy),
                    _mm_setr_ps(sz, sz, sz, cz)
                )
            )
        );
    }

    // Indexing Operator
    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE const f32& operator[](int index) const
    {
        return data[min(index, 3)];
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE f32& operator[](int index)
    {
        return data[min(index, 3)];
    }

    // Constants

    static const Quaternion identity;
};

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE f32 Dot(const Quaternion& lhs, const Quaternion& rhs)
{
    return _mm_cvtss_f32(_mm_dp_ps(lhs._sse, rhs._sse, SSE::DP_SCALAR_MASK_V4));
}

// Arithmetic Operators with Scalar on the left
GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Quaternion operator*(f32 scalar, const Quaternion& quat)
{
    return quat * scalar;
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Quaternion Lerp(const Quaternion& a, const Quaternion& b, f32 t)
{
    return Quaternion(_mm_add_ps(
        _mm_mul_ps(a._sse, _mm_set1_ps(1.0f - t)),
        _mm_mul_ps(b._sse, _mm_set1_ps(t))
    ));
}

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Quaternion SLerp(const Quaternion& a, const Quaternion& b, f32 t)
{
    f32 cosTheta = Dot(a, b);
    f32 angle = acosf(cosTheta);

    f32 sin_1_tTheta = sinf((1.0f - t) * angle);
    f32 sin_tTheta   = sinf(t * angle);

    Quaternion left  = a * sin_1_tTheta;
    Quaternion right = b * sin_tTheta;

    return (left + right) / sinf(angle);
}