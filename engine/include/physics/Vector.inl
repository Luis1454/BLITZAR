/*
 * @file engine/include/physics/Vector.inl
 * @author Luis1454
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
