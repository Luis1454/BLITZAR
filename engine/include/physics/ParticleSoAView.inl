/*
 * @file engine/include/physics/ParticleSoAView.inl
 * @author Luis1454
 * @project BLITZAR
 * @brief Public physics interfaces and data contracts for deterministic simulation kernels.
 */

/*
 * @brief Documents the get so aposition operation contract.
 * @param view Input value used by this contract.
 * @param i Input value used by this contract.
 * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
GRAVITY_HD_HOST GRAVITY_HD_DEVICE inline Vector3 getSoAPosition(ParticleSoAView view, int i)
{
    return Vector3{view.posX[i], view.posY[i], view.posZ[i]};
}

/*
 * @brief Documents the set so aposition operation contract.
 * @param view Input value used by this contract.
 * @param i Input value used by this contract.
 * @param p Input value used by this contract.
 * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE void value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
GRAVITY_HD_HOST GRAVITY_HD_DEVICE inline void setSoAPosition(ParticleSoAView view, int i, Vector3 p)
{
    view.posX[i] = p.x;
    view.posY[i] = p.y;
    view.posZ[i] = p.z;
}

/*
 * @brief Documents the get so avelocity operation contract.
 * @param view Input value used by this contract.
 * @param i Input value used by this contract.
 * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
GRAVITY_HD_HOST GRAVITY_HD_DEVICE inline Vector3 getSoAVelocity(ParticleSoAView view, int i)
{
    return Vector3{view.velX[i], view.velY[i], view.velZ[i]};
}

/*
 * @brief Documents the set so avelocity operation contract.
 * @param view Input value used by this contract.
 * @param i Input value used by this contract.
 * @param v Input value used by this contract.
 * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE void value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
GRAVITY_HD_HOST GRAVITY_HD_DEVICE inline void setSoAVelocity(ParticleSoAView view, int i, Vector3 v)
{
    view.velX[i] = v.x;
    view.velY[i] = v.y;
    view.velZ[i] = v.z;
}

/*
 * @brief Documents the set so apressure operation contract.
 * @param view Input value used by this contract.
 * @param i Input value used by this contract.
 * @param p Input value used by this contract.
 * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE void value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
GRAVITY_HD_HOST GRAVITY_HD_DEVICE inline void setSoAPressure(ParticleSoAView view, int i, Vector3 p)
{
    view.pressX[i] = p.x;
    view.pressY[i] = p.y;
    view.pressZ[i] = p.z;
}

/*
 * @brief Documents the get so apressure operation contract.
 * @param view Input value used by this contract.
 * @param i Input value used by this contract.
 * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
GRAVITY_HD_HOST GRAVITY_HD_DEVICE inline Vector3 getSoAPressure(ParticleSoAView view, int i)
{
    return Vector3{view.pressX[i], view.pressY[i], view.pressZ[i]};
}
