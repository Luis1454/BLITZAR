#pragma once

#include <cmath>

#define GRAVITY_HD GRAVITY_HD_HOST GRAVITY_HD_DEVICE

class Vector3 {
    public:
        GRAVITY_HD Vector3();
        GRAVITY_HD Vector3(float xValue, float yValue, float zValue);

        float x, y, z;

        GRAVITY_HD float norm() const;
};

GRAVITY_HD Vector3 operator+(Vector3 a, Vector3 b);
GRAVITY_HD Vector3 operator-(Vector3 a, Vector3 b);
GRAVITY_HD Vector3 operator*(Vector3 a, float b);
GRAVITY_HD Vector3 operator/(Vector3 a, float b);
GRAVITY_HD Vector3 &operator+=(Vector3 &a, Vector3 b);
GRAVITY_HD Vector3 &operator-=(Vector3 &a, Vector3 b);
GRAVITY_HD float dot(Vector3 a, Vector3 b);
GRAVITY_HD Vector3 normalize(Vector3 v);

#include "physics/Vector.inl"
