/*
 * @file engine/include/physics/Vector.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Public physics interfaces and data contracts for deterministic simulation kernels.
 */

#ifndef GRAVITY_ENGINE_INCLUDE_PHYSICS_VECTOR_HPP_
#define GRAVITY_ENGINE_INCLUDE_PHYSICS_VECTOR_HPP_
#include <cmath>

/*
 * @brief Defines the vector3 type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class Vector3 {
public:
    /*
     * @brief Documents the vector3 operation contract.
     * @param None This contract does not take explicit parameters.
     * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3();
    /*
     * @brief Documents the vector3 operation contract.
     * @param xValue Input value used by this contract.
     * @param yValue Input value used by this contract.
     * @param zValue Input value used by this contract.
     * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3(float xValue, float yValue, float zValue);
    float x, y, z;
    /*
     * @brief Documents the norm operation contract.
     * @param None This contract does not take explicit parameters.
     * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE float value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE float norm() const;
};

/*
 * @brief Documents the operator + operation contract.
 * @param a Input value used by this contract.
 * @param b Input value used by this contract.
 * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 operator+(Vector3 a, Vector3 b);
/*
 * @brief Documents the operator - operation contract.
 * @param a Input value used by this contract.
 * @param b Input value used by this contract.
 * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 operator-(Vector3 a, Vector3 b);
/*
 * @brief Documents the operator * operation contract.
 * @param a Input value used by this contract.
 * @param b Input value used by this contract.
 * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 operator*(Vector3 a, float b);
/*
 * @brief Documents the operator / operation contract.
 * @param a Input value used by this contract.
 * @param b Input value used by this contract.
 * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 operator/(Vector3 a, float b);
GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3& operator+=(Vector3& a, Vector3 b);
GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3& operator-=(Vector3& a, Vector3 b);
/*
 * @brief Documents the dot operation contract.
 * @param a Input value used by this contract.
 * @param b Input value used by this contract.
 * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE float value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
GRAVITY_HD_HOST GRAVITY_HD_DEVICE float dot(Vector3 a, Vector3 b);
/*
 * @brief Documents the normalize operation contract.
 * @param v Input value used by this contract.
 * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 normalize(Vector3 v);
#include "physics/Vector.inl"
#endif // GRAVITY_ENGINE_INCLUDE_PHYSICS_VECTOR_HPP_
