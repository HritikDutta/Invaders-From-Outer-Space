#pragma once

#include "core/types.h"
#include "math/vecs/vector4.h"

union Rect
{
    struct { f32 left, top, right, bottom; };
    Vector4 v4;
};