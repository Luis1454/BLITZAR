// File: engine/include/physics/Vector.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_PHYSICS_VECTOR_HPP_
#define GRAVITY_ENGINE_INCLUDE_PHYSICS_VECTOR_HPP_
#include <cmath>

/// Description: Defines the Vector3 data or behavior contract.
class Vector3 {
public:
    /// Description: Describes the vector3 operation contract.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3();
    /// Description: Describes the vector3 operation contract.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3(float xValue, float yValue, float zValue);
    float x, y, z;
    /// Description: Describes the norm operation contract.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE float norm() const;
};

/// Description: Describes the operator+ operation contract.
GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 operator+(Vector3 a, Vector3 b);
/// Description: Describes the operator- operation contract.
GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 operator-(Vector3 a, Vector3 b);
/// Description: Describes the operator* operation contract.
GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 operator*(Vector3 a, float b);
/// Description: Describes the operator/ operation contract.
GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 operator/(Vector3 a, float b);
GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3& operator+=(Vector3& a, Vector3 b);
GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3& operator-=(Vector3& a, Vector3 b);
/// Description: Executes the dot operation.
GRAVITY_HD_HOST GRAVITY_HD_DEVICE float dot(Vector3 a, Vector3 b);
/// Description: Executes the normalize operation.
GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 normalize(Vector3 v);
#include "physics/Vector.inl"
#endif // GRAVITY_ENGINE_INCLUDE_PHYSICS_VECTOR_HPP_
