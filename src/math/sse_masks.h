#pragma once

#include <xmmintrin.h>
#include "core/types.h"

namespace SSE
{

// This mask tells _mm_dp_ps to multiply all the corresponding
// elements, add them up, and put the result in the last element
constexpr s32 DP_SCALAR_MASK_V3 = 0b01110001;  // Or 0x71
constexpr s32 DP_VECTOR_MASK_V3 = 0b01110111;  // Or 0x77
constexpr s32 DP_SCALAR_MASK_V4 = 0b11110001;  // Or 0xF1
constexpr s32 DP_VECTOR_MASK_V4 = 0b11111111;  // Or 0xFF

// This mask is for negating all values in the vector using xor
static const __m128 SIGN_MASK_V3 = _mm_setr_ps(-0.0f, -0.0f, -0.0f, 0.0f);
static const __m128 SIGN_MASK_V4 = _mm_set1_ps(-0.0f);

// This mask is used in matrix multiplication for shuffling
// vectors to get an __m128 with only one element as all values.
constexpr u32 SHUFFLE_MASK_V4_Xs = 0b00000000; // All x's, basically 0000
constexpr u32 SHUFFLE_MASK_V4_Ys = 0b01010101; // All y's, basically 1111
constexpr u32 SHUFFLE_MASK_V4_Zs = 0b10101010; // All z's, basically 2222
constexpr u32 SHUFFLE_MASK_V4_Ws = 0b11111111; // All w's, basically 3333

} // namespace Math
