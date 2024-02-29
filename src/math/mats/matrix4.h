#pragma once

#include <xmmintrin.h>

#include "core/types.h"
#include "core/compiler_utils.h"
#include "../vecs/vector3.h"
#include "../vecs/vector4.h"

// Data is stored column-wise as that is standard
// for OpenGL and other graphics APIs and it also
// allows for easier vectorization.
union Matrix4
{
    f32     data[4][4];
    __m128  _sse[4];
    Vector4 _vector[4];

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
    Matrix4(f32 diagonal = 1.0f)
    :   _sse {
        _mm_setr_ps(diagonal, 0.0f, 0.0f, 0.0f),
        _mm_setr_ps(0.0f, diagonal, 0.0f, 0.0f),
        _mm_setr_ps(0.0f, 0.0f, diagonal, 0.0f),
        _mm_setr_ps(0.0f, 0.0f, 0.0f, diagonal)
    }
    {
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
    Matrix4(f32 d1, f32 d2, f32 d3, f32 d4 = 1.0f)
    :   _sse {
        _mm_setr_ps(d1, 0.0f, 0.0f, 0.0f),
        _mm_setr_ps(0.0f, d2, 0.0f, 0.0f),
        _mm_setr_ps(0.0f, 0.0f, d3, 0.0f),
        _mm_setr_ps(0.0f, 0.0f, 0.0f, d4)
    }
    {
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
    Matrix4(__m128 c0, __m128 c1, __m128 c2, __m128 c3)
    :   _sse { c0, c1, c2, c3 }
    {
    }

    // Transformation Matrices
    static GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE
    Matrix4 Translation(const Vector3& displacement)
    {
        return Matrix4(
            _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f),
            _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f),
            _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f),
            _mm_setr_ps(displacement.x, displacement.y, displacement.z, 1.0f)
        );
    }

    static GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Matrix4 Rotation(Vector3 axis, f32 angle)
    {
        axis = normalize(axis);

        f32 st = Math::sin(angle);
        f32 ct = Math::cos(angle);
        f32 _1_ct = 1.0f - ct;

        __m128 c0 = _mm_setr_ps((
            axis.x * axis.x * _1_ct) + ct,
            (axis.x * axis.y * _1_ct) + axis.z * st,
            (axis.x * axis.z * _1_ct) - axis.y * st,
            0.0f
        );

        __m128 c1 = _mm_setr_ps(
            (axis.y * axis.x * _1_ct) - axis.z * st,
            (axis.y * axis.y * _1_ct) + ct,
            (axis.y * axis.z * _1_ct) + axis.x * st,
            0.0f
        );

        __m128 c2 = _mm_setr_ps(
            (axis.z * axis.x * _1_ct) + axis.y * st,
            (axis.z * axis.y * _1_ct) - axis.x * st,
            (axis.z * axis.z * _1_ct) + ct,
            0.0f
        );

        __m128 c3 = _mm_setr_ps(
            0.0f, 0.0f, 0.0f, 1.0f
        );

        return Matrix4(c0, c1, c2, c3);
    }

    static GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Matrix4 Scaling(f32 scale)
    {
        return Matrix4(scale, scale, scale);
    }

    static GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Matrix4 Scaling(const Vector3& scale)
    {
        return Matrix4(scale.x, scale.y, scale.z);
    }

    // Projection Matrices

    static GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Matrix4 Perspective(f32 fov, f32 aspectRatio, f32 near, f32 far)
    {
        Matrix4 m(0.0f);

        f32 cotangent = 1.0f / tanf(fov / 2.0f);

        m.data[0][0] = cotangent / aspectRatio;
        m.data[1][1] = cotangent;
        m.data[2][2] = (near + far) / (near - far);
        m.data[2][3] = -1.0f;
        m.data[3][2] = (2.0f * near * far) / (near - far);

        return m;
    }

    static GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Matrix4 Orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far)
    {
        Matrix4 m(1.0f);

        m.data[0][0] = 2.0f / (right - left);
        m.data[1][1] = 2.0f / (top - bottom);
        m.data[2][2] = 2.0f / (near - far);
        m.data[3][3] = 1.0f;

        m.data[3][0] = (left + right) / (left - right);
        m.data[3][1] = (bottom + top) / (bottom - top);
        m.data[3][2] = (far + near) / (near - far);

        return m;
    }

    // Camera View Transformation

    static GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Matrix4 LookAt(Vector3 eye, Vector3 center, Vector3 up)
    {
        Vector3 f = normalize(center - eye);
        Vector3 s = normalize(cross(f, up));
        Vector3 u = cross(s, f);

        __m128 c1 = _mm_setr_ps(s.x, u.x, -f.x, 0.0f);
        __m128 c2 = _mm_setr_ps(s.y, u.y, -f.y, 0.0f);
        __m128 c3 = _mm_setr_ps(s.z, u.z, -f.z, 0.0f);
        __m128 c4 = _mm_setr_ps(-dot(s, eye), -dot(u, eye), dot(f, eye), 1.0f);

        return Matrix4(c1, c2, c3, c4);
    }

    // Tranforming self
    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Matrix4& Translate(const Vector3& displacement)
    {
        *this *= Translation(displacement);
        return *this;
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Matrix4& Rotate(const Vector3& axis, f32 angle)
    {
        *this *= Rotation(axis, angle);
        return *this;
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Matrix4& Scale(const Vector3& scale)
    {
        *this *= Scaling(scale);
        return *this;
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Matrix4& Scale(f32 scale)
    {
        *this *= Scaling(scale);
        return *this;
    }

    // Matrix Operations
    
    // Extremely Complicated. DO NOT RECOMMEND...
    // There's probably a more straightforward way of doing this.
    f32 Determinant() const
    {
        /*
        
        Working out how determinants could work with SSE.

        mat =
        a0, b0, c0, d0;
        a1, b1, c1, d1;
        a2, b2, c2, d2;
        a3, b3, c3, d3;

        b1, c1, d1;
        b2, c2, d2;
        b3, c3, d3;
        det0 = b1 * (c2 * d3 - c3 * d2)
             - c1 * (b2 * d3 - b3 * d2)
             + d1 * (b2 * c3 - b3 * c2);

        det0 = (b1 * c2 * d3) - (b1 * c3 * d2) - (c1 * b2 * d3) + (c1 * b3 * d2) + (d1 * b2 * c3) - (d1 * b3 * c2);

        a1, c1, d1;
        a2, c2, d2;
        a3, c3, d3;
        det1 = a1 * (c2 * d3 - c3 * d2)
             - c1 * (a2 * d3 - a3 * d2)
             + d1 * (a2 * c3 - a3 * c2);
        det1 = (a1 * c2 * d3) - (a1 * c3 * d2) - (c1 * a2 * d3) + (c1 * a3 * d2) + (d1 * a2 * c3) - (d1 * a3 * c2);

        a1, b1, d1;
        a2, b2, d2;
        a3, b3, d3;
        det2 = a1 * (b2 * d3 - b3 * d2)
             - b1 * (a2 * d3 - a3 * d2)
             + d1 * (a2 * b3 - a3 * b2);
        det2 = (a1 * b2 * d3) - (a1 * b3 * d2) - (b1 * a2 * d3) + (b1 * a3 * d2) + (d1 * a2 * b3) - (d1 * a3 * b2);

        a1, b1, c1;
        a2, b2, c2;
        a3, b3, c3;
        det3 = a1 * (b2 * c3 - b3 * c2)
             - b1 * (a2 * c3 - a3 * c2)
             + c1 * (a2 * b3 - a3 * b2);
        det3 = (a1 * b2 * c3) - (a1 * b3 * c2) - (b1 * a2 * c3) + (b1 * a3 * c2) + (c1 * a2 * b3) - (c1 * a3 * b2);

        det  = (a0 * b1 * c2 * d3) - (a0 * b1 * c3 * d2) - (a0 * c1 * b2 * d3) + (a0 * c1 * b3 * d2) + (a0 * d1 * b2 * c3) - (a0 * d1 * b3 * c2)
             - (b0 * a1 * c2 * d3) + (b0 * a1 * c3 * d2) + (b0 * c1 * a2 * d3) - (b0 * c1 * a3 * d2) - (b0 * d1 * a2 * c3) + (b0 * d1 * a3 * c2)
             + (c0 * a1 * b2 * d3) - (c0 * a1 * b3 * d2) - (c0 * b1 * a2 * d3) + (c0 * b1 * a3 * d2) + (c0 * d1 * a2 * b3) - (c0 * d1 * a3 * b2)
             - (d0 * a1 * b2 * c3) + (d0 * a1 * b3 * c2) + (d0 * b1 * a2 * c3) - (d0 * b1 * a3 * c2) - (d0 * c1 * a2 * b3) + (d0 * c1 * a3 * b2);

        Add all of them together (In that way ->)
        +1, -1, -1, +1, | +1, -1, -1, +1, | +1, -1, -1, +1, | +1, -1, -1, +1, | +1, -1, -1, +1, | +1, -1, -1, +1
        a0, a0, a0, a0, | a0, a0, a1, a1, | a2, a3, a2, a3, | a1, a1, a2, a3, | a2, a3, a1, a1, | a2, a3, a2, a3
        b1, b1, b2, b3, | b2, b3, b0, b0, | b0, b0, b0, b0, | b2, b3, b1, b1, | b3, b2, b2, b3, | b1, b1, b3, b2
        c2, c3, c1, c1, | c3, c2, c2, c3, | c1, c1, c3, c2, | c0, c0, c0, c0, | c0, c0, c3, c2, | c3, c2, c1, c1
        d3, d2, d3, d2, | d1, d1, d3, d2, | d3, d2, d1, d1, | d3, d2, d3, d2, | d1, d1, d0, d0, | d0, d0, d0, d0

        Ordering them according to sign
                           Positive Terms                   |                   Negative Terms
        a0, a0, a1, a0, | a2, a3, a1, a3, | a2, a1, a2, a3, | a0, a1, a0, a0, | a3, a2, a1, a2, | a3, a1, a3, a2
        b1, b3, b0, b2, | b0, b0, b2, b1, | b3, b3, b1, b2, | b3, b0, b1, b2, | b0, b0, b3, b1, | b2, b2, b1, b3
        c2, c1, c3, c3, | c1, c2, c0, c0, | c0, c2, c3, c1, | c2, c2, c3, c1, | c1, c3, c0, c0, | c0, c3, c2, c1
        d3, d2, d2, d1, | d3, d1, d3, d2, | d1, d0, d0, d0, | d1, d3, d2, d3, | d2, d1, d2, d3, | d1, d0, d0, d0

        */

        __m128 pos, neg;

        // Doing them in batches of 4 cause SSE

        // Positive Terms

        {   // First Batch
            pos = _mm_mul_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(_sse[0], _sse[0], 0b00000100),       // a0, a0, a1, a0
                    _mm_shuffle_ps(_sse[1], _sse[1], 0b01110010)        // b1, b3, b0, b2
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(_sse[2], _sse[2], 0b10011111),       // c2, c1, c3, c3
                    _mm_shuffle_ps(_sse[3], _sse[3], 0b11101001)        // d3, d2, d2, d1
                )
            );
        }

        {   // Second Batch
            pos = _mm_add_ps(
                pos,
                _mm_mul_ps(
                    _mm_mul_ps(
                        _mm_shuffle_ps(_sse[0], _sse[0], 0b10110111),   // a2, a3, a1, a3
                        _mm_shuffle_ps(_sse[1], _sse[1], 0b00001001)    // b0, b0, b2, b1
                    ),
                    _mm_mul_ps(
                        _mm_shuffle_ps(_sse[2], _sse[2], 0b01100000),   // c1, c2, c0, c0
                        _mm_shuffle_ps(_sse[3], _sse[3], 0b11011110)    // d3, d1, d3, d2
                    )
                )
            );
        }

        {   // Third Batch
            pos = _mm_add_ps(
                pos,
                _mm_mul_ps(
                    _mm_mul_ps(
                        _mm_shuffle_ps(_sse[0], _sse[0], 0b10011011),   // a2, a1, a2, a3
                        _mm_shuffle_ps(_sse[1], _sse[1], 0b11110110)    // b3, b3, b1, b2
                    ),
                    _mm_mul_ps(
                        _mm_shuffle_ps(_sse[2], _sse[2], 0b00101101),   // c0, c2, c3, c1
                        _mm_shuffle_ps(_sse[3], _sse[3], 0b01000000)    // d1, d0, d0, d0
                    )
                )
            );
        }

        // Negative Terms

        {   // First Batch
            neg = _mm_mul_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(_sse[0], _sse[0], 0b00010000),       // a0, a1, a0, a0
                    _mm_shuffle_ps(_sse[1], _sse[1], 0b11000110)        // b3, b0, b1, b2
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(_sse[2], _sse[2], 0b10101101),       // c2, c2, c3, c1
                    _mm_shuffle_ps(_sse[3], _sse[3], 0b01111011)        // d1, d3, d2, d3
                )
            );
        }

        {   // Second Batch
            neg = _mm_add_ps(
                neg,
                _mm_mul_ps(
                    _mm_mul_ps(
                        _mm_shuffle_ps(_sse[0], _sse[0], 0b11100110),   // a3, a2, a1, a2
                        _mm_shuffle_ps(_sse[1], _sse[1], 0b00001101)    // b0, b0, b3, b1
                    ),
                    _mm_mul_ps(
                        _mm_shuffle_ps(_sse[2], _sse[2], 0b01110000),   // c1, c3, c0, c0
                        _mm_shuffle_ps(_sse[3], _sse[3], 0b10011011)    // d2, d1, d2, d3
                    )
                )
            );
        }

        {   // Third Batch
            neg = _mm_add_ps(
                neg,
                _mm_mul_ps(
                    _mm_mul_ps(
                        _mm_shuffle_ps(_sse[0], _sse[0], 0b11011110),   // a3, a1, a3, a2
                        _mm_shuffle_ps(_sse[1], _sse[1], 0b10100111)    // b2, b2, b1, b3
                    ),
                    _mm_mul_ps(
                        _mm_shuffle_ps(_sse[2], _sse[2], 0b00111001),   // c0, c3, c2, c1
                        _mm_shuffle_ps(_sse[3], _sse[3], 0b01000000)    // d1, d0, d0, d0
                    )
                )
            );
        }

        /*  Logic for reducing the expression
            [p0 p1 p2 p3] hadd [n0 n1 n2 n3]
            [p0+p1 p2+p3 n0+n1 n2+n3] hadd [p0+p1 p2+p3 n0+n1 n2+n3]
            [p0+p1+p2+p3 n0+n1+n2+n3 p0+p1+p2+p3 n0+n1+n2+n3] = [p n p n]
            [p n p n] hsub [p n p n]
            [p-n p-n p-n p-n]
        */

       __m128 hsum = _mm_hadd_ps(pos, neg);
       hsum = _mm_hadd_ps(hsum, hsum);

       return _mm_cvtss_f32(
           _mm_hsub_ps(hsum, hsum)
       );
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Matrix4 Transpose() const
    {
        return Matrix4(
            _mm_setr_ps(data[0][0], data[1][0], data[2][0], data[3][0]),
            _mm_setr_ps(data[0][1], data[1][1], data[2][1], data[3][1]),
            _mm_setr_ps(data[0][2], data[1][2], data[2][2], data[3][2]),
            _mm_setr_ps(data[0][3], data[1][3], data[2][3], data[3][3])
        );
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Matrix4 Inverse() const
    {
        f32 det = Determinant();

        if (det == 0.0f)
            return identity;

        return Transpose() / det;
    }    

    // Comparative Operators
    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE bool operator==(const Matrix4& rhs) const
    {
        return (_mm_movemask_ps(_mm_cmpeq_ps(_sse[0], rhs._sse[0])) == 0xF) &&
               (_mm_movemask_ps(_mm_cmpeq_ps(_sse[1], rhs._sse[1])) == 0xF) &&
               (_mm_movemask_ps(_mm_cmpeq_ps(_sse[2], rhs._sse[2])) == 0xF) &&
               (_mm_movemask_ps(_mm_cmpeq_ps(_sse[3], rhs._sse[3])) == 0xF);
    }

    
    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE bool operator!=(const Matrix4& rhs) const
    {
        return (_mm_movemask_ps(_mm_cmpeq_ps(_sse[0], rhs._sse[0])) != 0xF) ||
               (_mm_movemask_ps(_mm_cmpeq_ps(_sse[1], rhs._sse[1])) != 0xF) ||
               (_mm_movemask_ps(_mm_cmpeq_ps(_sse[2], rhs._sse[2])) != 0xF) ||
               (_mm_movemask_ps(_mm_cmpeq_ps(_sse[3], rhs._sse[3])) != 0xF);
    }

    // Arithmetic Operators
    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Matrix4 operator+(const Matrix4& rhs) const
    {
        return Matrix4(
            _mm_add_ps(_sse[0], rhs._sse[0]),
            _mm_add_ps(_sse[1], rhs._sse[1]),
            _mm_add_ps(_sse[2], rhs._sse[2]),
            _mm_add_ps(_sse[3], rhs._sse[3])
        );
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Matrix4 operator-(const Matrix4& rhs) const
    {
        return Matrix4(
            _mm_sub_ps(_sse[0], rhs._sse[0]),
            _mm_sub_ps(_sse[1], rhs._sse[1]),
            _mm_sub_ps(_sse[2], rhs._sse[2]),
            _mm_sub_ps(_sse[3], rhs._sse[3])
        );
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE __m128 operator*(const __m128& rhs) const
    {
        return _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_sse[0], _mm_shuffle_ps(rhs, rhs, SSE::SHUFFLE_MASK_V4_Xs)),
                _mm_mul_ps(_sse[1], _mm_shuffle_ps(rhs, rhs, SSE::SHUFFLE_MASK_V4_Ys))
            ),
            _mm_add_ps(
                _mm_mul_ps(_sse[2], _mm_shuffle_ps(rhs, rhs, SSE::SHUFFLE_MASK_V4_Zs)),
                _mm_mul_ps(_sse[3], _mm_shuffle_ps(rhs, rhs, SSE::SHUFFLE_MASK_V4_Ws))
            )
        );
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Vector3 operator*(const Vector3& rhs) const
    {
        return _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_sse[0], _mm_shuffle_ps(rhs._sse, rhs._sse, SSE::SHUFFLE_MASK_V4_Xs)),
                _mm_mul_ps(_sse[1], _mm_shuffle_ps(rhs._sse, rhs._sse, SSE::SHUFFLE_MASK_V4_Ys))
            ),
            _mm_add_ps(
                _mm_mul_ps(_sse[2], _mm_shuffle_ps(rhs._sse, rhs._sse, SSE::SHUFFLE_MASK_V4_Zs)),
                _mm_mul_ps(_sse[3], _mm_set1_ps(1.0f))
            )
        );
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Matrix4 operator*(const Matrix4& rhs) const
    {
        return Matrix4(
            *this * rhs._sse[0],
            *this * rhs._sse[1],
            *this * rhs._sse[2],
            *this * rhs._sse[3]
        );
    }

    // op= Operators
    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Matrix4& operator+=(const Matrix4& rhs)
    {
        _sse[0] = _mm_add_ps(_sse[0], rhs._sse[0]);
        _sse[1] = _mm_add_ps(_sse[1], rhs._sse[1]);
        _sse[2] = _mm_add_ps(_sse[2], rhs._sse[2]);
        _sse[3] = _mm_add_ps(_sse[3], rhs._sse[3]);
        return *this;
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Matrix4& operator-=(const Matrix4& rhs)
    {
        _sse[0] = _mm_sub_ps(_sse[0], rhs._sse[0]);
        _sse[1] = _mm_sub_ps(_sse[1], rhs._sse[1]);
        _sse[2] = _mm_sub_ps(_sse[2], rhs._sse[2]);
        _sse[3] = _mm_sub_ps(_sse[3], rhs._sse[3]);
        return *this;
    }

    // A *= B means B * A; This way makes more sense to me
    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Matrix4& operator*=(const Matrix4& rhs)
    {
        *this = rhs * (*this);
        return *this;
    }

    // Arithmetic Operators with Scalars
    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Matrix4 operator*(f32 scalar) const
    {
        __m128 scalarSse = _mm_set1_ps(scalar);

        return Matrix4(
            _mm_mul_ps(_sse[0], scalarSse),
            _mm_mul_ps(_sse[1], scalarSse),
            _mm_mul_ps(_sse[2], scalarSse),
            _mm_mul_ps(_sse[3], scalarSse)
        );
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Matrix4 operator/(f32 scalar) const
    {
        __m128 scalarSse = _mm_set1_ps(scalar);
        
        return Matrix4(
            _mm_div_ps(_sse[0], scalarSse),
            _mm_div_ps(_sse[1], scalarSse),
            _mm_div_ps(_sse[2], scalarSse),
            _mm_div_ps(_sse[3], scalarSse)
        );
    }

    // op= Operators with a Scalar
    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Matrix4& operator*=(f32 scalar)
    {
        __m128 scalarSse = _mm_set1_ps(scalar);
        _sse[0] = _mm_mul_ps(_sse[0], scalarSse);
        _sse[1] = _mm_mul_ps(_sse[1], scalarSse);
        _sse[2] = _mm_mul_ps(_sse[2], scalarSse);
        _sse[3] = _mm_mul_ps(_sse[3], scalarSse);
        return *this;
    }

    GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Matrix4& operator/=(f32 scalar)
    {
        __m128 scalarSse = _mm_set1_ps(scalar);
        _sse[0] = _mm_div_ps(_sse[0], scalarSse);
        _sse[1] = _mm_div_ps(_sse[1], scalarSse);
        _sse[2] = _mm_div_ps(_sse[2], scalarSse);
        _sse[3] = _mm_div_ps(_sse[3], scalarSse);
        return *this;
    }

    // Constants
    static const Matrix4 identity;
};

GN_DISABLE_SECURITY_COOKIE_CHECK GN_FORCE_INLINE Matrix4 operator*(f32 scalar, const Matrix4& mat)
{
    __m128 scalarSse = _mm_set1_ps(scalar);

    return Matrix4(
        _mm_mul_ps(mat._sse[0], scalarSse),
        _mm_mul_ps(mat._sse[1], scalarSse),
        _mm_mul_ps(mat._sse[2], scalarSse),
        _mm_mul_ps(mat._sse[3], scalarSse)
    );
}