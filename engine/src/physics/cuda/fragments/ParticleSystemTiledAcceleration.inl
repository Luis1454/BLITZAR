/*
 * Module: physics/cuda
 * Responsibility: Optimized tiled pairwise acceleration with shared memory and warp shuffles.
 */

#include <cuda_runtime.h>

// Warp shuffle reduction for single float (0xffffffff = warp mask for sm_86 32-thread warps)
__device__ __forceinline__ float warp_reduce_sum(float val) {
    for (int offset = warpSize / 2; offset > 0; offset /= 2) {
        val += __shfl_down_sync(0xffffffff, val, offset);
    }
    return val;
}

// Warp shuffle reduction for Vector3
__device__ __forceinline__ Vector3 warp_reduce_vec3(Vector3 val) {
    val.x = warp_reduce_sum(val.x);
    val.y = warp_reduce_sum(val.y);
    val.z = warp_reduce_sum(val.z);
    return val;
}

// Optimized tiled pairwise acceleration kernel with shared memory and warp shuffles
__global__ void computePairwiseAccelerationKernelTiled(
    ParticleSoAView state,
    Vector3Handle outAcceleration,
    int numParticles,
    ForceLawPolicy policy,
    float maxAcceleration
)
{
    const int targetIdx = blockIdx.x * blockDim.x + threadIdx.x;
    if (targetIdx >= numParticles) {
        return;
    }

    // Load target particle position once
    const Vector3 targetPos = getSoAPosition(state, targetIdx);
    const int threadId = threadIdx.x;
    Vector3 totalForce = {0.0f, 0.0f, 0.0f};

    // Process all particles in tiles to reduce VRAM bandwidth
    constexpr int TILE_SIZE = 256;
    for (int tileStart = 0; tileStart < numParticles; tileStart += TILE_SIZE) {
        const int tileEnd = min(tileStart + TILE_SIZE, numParticles);
        const int tileSize = tileEnd - tileStart;

        // Synchronize to ensure shared memory is ready
        __syncthreads();

        // Load a tile of particle data into shared memory
        // Each thread loads one particle from the tile, wrapping if necessary
        extern __shared__ char smemBuffer[];
        float *smem_posX = (float*)(smemBuffer);
        float *smem_posY = (float*)(smemBuffer + TILE_SIZE * sizeof(float));
        float *smem_posZ = (float*)(smemBuffer + 2 * TILE_SIZE * sizeof(float));
        float *smem_mass = (float*)(smemBuffer + 3 * TILE_SIZE * sizeof(float));

        // All threads cooperatively load the tile
        for (int i = threadId; i < tileSize; i += blockDim.x) {
            const int globalIdx = tileStart + i;
            smem_posX[i] = state.posX[globalIdx];
            smem_posY[i] = state.posY[globalIdx];
            smem_posZ[i] = state.posZ[globalIdx];
            smem_mass[i] = state.mass[globalIdx];
        }
        /// Description: Executes the __syncthreads operation.
        __syncthreads();

        // Each thread processes its target particle against all particles in this tile
        for (int i = 0; i < tileSize; ++i) {
            const int sourceIdx = tileStart + i;
            if (sourceIdx == targetIdx) {
                continue;
            }

            const Vector3 sourcePos = {smem_posX[i], smem_posY[i], smem_posZ[i]};
            const float sourceMass = smem_mass[i];

            // Compute pairwise force
            totalForce += gravityAccelerationFromSource(
                targetPos,
                sourcePos,
                sourceMass,
                policy);
        }
    }

    // Clamp and store result
    outAcceleration[targetIdx] = clampAcceleration(totalForce, maxAcceleration);
}

// Alternative: Optimized pairwise acceleration with warp-level reduction for intermediate results
__global__ void computePairwiseAccelerationKernelWarpReduced(
    ParticleSoAView state,
    Vector3Handle outAcceleration,
    int numParticles,
    ForceLawPolicy policy,
    float maxAcceleration
)
{
    const int warpIdx = blockIdx.x * blockDim.x / warpSize + threadIdx.x / warpSize;
    const int laneIdx = threadIdx.x % warpSize;
    const int warpThreadCount = warpSize;

    // Assign particles to warps (each warp processes one particle)
    const int particlePerWarp = 1;
    for (int wp = warpIdx; wp < numParticles; wp += (gridDim.x * blockDim.x / warpSize)) {
        const int targetIdx = wp;
        const Vector3 targetPos = getSoAPosition(state, targetIdx);

        // Each lane in the warp accumulates forces for the target particle
        Vector3 laneForce = {0.0f, 0.0f, 0.0f};

        // Process all source particles, stride by warpSize for coalesced memory access
        for (int srcBase = laneIdx; srcBase < numParticles; srcBase += warpSize) {
            if (srcBase != targetIdx) {
                const Vector3 sourcePos = getSoAPosition(state, srcBase);
                const float sourceMass = state.mass[srcBase];
                laneForce += gravityAccelerationFromSource(
                    targetPos,
                    sourcePos,
                    sourceMass,
                    policy);
            }
        }

        // Reduce forces across warp using shuffle
        Vector3 totalForce = warp_reduce_vec3(laneForce);

        // Only lane 0 writes the result
        if (laneIdx == 0) {
            outAcceleration[targetIdx] = clampAcceleration(totalForce, maxAcceleration);
        }
    }
}
