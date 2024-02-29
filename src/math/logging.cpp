#include "core/logger.h"
#include "vecs/vector2.h"
#include "vecs/vector3.h"
#include "vecs/vector4.h"

template <>
void print_to_file(FILE* file, const Vector2& vector)
{
    print_to_file(file, "Vector2 { %, % }", vector.x, vector.y);
}

template <>
void print_to_file(FILE* file, const Vector3& vector)
{
    print_to_file(file, "Vector3 { %, %, % }", vector.x, vector.y, vector.z);
}

template <>
void print_to_file(FILE* file, const Vector4& vector)
{
    print_to_file(file, "Vector4 { %, %, %, % }", vector.x, vector.y, vector.z, vector.w);
}