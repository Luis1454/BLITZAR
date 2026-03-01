/*
** EPITECH PROJECT, 2024
** rtxcpp
** File description:
** Vector
*/

#ifndef VECTOR_HPP_
#define VECTOR_HPP_

#include <cmath>

#ifdef __CUDACC__
    #define GRAVITY_HD __host__ __device__
#else
    #define GRAVITY_HD
#endif

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

#endif /* !VECTOR_HPP_ */
