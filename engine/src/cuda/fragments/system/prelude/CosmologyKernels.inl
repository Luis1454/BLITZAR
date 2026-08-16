/*
 * @file engine/src/cuda/fragments/system/prelude/CosmologyKernels.inl
 * @project BLITZAR
 * @brief Shared CUDA system helper or kernel implementation fragment.
 */

__global__ void cosmologyDriftKernel(ParticleSoAView state, const float3* momentumHalf,
                                     float driftFactor, float boxLength, ParticleSoAView out,
                                     int numParticles)
{
    const int index = blockIdx.x * blockDim.x + threadIdx.x;
    if (index >= numParticles) {
        return;
    }
    const float3 momentum = momentumHalf[index];
    float x = fmodf(state.posX[index] + momentum.x * driftFactor, boxLength);
    float y = fmodf(state.posY[index] + momentum.y * driftFactor, boxLength);
    float z = fmodf(state.posZ[index] + momentum.z * driftFactor, boxLength);
    x = x < 0.0f ? x + boxLength : x;
    y = y < 0.0f ? y + boxLength : y;
    z = z < 0.0f ? z + boxLength : z;
    setSoAPosition(out, index, Vector3(x, y, z));
    setSoAVelocity(out, index, Vector3(momentum.x, momentum.y, momentum.z));
    out.mass[index] = state.mass[index];
    out.temp[index] = state.temp[index];
    out.dens[index] = state.dens[index];
}

/*
 * @brief Documents the pack so akernel operation contract.
 * @param src Input value used by this contract.
 * @param dst Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
