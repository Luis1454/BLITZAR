/*
 * @file engine/include/physics/Vector.inl
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Public physics interfaces and data contracts for deterministic simulation kernels.
 */

/*
 * @brief Documents the vector3 operation contract.
 * @param f Input value used by this contract.
 * @param f Input value used by this contract.
 * @param f Input value used by this contract.
 * @return BLITZAR_HD_HOST BLITZAR_HD_DEVICE Vector3:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline Vector3::Vector3() : x(0.0f), y(0.0f), z(0.0f)
{
}

/*
 * @brief Documents the vector3 operation contract.
 * @param xValue Input value used by this contract.
 * @param yValue Input value used by this contract.
 * @param zValue Input value used by this contract.
 * @return BLITZAR_HD_HOST BLITZAR_HD_DEVICE Vector3:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline Vector3::Vector3(float xValue, float yValue, float zValue)
    /*
     * @brief Documents the x operation contract.
     * @param xValue Input value used by this contract.
     * @param yValue Input value used by this contract.
     * @param zValue Input value used by this contract.
     * @return : value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    : x(xValue), y(yValue), z(zValue)
{
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline Vector3 operator+(Vector3 a, Vector3 b)
{
    return Vector3{a.x + b.x, a.y + b.y, a.z + b.z};
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline Vector3 operator-(Vector3 a, Vector3 b)
{
    return Vector3{a.x - b.x, a.y - b.y, a.z - b.z};
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline Vector3 operator*(Vector3 a, float b)
{
    return Vector3{a.x * b, a.y * b, a.z * b};
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline Vector3 operator/(Vector3 a, float b)
{
    return Vector3{a.x / b, a.y / b, a.z / b};
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline Vector3 operator*(Vector3 a, Vector3 b)
{
    return Vector3{a.x * b.x, a.y * b.y, a.z * b.z};
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline Vector3 operator/(Vector3 a, Vector3 b)
{
    return Vector3{a.x / b.x, a.y / b.y, a.z / b.z};
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline Vector3& operator+=(Vector3& a, Vector3 b)
{
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    return a;
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline Vector3& operator-=(Vector3& a, Vector3 b)
{
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    return a;
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline float dot(Vector3 a, Vector3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline float Vector3::norm() const
{
    return sqrtf(x * x + y * y + z * z);
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline Vector3 normalize(Vector3 v)
{
    const float n = v.norm();
    if (n > 1e-12f) {
        return v / n;
    }
    return Vector3{0.0f, 0.0f, 0.0f};
}
