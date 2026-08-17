/*
 * @file engine/physics/core/cuda/prelude/CudForceKernels.inl
 * @project BLITZAR
 * @brief Shared CUDA system helper or kernel implementation fragment.
 */

__host__ __device__ Vector3 clampAcceleration(Vector3 accel, float maxAcceleration)
{
    const float accelNorm = accel.norm();
    if (accelNorm > maxAcceleration && accelNorm > 1e-12f) {
        return accel * (maxAcceleration / accelNorm);
    }
    return accel;
}

/*
 * @brief Documents the clamp softening value operation contract.
 * @param softening Input value used by this contract.
 * @param minSoftening Input value used by this contract.
 * @return BLITZAR_HD_HOST BLITZAR_HD_DEVICE float value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
BLITZAR_HD_HOST BLITZAR_HD_DEVICE float clampSofteningValue(float softening, float minSoftening)
{
    return fmaxf(softening, minSoftening);
}

/*
 * @brief Documents the clamp theta value operation contract.
 * @param theta Input value used by this contract.
 * @param minTheta Input value used by this contract.
 * @return BLITZAR_HD_HOST BLITZAR_HD_DEVICE float value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
BLITZAR_HD_HOST BLITZAR_HD_DEVICE float clampThetaValue(float theta, float minTheta)
{
    return fmaxf(theta, minTheta);
}

/*
 * @brief Documents the softened distance squared operation contract.
 * @param delta Input value used by this contract.
 * @param policy Input value used by this contract.
 * @return BLITZAR_HD_HOST BLITZAR_HD_DEVICE float value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
BLITZAR_HD_HOST BLITZAR_HD_DEVICE float softenedDistanceSquared(Vector3 delta,
                                                                ForceLawPolicy policy)
{
    return dot(delta, delta) + policy.softening * policy.softening;
}

/*
 * @brief Documents the blitzar acceleration from source operation contract.
 * @param selfPosition Input value used by this contract.
 * @param sourcePosition Input value used by this contract.
 * @param sourceMass Input value used by this contract.
 * @param policy Input value used by this contract.
 * @return BLITZAR_HD_HOST BLITZAR_HD_DEVICE Vector3 value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline Vector3
blitzarAccelerationFromSource(Vector3 selfPosition, Vector3 sourcePosition, float sourceMass,
                              ForceLawPolicy policy)
{
    const Vector3 delta = sourcePosition - selfPosition;
    const float dist2 = softenedDistanceSquared(delta, policy);
    if (dist2 <= policy.minDistance2) {
        return Vector3(0.0f, 0.0f, 0.0f);
    }
    const float invDist =
#if defined(__CUDA_ARCH__)
        rsqrtf(dist2);
#else
        1.0f / sqrtf(dist2);
#endif
    const float invDist2 = invDist * invDist;
    const float invDist3 = invDist2 * invDist;
    float shortRangeWeight = 1.0f;
    if (policy.treePmShortRangeScale > 0.0f) {
        constexpr float kInverseSqrtPi = 0.5641895835477563f;
        const float distance = 1.0f / invDist;
        const float splitScale = policy.treePmShortRangeScale;
        const float argument = 0.5f * distance / splitScale;
        shortRangeWeight =
            erfcf(argument) + distance * kInverseSqrtPi / splitScale * expf(-argument * argument);
    }
    return delta * (sourceMass * invDist3 * shortRangeWeight);
}

/*
 * @brief Documents the compute pairwise acceleration operation contract.
 * @param state Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @param idx Input value used by this contract.
 * @param policy Input value used by this contract.
 * @param maxAcceleration Input value used by this contract.
 * @return Vector3 value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__host__ __device__ inline Vector3 computePairwiseAcceleration(ParticleSoAView state,
                                                               int numParticles, int idx,
                                                               ForceLawPolicy policy,
                                                               float maxAcceleration)
{
    const Vector3 selfPos = getSoAPosition(state, idx);
    const float softening2 = policy.softening * policy.softening;
    float forceX = 0.0f;
    float forceY = 0.0f;
    float forceZ = 0.0f;

    for (int i = 0; i < numParticles; ++i) {
        if (i == idx) {
            continue;
        }
        const float otherMass = state.mass[i];
        const float otherX = state.posX[i];
        const float otherY = state.posY[i];
        const float otherZ = state.posZ[i];
        const float deltaX = otherX - selfPos.x;
        const float deltaY = otherY - selfPos.y;
        const float deltaZ = otherZ - selfPos.z;
        const float dist2 = deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ + softening2;
        if (dist2 <= policy.minDistance2) {
            continue;
        }

        const float invDist =
#if defined(__CUDA_ARCH__)
            rsqrtf(dist2);
#else
            1.0f / sqrtf(dist2);
#endif
        const float invDist2 = invDist * invDist;
        const float factor = otherMass * invDist2 * invDist;
        forceX += deltaX * factor;
        forceY += deltaY * factor;
        forceZ += deltaZ * factor;
    }

    return clampAcceleration(Vector3(forceX, forceY, forceZ), maxAcceleration);
}

/*
 * @brief Documents the sph poly6 operation contract.
 * @param r2 Input value used by this contract.
 * @param h Input value used by this contract.
 * @return float value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__device__ float sphPoly6(float r2, float h)
{
    const float h2 = h * h;
    if (r2 >= h2) {
        return 0.0f;
    }
    const float diff = h2 - r2;
    const float coeff = 315.0f / (64.0f * kPi * powf(h, 9.0f));
    return coeff * diff * diff * diff;
}

/*
 * @brief Documents the sph spiky grad operation contract.
 * @param r Input value used by this contract.
 * @param h Input value used by this contract.
 * @return float value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__device__ float sphSpikyGrad(float r, float h)
{
    if (r <= 1e-6f || r >= h) {
        return 0.0f;
    }
    const float coeff = -45.0f / (kPi * powf(h, 6.0f));
    const float diff = h - r;
    return coeff * diff * diff;
}

/*
 * @brief Documents the sph viscosity laplacian operation contract.
 * @param r Input value used by this contract.
 * @param h Input value used by this contract.
 * @return float value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__device__ float sphViscosityLaplacian(float r, float h)
{
    if (r >= h) {
        return 0.0f;
    }
    const float coeff = 45.0f / (kPi * powf(h, 6.0f));
    return coeff * (h - r);
}

/*
 * @brief Documents the publish metrics kernel operation contract.
 * @param mappedMetrics Input value used by this contract.
 * @param state Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @param stepId Input value used by this contract.
 * @param simTime Input value used by this contract.
 * @param dt Input value used by this contract.
 * @param vramUsedBytes Input value used by this contract.
 * @param vramPeakBytes Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
