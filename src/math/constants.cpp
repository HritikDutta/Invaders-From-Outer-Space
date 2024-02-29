#pragma once

// Linear Algebra Constants

#include "vecs/vector3.h"
#include "mats/matrix4.h"
#include "quats/quaternion.h"

const Vector3 Vector3::up      = Vector3(0.0f, 1.0f, 0.0f);
const Vector3 Vector3::down    = Vector3(0.0f, -1.0f, 0.0f);
const Vector3 Vector3::left    = Vector3(-1.0f, 0.0f, 0.0f);
const Vector3 Vector3::right   = Vector3(1.0f, 0.0f, 0.0f);
const Vector3 Vector3::forward = Vector3(0.0f, 0.0f, 1.0f);
const Vector3 Vector3::back    = Vector3(0.0f, 0.0f, -1.0f);

const Matrix4 Matrix4::identity = Matrix4(1.0f);

const Quaternion Quaternion::identity = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);