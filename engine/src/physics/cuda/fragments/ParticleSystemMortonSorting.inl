/*
 * Module: physics/cuda
 * Responsibility: Morton code spatial sorting and particle buffer reordering for cache coherency.
 */
 
 #include <cuda_runtime.h>
 
 // Expand 21-bit value by inserting 2 zeros between every bit
 __device__ __forceinline__ unsigned long long expandBits21(unsigned int v)
 {
     v = (v | (v << 16)) & 0x030000FFu;
     v = (v | (v << 8))  & 0x0300F00Fu;
     v = (v | (v << 4))  & 0x030C30C3u;
     v = (v | (v << 2))  & 0x09249249u;
     return static_cast<unsigned long long>(v);
 }
 
 // Compute 63-bit Morton code from 3D coordinates (21 bits per axis)
 __device__ __forceinline__ unsigned long long mortonEncode63(unsigned int x, unsigned int y, unsigned int z)
 {
     return (expandBits21(x) << 2) | (expandBits21(y) << 1) | expandBits21(z);
 }
 
 // Compute Morton code for each particle based on its position
__global__ void computeMortonCodesKernel(
    ParticleSoAView state,
    Vector3 aabbMin,
    Vector3 aabbMax,
    unsigned long long *mortonKeys,
    int *particleIndices,
    int numParticles
)
{
    const int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= numParticles) {
        return;
    }

    // Load position
    const Vector3 pos = getSoAPosition(state, idx);
    
    // Compute position in AABB-normalized [0, 2^21-1] space
    const Vector3 extent = {
        aabbMax.x - aabbMin.x,
        aabbMax.y - aabbMin.y,
        aabbMax.z - aabbMin.z
    };
    
    const float NORM_SCALE = 2097151.0f;  // 2^21 - 1
    
    const unsigned int xi = static_cast<unsigned int>(
        /// Description: Executes the fmaxf operation.
        fmaxf(0.0f, fminf(NORM_SCALE, (pos.x - aabbMin.x) / extent.x * NORM_SCALE))
    );
    const unsigned int yi = static_cast<unsigned int>(
        /// Description: Executes the fmaxf operation.
        fmaxf(0.0f, fminf(NORM_SCALE, (pos.y - aabbMin.y) / extent.y * NORM_SCALE))
    );
    const unsigned int zi = static_cast<unsigned int>(
        /// Description: Executes the fmaxf operation.
        fmaxf(0.0f, fminf(NORM_SCALE, (pos.z - aabbMin.z) / extent.z * NORM_SCALE))
    );
    
    mortonKeys[idx] = mortonEncode63(xi, yi, zi);
    particleIndices[idx] = idx;
}

// Reorder particle SoA buffers by sorted Morton indices for spatial cache coherency
// This kernel reorders only position and velocity (which are double-buffered)
__global__ void reorderParticleBuffersKernel(
    ParticleSoAView srcState,
    ParticleSoAView dstState,
    const int *sortedIndices,
    int numParticles
)
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
