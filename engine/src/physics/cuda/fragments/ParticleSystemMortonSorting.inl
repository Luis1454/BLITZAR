/*
 * @file engine/src/physics/cuda/fragments/ParticleSystemMortonSorting.inl
 * @author Luis1454
 * @project BLITZAR
 * @brief Physics and CUDA implementation for the deterministic simulation core.
 */

/*
 * Module: physics/cuda
 * Responsibility: Morton code spatial sorting and particle buffer reordering for cache coherency.
 */

#include <cuda_runtime.h>

// Expand 21-bit value by inserting 2 zeros between every bit
/*
 * @brief Documents the expand bits21 operation contract.
 * @param v Input value used by this contract.
 * @return unsigned long long value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__device__ __forceinline__ unsigned long long expandBits21(unsigned int v)
{
    v = (v | (v << 16)) & 0x030000FFu;
    v = (v | (v << 8)) & 0x0300F00Fu;
    v = (v | (v << 4)) & 0x030C30C3u;
    v = (v | (v << 2)) & 0x09249249u;
    return static_cast<unsigned long long>(v);
}

// Compute 63-bit Morton code from 3D coordinates (21 bits per axis)
/*
 * @brief Documents the morton encode63 operation contract.
 * @param x Input value used by this contract.
 * @param y Input value used by this contract.
 * @param z Input value used by this contract.
 * @return unsigned long long value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__device__ __forceinline__ unsigned long long mortonEncode63(unsigned int x, unsigned int y,
                                                             unsigned int z)
{
    return (expandBits21(x) << 2) | (expandBits21(y) << 1) | expandBits21(z);
}

// Compute Morton code for each particle based on its position
/*
 * @brief Documents the compute morton codes kernel operation contract.
 * @param state Input value used by this contract.
 * @param aabbMin Input value used by this contract.
 * @param aabbMax Input value used by this contract.
 * @param mortonKeys Input value used by this contract.
 * @param particleIndices Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void computeMortonCodesKernel(ParticleSoAView state, Vector3 aabbMin, Vector3 aabbMax,
                                         unsigned long long* mortonKeys, int* particleIndices,
                                         int numParticles)
{
    const int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= numParticles) {
        return;
    }

    // Load position
    const Vector3 pos = getSoAPosition(state, idx);

    // Compute position in AABB-normalized [0, 2^21-1] space
    const Vector3 extent = {aabbMax.x - aabbMin.x, aabbMax.y - aabbMin.y, aabbMax.z - aabbMin.z};

    const float NORM_SCALE = 2097151.0f; // 2^21 - 1

    const unsigned int xi = static_cast<unsigned int>(
        fmaxf(0.0f, fminf(NORM_SCALE, (pos.x - aabbMin.x) / extent.x * NORM_SCALE)));
    const unsigned int yi = static_cast<unsigned int>(
        fmaxf(0.0f, fminf(NORM_SCALE, (pos.y - aabbMin.y) / extent.y * NORM_SCALE)));
    const unsigned int zi = static_cast<unsigned int>(
        fmaxf(0.0f, fminf(NORM_SCALE, (pos.z - aabbMin.z) / extent.z * NORM_SCALE)));

    mortonKeys[idx] = mortonEncode63(xi, yi, zi);
    particleIndices[idx] = idx;
}

// Reorder particle SoA buffers by sorted Morton indices for spatial cache coherency
// This kernel reorders only position and velocity (which are double-buffered)
/*
 * @brief Documents the reorder particle buffers kernel operation contract.
 * @param srcState Input value used by this contract.
 * @param dstState Input value used by this contract.
 * @param sortedIndices Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void reorderParticleBuffersKernel(ParticleSoAView srcState, ParticleSoAView dstState,
                                             const int* sortedIndices, int numParticles)
{
    const int outIdx = blockIdx.x * blockDim.x + threadIdx.x;
    if (outIdx >= numParticles) {
        return;
    }

    // Read from source using sorted permutation, write sequentially to destination
    const int srcIdx = sortedIndices[outIdx];

    // Position
    dstState.posX[outIdx] = srcState.posX[srcIdx];
    dstState.posY[outIdx] = srcState.posY[srcIdx];
    dstState.posZ[outIdx] = srcState.posZ[srcIdx];

    // Velocity
    dstState.velX[outIdx] = srcState.velX[srcIdx];
    dstState.velY[outIdx] = srcState.velY[srcIdx];
    dstState.velZ[outIdx] = srcState.velZ[srcIdx];
}
