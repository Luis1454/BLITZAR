/*
 * @file engine/physics/treepm/cuda/layout/TpmBounds.inl
 * @project BLITZAR
 * @brief TreePM bounds and concentration reduction kernels.
 */

namespace blitzar_cuda_tree_pm_gpu {

constexpr int kTreePmBoundsBlockSize = 256;
constexpr int kTreePmBoundsFieldCount = 10;
constexpr int kTreePmConcentrationBinCount = 32;

enum TreePmLayoutMode : int {
    kTreePmLayoutLegacy = 0,
    kTreePmLayoutAuto = 1,
    kTreePmLayoutLinear = 2,
    kTreePmLayoutGatherLinear = 3,
    kTreePmLayoutGatherMorton = 4
};

__device__ __forceinline__ void treePmCombineBounds(float& minX, float& minY, float& minZ,
                                                     float& maxX, float& maxY, float& maxZ,
                                                     float& mass, float& weightedX,
                                                     float& weightedY, float& weightedZ,
                                                     float otherMinX, float otherMinY,
                                                     float otherMinZ, float otherMaxX,
                                                     float otherMaxY, float otherMaxZ,
                                                     float otherMass, float otherWeightedX,
                                                     float otherWeightedY, float otherWeightedZ)
{
    minX = fminf(minX, otherMinX);
    minY = fminf(minY, otherMinY);
    minZ = fminf(minZ, otherMinZ);
    maxX = fmaxf(maxX, otherMaxX);
    maxY = fmaxf(maxY, otherMaxY);
    maxZ = fmaxf(maxZ, otherMaxZ);
    mass += otherMass;
    weightedX += otherWeightedX;
    weightedY += otherWeightedY;
    weightedZ += otherWeightedZ;
}

__global__ void treePmReduceBoundsKernel(ParticleSoAView state, int numParticles,
                                         float* partialBounds)
{
    __shared__ float shared[kTreePmBoundsFieldCount][kTreePmBoundsBlockSize];
    const int thread = threadIdx.x;
    const int particle = blockIdx.x * blockDim.x + thread;
    const bool valid = particle < numParticles;
    const Vector3 position = valid ? octreeLoadParticlePosition(state, particle) : Vector3{};
    const float mass = valid ? octreeLoadParticleMass(state, particle) : 0.0f;

    shared[0][thread] = valid ? position.x : FLT_MAX;
    shared[1][thread] = valid ? position.y : FLT_MAX;
    shared[2][thread] = valid ? position.z : FLT_MAX;
    shared[3][thread] = valid ? position.x : -FLT_MAX;
    shared[4][thread] = valid ? position.y : -FLT_MAX;
    shared[5][thread] = valid ? position.z : -FLT_MAX;
    shared[6][thread] = mass;
    shared[7][thread] = mass * position.x;
    shared[8][thread] = mass * position.y;
    shared[9][thread] = mass * position.z;
    __syncthreads();

    for (int stride = blockDim.x / 2; stride > 0; stride >>= 1) {
        if (thread < stride) {
            shared[0][thread] = fminf(shared[0][thread], shared[0][thread + stride]);
            shared[1][thread] = fminf(shared[1][thread], shared[1][thread + stride]);
            shared[2][thread] = fminf(shared[2][thread], shared[2][thread + stride]);
            shared[3][thread] = fmaxf(shared[3][thread], shared[3][thread + stride]);
            shared[4][thread] = fmaxf(shared[4][thread], shared[4][thread + stride]);
            shared[5][thread] = fmaxf(shared[5][thread], shared[5][thread + stride]);
            shared[6][thread] += shared[6][thread + stride];
            shared[7][thread] += shared[7][thread + stride];
            shared[8][thread] += shared[8][thread + stride];
            shared[9][thread] += shared[9][thread + stride];
        }
        __syncthreads();
    }

    if (thread == 0) {
        float* output = partialBounds + blockIdx.x * kTreePmBoundsFieldCount;
        for (int field = 0; field < kTreePmBoundsFieldCount; ++field) {
            output[field] = shared[field][0];
        }
    }
}

__global__ void treePmFinalizeBoundsKernel(const float* partialBounds, int blockCount,
                                           float* bounds)
{
    __shared__ float shared[kTreePmBoundsFieldCount][kTreePmBoundsBlockSize];
    const int thread = threadIdx.x;
    float minX = FLT_MAX;
    float minY = FLT_MAX;
    float minZ = FLT_MAX;
    float maxX = -FLT_MAX;
    float maxY = -FLT_MAX;
    float maxZ = -FLT_MAX;
    float mass = 0.0f;
    float weightedX = 0.0f;
    float weightedY = 0.0f;
    float weightedZ = 0.0f;

    for (int block = thread; block < blockCount; block += blockDim.x) {
        const float* input = partialBounds + block * kTreePmBoundsFieldCount;
        treePmCombineBounds(minX, minY, minZ, maxX, maxY, maxZ, mass, weightedX, weightedY,
                             weightedZ, input[0], input[1], input[2], input[3], input[4], input[5],
                             input[6], input[7], input[8], input[9]);
    }

    shared[0][thread] = minX;
    shared[1][thread] = minY;
    shared[2][thread] = minZ;
    shared[3][thread] = maxX;
    shared[4][thread] = maxY;
    shared[5][thread] = maxZ;
    shared[6][thread] = mass;
    shared[7][thread] = weightedX;
    shared[8][thread] = weightedY;
    shared[9][thread] = weightedZ;
    __syncthreads();

    for (int stride = blockDim.x / 2; stride > 0; stride >>= 1) {
        if (thread < stride) {
            shared[0][thread] = fminf(shared[0][thread], shared[0][thread + stride]);
            shared[1][thread] = fminf(shared[1][thread], shared[1][thread + stride]);
            shared[2][thread] = fminf(shared[2][thread], shared[2][thread + stride]);
            shared[3][thread] = fmaxf(shared[3][thread], shared[3][thread + stride]);
            shared[4][thread] = fmaxf(shared[4][thread], shared[4][thread + stride]);
            shared[5][thread] = fmaxf(shared[5][thread], shared[5][thread + stride]);
            shared[6][thread] += shared[6][thread + stride];
            shared[7][thread] += shared[7][thread + stride];
            shared[8][thread] += shared[8][thread + stride];
            shared[9][thread] += shared[9][thread + stride];
        }
        __syncthreads();
    }

    if (thread == 0) {
        for (int field = 0; field < kTreePmBoundsFieldCount; ++field) {
            bounds[field] = shared[field][0];
        }
    }
}

__global__ void treePmRadialMassHistogramKernel(ParticleSoAView state, int numParticles,
                                                 const float* bounds, float* histogram)
{
    const int particle = blockIdx.x * blockDim.x + threadIdx.x;
    if (particle >= numParticles) {
        return;
    }

    const float totalMass = bounds[6];
    if (!(totalMass > 0.0f)) {
        return;
    }
    const Vector3 position = octreeLoadParticlePosition(state, particle);
    const Vector3 center(bounds[7] / totalMass, bounds[8] / totalMass, bounds[9] / totalMass);
    const Vector3 boxCenter(0.5f * (bounds[0] + bounds[3]), 0.5f * (bounds[1] + bounds[4]),
                            0.5f * (bounds[2] + bounds[5]));
    const Vector3 halfExtent(0.5f * (bounds[3] - bounds[0]), 0.5f * (bounds[4] - bounds[1]),
                             0.5f * (bounds[5] - bounds[2]));
    const Vector3 centerOffset = center - boxCenter;
    const float boundingRadius =
        sqrtf(halfExtent.x * halfExtent.x + halfExtent.y * halfExtent.y +
              halfExtent.z * halfExtent.z) +
        sqrtf(centerOffset.x * centerOffset.x + centerOffset.y * centerOffset.y +
              centerOffset.z * centerOffset.z);
    if (!(boundingRadius > 0.0f)) {
        return;
    }

    const Vector3 offset = position - center;
    const float normalizedRadius =
        fminf(fmaxf(sqrtf(offset.x * offset.x + offset.y * offset.y + offset.z * offset.z) /
                         boundingRadius,
                     0.0f),
              0.999999f);
    const int bin = min(static_cast<int>(normalizedRadius * kTreePmConcentrationBinCount),
                        kTreePmConcentrationBinCount - 1);
    atomicAdd(histogram + bin, octreeLoadParticleMass(state, particle));
}

} // namespace blitzar_cuda_tree_pm_gpu
