/*
 * @file engine/src/cuda/fragments/treepm/Gpu.inl
 * @author Luis1454
 * @project BLITZAR
 * @brief Physics and CUDA implementation for the deterministic simulation core.
 */

/*
 * Module: cuda
 * Responsibility: Implement the TreePM long-range mesh and short-range tree correction path.
 */

#include <limits>

namespace blitzar_cuda_tree_pm_gpu {

bool checkTreePmFftStatus(cufftResult status, const char* operation)
{
    if (status == CUFFT_SUCCESS) {
        return true;
    }
    fprintf(stderr, "[treepm] cuFFT failure operation=%s status=%d\n", operation,
            static_cast<int>(status));
    return false;
}

__device__ __forceinline__ int treePmGridIndex(int x, int y, int z, const TreePmGridParams& grid)
{
    return (z * grid.gridSize + y) * grid.gridSize + x;
}

__device__ __forceinline__ int treePmWrapIndex(int value, int size)
{
    const int wrapped = value % size;
    return wrapped < 0 ? wrapped + size : wrapped;
}

__device__ __forceinline__ unsigned int treePmMortonPart1By2(unsigned int value)
{
    value &= 0xffu;
    value = (value | (value << 8u)) & 0x00ff00ffu;
    value = (value | (value << 4u)) & 0x0f0f0f0fu;
    value = (value | (value << 2u)) & 0x33333333u;
    value = (value | (value << 1u)) & 0x55555555u;
    return value;
}

__device__ __forceinline__ int treePmMortonIndex(int x, int y, int z)
{
    return static_cast<int>(treePmMortonPart1By2(static_cast<unsigned int>(x)) |
                            (treePmMortonPart1By2(static_cast<unsigned int>(y)) << 1u) |
                            (treePmMortonPart1By2(static_cast<unsigned int>(z)) << 2u));
}

__device__ __forceinline__ float treePmBoundaryPotential(int x, int y, int z,
                                                         const TreePmGridParams& grid)
{
    const float px = grid.originX + static_cast<float>(x) * grid.cellSize;
    const float py = grid.originY + static_cast<float>(y) * grid.cellSize;
    const float pz = grid.originZ + static_cast<float>(z) * grid.cellSize;
    const float dx = px - grid.boundaryCenterX;
    const float dy = py - grid.boundaryCenterY;
    const float dz = pz - grid.boundaryCenterZ;
    const float distance2 = dx * dx + dy * dy + dz * dz +
                            grid.boundarySoftening * grid.boundarySoftening;
    return -grid.boundaryMass * rsqrtf(fmaxf(distance2, 1.0e-12f));
}

__device__ __forceinline__ void treePmSetCellMask(unsigned int* cellMask, int cell)
{
    atomicOr(&cellMask[cell >> 5], 1u << (cell & 31));
}

__device__ __forceinline__ bool treePmCellMaskEnabled(const unsigned int* cellMask, int cell)
{
    return (cellMask[cell >> 5] & (1u << (cell & 31))) != 0u;
}

__device__ __forceinline__ float treePmAssignmentWeight(float distance, int assignment)
{
    const float absoluteDistance = fabsf(distance);
    if (assignment == 1) {
        if (absoluteDistance < 0.5f) {
            return 0.75f - absoluteDistance * absoluteDistance;
        }
        if (absoluteDistance < 1.5f) {
            const float tail = 1.5f - absoluteDistance;
            return 0.5f * tail * tail;
        }
        return 0.0f;
    }
    if (assignment == 2) {
        if (absoluteDistance < 1.0f) {
            return (4.0f - 6.0f * absoluteDistance * absoluteDistance +
                    3.0f * absoluteDistance * absoluteDistance * absoluteDistance) /
                   6.0f;
        }
        if (absoluteDistance < 2.0f) {
            const float tail = 2.0f - absoluteDistance;
            return tail * tail * tail / 6.0f;
        }
        return 0.0f;
    }
    return absoluteDistance < 1.0f ? 1.0f - absoluteDistance : 0.0f;
}

__device__ __forceinline__ float treePmAssignmentWindow(float value, int assignment)
{
    const float sincValue = fabsf(value) < 1.0e-5f ? 1.0f : sinf(value) / value;
    const int power = assignment == 1 ? 6 : assignment == 2 ? 8 : 4;
    float result = 1.0f;
    for (int exponent = 0; exponent < power; ++exponent) {
        result *= sincValue;
    }
    return result;
}

__device__ __forceinline__ float treePmSampleField(const float* field, const TreePmGridParams& grid,
                                                   const Vector3& pos)
{
    const float scaledX = (pos.x - grid.originX) * grid.invCellSize;
    const float scaledY = (pos.y - grid.originY) * grid.invCellSize;
    const float scaledZ = (pos.z - grid.originZ) * grid.invCellSize;

    const float extent = static_cast<float>(grid.gridSize);
    const float clampedX = grid.periodic ? scaledX - floorf(scaledX / extent) * extent
                                         : fminf(fmaxf(scaledX, 0.0f), extent - 1.0f);
    const float clampedY = grid.periodic ? scaledY - floorf(scaledY / extent) * extent
                                         : fminf(fmaxf(scaledY, 0.0f), extent - 1.0f);
    const float clampedZ = grid.periodic ? scaledZ - floorf(scaledZ / extent) * extent
                                         : fminf(fmaxf(scaledZ, 0.0f), extent - 1.0f);

    const int centerX = min(max(static_cast<int>(floorf(clampedX + 0.5f)), 0), grid.gridSize - 1);
    const int centerY = min(max(static_cast<int>(floorf(clampedY + 0.5f)), 0), grid.gridSize - 1);
    const int centerZ = min(max(static_cast<int>(floorf(clampedZ + 0.5f)), 0), grid.gridSize - 1);
    float result = 0.0f;
    for (int dz = -2; dz <= 2; ++dz) {
        const int z = grid.periodic ? treePmWrapIndex(centerZ + dz, grid.gridSize)
                                    : min(max(centerZ + dz, 0), grid.gridSize - 1);
        const float wz = treePmAssignmentWeight(clampedZ - static_cast<float>(centerZ + dz),
                                                 grid.assignment);
        for (int dy = -2; dy <= 2; ++dy) {
            const int y = grid.periodic ? treePmWrapIndex(centerY + dy, grid.gridSize)
                                        : min(max(centerY + dy, 0), grid.gridSize - 1);
            const float wy = treePmAssignmentWeight(clampedY - static_cast<float>(centerY + dy),
                                                     grid.assignment);
            for (int dx = -2; dx <= 2; ++dx) {
                const int x = grid.periodic ? treePmWrapIndex(centerX + dx, grid.gridSize)
                                            : min(max(centerX + dx, 0), grid.gridSize - 1);
                const float wx = treePmAssignmentWeight(
                    clampedX - static_cast<float>(centerX + dx), grid.assignment);
                result += field[treePmGridIndex(x, y, z, grid)] * wx * wy * wz;
            }
        }
    }
    return result;
}

__device__ __forceinline__ Vector3 treePmSampleAcceleration(const TreePmGridParams& grid,
                                                            const Vector3& pos, const float* accelX,
                                                            const float* accelY,
                                                            const float* accelZ)
{
    return Vector3(treePmSampleField(accelX, grid, pos), treePmSampleField(accelY, grid, pos),
                   treePmSampleField(accelZ, grid, pos));
}

__device__ __forceinline__ float treePmModifiedBesselK1(float value)
{
    const float x = fmaxf(value, 1.0e-4f);
    if (x <= 2.0f) {
        const float y = x * x * 0.25f;
        const float i1 = (x * 0.5f) *
                         (1.0f + y * (0.87890594f + y * (0.51498869f +
                         y * (0.15084934f + y * (0.02658733f +
                         y * (0.00301532f + y * 0.00032411f))))));
        return logf(x * 0.5f) * i1 +
               (1.0f / x) * (1.0f + y * (0.15443144f + y * (-0.67278579f +
               y * (-0.18156897f + y * (-0.01919402f + y * (-0.00110404f +
               y * -0.00004686f))))));
    }
    const float y = 2.0f / x;
    return expf(-x) * rsqrtf(x) * 1.25331414f *
           (1.0f + y * (0.23498619f + y * (-0.03655620f +
           y * (0.01504268f + y * (-0.00780353f + y *
           (0.00325614f + y * -0.00068245f))))));
}

__device__ __forceinline__ float treePmSinc(float value)
{
    return fabsf(value) < 1.0e-5f ? 1.0f : sinf(value) / value;
}

__global__ void treePmApplyFftKernel(const cufftComplex* densitySpectrum,
                                     cufftComplex* potentialSpectrum, TreePmGridParams grid,
                                     float softening)
{
    const int halfGridSize = grid.gridSize / 2 + 1;
    const int spectrumCells = grid.gridSize * grid.gridSize * halfGridSize;
    const int index = blockIdx.x * blockDim.x + threadIdx.x;
    if (index >= spectrumCells) {
        return;
    }

    const int x = index % halfGridSize;
    const int planeIndex = index / halfGridSize;
    const int y = planeIndex % grid.gridSize;
    const int z = planeIndex / grid.gridSize;
    const int signedY = y <= grid.gridSize / 2 ? y : y - grid.gridSize;
    const int signedZ = z <= grid.gridSize / 2 ? z : z - grid.gridSize;
    const float waveScale = 6.2831853071795864769f /
                            (static_cast<float>(grid.gridSize) * grid.cellSize);
    const float kx = static_cast<float>(x) * waveScale;
    const float ky = static_cast<float>(signedY) * waveScale;
    const float kz = static_cast<float>(signedZ) * waveScale;
    const float kSquared = kx * kx + ky * ky + kz * kz;
    const cufftComplex source = densitySpectrum[index];
    if (kSquared <= 1.0e-12f) {
        potentialSpectrum[index] = make_cuFloatComplex(0.0f, 0.0f);
        return;
    }

    (void)softening;
    const float splitScale = fmaxf(grid.shortRangeScale, 1.0e-6f);
    const float greenMagnitude = grid.periodic
                                     ? grid.poissonCoefficient / kSquared
                                     : 12.566370614359172f *
                                           expf(-kSquared * splitScale * splitScale) / kSquared;
    // Assignment is applied once during deposition and once during particle sampling.
    const float assignmentWindow = fmaxf(
        treePmAssignmentWindow(0.5f * kx * grid.cellSize, grid.assignment) *
            treePmAssignmentWindow(0.5f * ky * grid.cellSize, grid.assignment) *
            treePmAssignmentWindow(0.5f * kz * grid.cellSize, grid.assignment),
        0.08f);
    const float scale = greenMagnitude / assignmentWindow;
    // treePmPotentialGradientKernel evaluates -grad(phi); the spectral potential
    // therefore carries the negative Poisson Green function.
    potentialSpectrum[index] = make_cuFloatComplex(-scale * source.x, -scale * source.y);
}

__global__ void treePmNormalizeFftFieldKernel(float* field, int totalCells)
{
    const int index = blockIdx.x * blockDim.x + threadIdx.x;
    if (index < totalCells) {
        field[index] /= static_cast<float>(totalCells);
    }
}

__global__ void treePmBuildDensityContrastKernel(float* density, int totalCells)
{
    const int index = blockIdx.x * blockDim.x + threadIdx.x;
    if (index < totalCells) {
        density[index] -= 1.0f;
    }
}

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

__global__ void treePmDepositMassKernel(ParticleSoAView state, int numParticles, int particleLimit,
                                        TreePmGridParams grid, float* density,
                                        unsigned int* cellMask)
{
    const int particleIndex = blockIdx.x * blockDim.x + threadIdx.x;
    if (particleIndex >= numParticles || particleIndex >= particleLimit) {
        return;
    }

    const Vector3 pos = octreeLoadParticlePosition(state, particleIndex);
    const float scaledX = (pos.x - grid.originX) * grid.invCellSize;
    const float scaledY = (pos.y - grid.originY) * grid.invCellSize;
    const float scaledZ = (pos.z - grid.originZ) * grid.invCellSize;

    const float extent = static_cast<float>(grid.gridSize);
    const float clampedX = grid.periodic ? scaledX - floorf(scaledX / extent) * extent
                                         : fminf(fmaxf(scaledX, 0.0f), extent - 1.0f);
    const float clampedY = grid.periodic ? scaledY - floorf(scaledY / extent) * extent
                                         : fminf(fmaxf(scaledY, 0.0f), extent - 1.0f);
    const float clampedZ = grid.periodic ? scaledZ - floorf(scaledZ / extent) * extent
                                         : fminf(fmaxf(scaledZ, 0.0f), extent - 1.0f);

    const float volumeInv = 1.0f / (grid.cellSize * grid.cellSize * grid.cellSize);
    const float massDensity = octreeLoadParticleMass(state, particleIndex) * volumeInv *
                              (grid.periodic ? grid.densityScale : 1.0f);
    const int centerX = min(max(static_cast<int>(floorf(clampedX + 0.5f)), 0), grid.gridSize - 1);
    const int centerY = min(max(static_cast<int>(floorf(clampedY + 0.5f)), 0), grid.gridSize - 1);
    const int centerZ = min(max(static_cast<int>(floorf(clampedZ + 0.5f)), 0), grid.gridSize - 1);
    for (int dz = -2; dz <= 2; ++dz) {
        const int z = grid.periodic ? treePmWrapIndex(centerZ + dz, grid.gridSize)
                                    : min(max(centerZ + dz, 0), grid.gridSize - 1);
        const float wz = treePmAssignmentWeight(clampedZ - static_cast<float>(centerZ + dz),
                                                 grid.assignment);
        for (int dy = -2; dy <= 2; ++dy) {
            const int y = grid.periodic ? treePmWrapIndex(centerY + dy, grid.gridSize)
                                        : min(max(centerY + dy, 0), grid.gridSize - 1);
            const float wy = treePmAssignmentWeight(clampedY - static_cast<float>(centerY + dy),
                                                     grid.assignment);
            for (int dx = -2; dx <= 2; ++dx) {
                const int x = grid.periodic ? treePmWrapIndex(centerX + dx, grid.gridSize)
                                            : min(max(centerX + dx, 0), grid.gridSize - 1);
                const float wx = treePmAssignmentWeight(
                    clampedX - static_cast<float>(centerX + dx), grid.assignment);
                const int cell = treePmGridIndex(x, y, z, grid);
                const float contribution = massDensity * wx * wy * wz;
                if (contribution != 0.0f) {
                    atomicAdd(&density[cell], contribution);
                    treePmSetCellMask(cellMask, cell);
                }
            }
        }
    }
}

__global__ void treePmJacobiStepKernel(const float* density, const float* potentialIn,
                                       float* potentialOut, TreePmGridParams grid)
{
    const int index = blockIdx.x * blockDim.x + threadIdx.x;
    if (index >= grid.totalCells) {
        return;
    }

    const int plane = grid.gridSize * grid.gridSize;
    const int z = index / plane;
    const int rem = index - z * plane;
    const int y = rem / grid.gridSize;
    const int x = rem - y * grid.gridSize;

    if (x == 0 || y == 0 || z == 0 || x == grid.gridSize - 1 || y == grid.gridSize - 1 ||
        z == grid.gridSize - 1) {
        potentialOut[index] = treePmBoundaryPotential(x, y, z, grid);
        return;
    }

    const int xm = treePmGridIndex(x - 1, y, z, grid);
    const int xp = treePmGridIndex(x + 1, y, z, grid);
    const int ym = treePmGridIndex(x, y - 1, z, grid);
    const int yp = treePmGridIndex(x, y + 1, z, grid);
    const int zm = treePmGridIndex(x, y, z - 1, grid);
    const int zp = treePmGridIndex(x, y, z + 1, grid);
    const float h2 = grid.cellSize * grid.cellSize;
    // Match the pairwise G=1 law: the Green function satisfies nabla^2(phi) = 4*pi*rho.
    constexpr float kFourPi = 12.566370614359172f;
    const float rhs = -kFourPi * density[index] * h2;
    potentialOut[index] = (potentialIn[xm] + potentialIn[xp] + potentialIn[ym] + potentialIn[yp] +
                           potentialIn[zm] + potentialIn[zp] + rhs) *
                          (1.0f / 6.0f);
}

__global__ void treePmPotentialGradientKernel(const float* potential, float* accelX, float* accelY,
                                              float* accelZ, TreePmGridParams grid)
{
    const int index = blockIdx.x * blockDim.x + threadIdx.x;
    if (index >= grid.totalCells) {
        return;
    }

    const int plane = grid.gridSize * grid.gridSize;
    const int z = index / plane;
    const int rem = index - z * plane;
    const int y = rem / grid.gridSize;
    const int x = rem - y * grid.gridSize;

    if (!grid.periodic &&
        (x == 0 || y == 0 || z == 0 || x == grid.gridSize - 1 || y == grid.gridSize - 1 ||
         z == grid.gridSize - 1)) {
        accelX[index] = 0.0f;
        accelY[index] = 0.0f;
        accelZ[index] = 0.0f;
        return;
    }

    const int xm = treePmGridIndex(grid.periodic ? treePmWrapIndex(x - 1, grid.gridSize) : x - 1, y, z, grid);
    const int xp = treePmGridIndex(grid.periodic ? treePmWrapIndex(x + 1, grid.gridSize) : x + 1, y, z, grid);
    const int ym = treePmGridIndex(x, grid.periodic ? treePmWrapIndex(y - 1, grid.gridSize) : y - 1, z, grid);
    const int yp = treePmGridIndex(x, grid.periodic ? treePmWrapIndex(y + 1, grid.gridSize) : y + 1, z, grid);
    const int zm = treePmGridIndex(x, y, grid.periodic ? treePmWrapIndex(z - 1, grid.gridSize) : z - 1, grid);
    const int zp = treePmGridIndex(x, y, grid.periodic ? treePmWrapIndex(z + 1, grid.gridSize) : z + 1, grid);
    const float invTwoH = 0.5f * grid.invCellSize;
    accelX[index] = -(potential[xp] - potential[xm]) * invTwoH;
    accelY[index] = -(potential[yp] - potential[ym]) * invTwoH;
    accelZ[index] = -(potential[zp] - potential[zm]) * invTwoH;
}

__global__ void treePmBuildCellHashKernel(ParticleSoAView state, int numParticles,
                                          TreePmGridParams grid, IndexHandle cellHash,
                                          IndexHandle particleIndex, int useMorton)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }
    const Vector3 pos = octreeLoadParticlePosition(state, i);
    const int x = min(max(static_cast<int>(floorf((pos.x - grid.originX) * grid.invCellSize)), 0),
                      grid.gridSize - 1);
    const int y = min(max(static_cast<int>(floorf((pos.y - grid.originY) * grid.invCellSize)), 0),
                      grid.gridSize - 1);
    const int z = min(max(static_cast<int>(floorf((pos.z - grid.originZ) * grid.invCellSize)), 0),
                      grid.gridSize - 1);
    cellHash[i] = useMorton != 0 ? treePmMortonIndex(x, y, z) : treePmGridIndex(x, y, z, grid);
    particleIndex[i] = i;
}

__global__ void treePmBuildSortedCellHashKernel(ParticleSoAView state, IndexConstHandle sortedIndex,
                                                IndexHandle sortedCellHash, int numParticles,
                                                TreePmGridParams grid)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }
    const int particleIndex = __ldg(&sortedIndex[i]);
    const Vector3 pos = octreeLoadParticlePosition(state, particleIndex);
    const int x = min(max(static_cast<int>(floorf((pos.x - grid.originX) * grid.invCellSize)), 0),
                      grid.gridSize - 1);
    const int y = min(max(static_cast<int>(floorf((pos.y - grid.originY) * grid.invCellSize)), 0),
                      grid.gridSize - 1);
    const int z = min(max(static_cast<int>(floorf((pos.z - grid.originZ) * grid.invCellSize)), 0),
                      grid.gridSize - 1);
    sortedCellHash[i] = treePmGridIndex(x, y, z, grid);
}

__global__ void treePmGatherSortedParticlesKernel(
    ParticleSoAView state, IndexConstHandle sortedIndex, float* sortedPosX, float* sortedPosY,
    float* sortedPosZ, float* sortedMass, int numParticles)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }
    const int sourceIndex = __ldg(&sortedIndex[i]);
    sortedPosX[i] = __ldg(&state.posX[sourceIndex]);
    sortedPosY[i] = __ldg(&state.posY[sourceIndex]);
    sortedPosZ[i] = __ldg(&state.posZ[sourceIndex]);
    sortedMass[i] = __ldg(&state.mass[sourceIndex]);
}

__global__ void treePmResetCellBoundsKernel(IndexHandle cellStart, IndexHandle cellEnd,
                                            int totalCells)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= totalCells) {
        return;
    }
    cellStart[i] = -1;
    cellEnd[i] = -1;
}

__global__ void treePmFindCellBoundsKernel(IndexConstHandle sortedHash, IndexHandle cellStart,
                                           IndexHandle cellEnd, int numParticles)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }
    const int hash = __ldg(&sortedHash[i]);
    if (i == 0 || hash != __ldg(&sortedHash[i - 1])) {
        cellStart[hash] = i;
    }
    if (i == numParticles - 1 || hash != __ldg(&sortedHash[i + 1])) {
        cellEnd[hash] = i + 1;
    }
}

__device__ __forceinline__ Vector3 treePmComputeLocalGridAcceleration(
    ParticleSoAView state, int selfIndex, TreePmGridParams grid, IndexConstHandle sortedIndex,
    IndexConstHandle cellStart, IndexConstHandle cellEnd, ForceLawPolicy forceLaw,
    float maxAcceleration, const float* accelX, const float* accelY, const float* accelZ,
    const unsigned int* cellMask, float cutoffSquared, int cellRadius, int maxLocalNeighbors,
    const float* sortedPosX, const float* sortedPosY, const float* sortedPosZ,
    const float* sortedMass)
{
    constexpr int kMaxTreePmCellRadius = 2;
    const Vector3 selfPos = octreeLoadParticlePosition(state, selfIndex);
    Vector3 force = treePmSampleAcceleration(grid, selfPos, accelX, accelY, accelZ);
    if (maxLocalNeighbors <= 0) {
        return clampAcceleration(force, maxAcceleration);
    }

    int acceptedNeighbors = 0;
    int examinedCandidates = 0;
    const int maxExaminedCandidates = max(maxLocalNeighbors * 4, maxLocalNeighbors);

    const int centerX =
        min(max(static_cast<int>(floorf((selfPos.x - grid.originX) * grid.invCellSize)), 0),
            grid.gridSize - 1);
    const int centerY =
        min(max(static_cast<int>(floorf((selfPos.y - grid.originY) * grid.invCellSize)), 0),
            grid.gridSize - 1);
    const int centerZ =
        min(max(static_cast<int>(floorf((selfPos.z - grid.originZ) * grid.invCellSize)), 0),
            grid.gridSize - 1);
    const int boundedRadius = min(max(cellRadius, 1), kMaxTreePmCellRadius);

    for (int shell = 0; shell <= kMaxTreePmCellRadius; ++shell) {
        if (shell > boundedRadius || acceptedNeighbors >= maxLocalNeighbors ||
            examinedCandidates >= maxExaminedCandidates) {
            break;
        }
        for (int dz = -kMaxTreePmCellRadius; dz <= kMaxTreePmCellRadius; ++dz) {
            if (acceptedNeighbors >= maxLocalNeighbors ||
                examinedCandidates >= maxExaminedCandidates) {
                break;
            }
            if (abs(dz) > shell) {
                continue;
            }
            const int z = centerZ + dz;
            if (z < 0 || z >= grid.gridSize) {
                continue;
            }
            for (int dy = -kMaxTreePmCellRadius; dy <= kMaxTreePmCellRadius; ++dy) {
                if (acceptedNeighbors >= maxLocalNeighbors ||
                    examinedCandidates >= maxExaminedCandidates) {
                    break;
                }
                if (abs(dy) > shell) {
                    continue;
                }
                const int y = centerY + dy;
                if (y < 0 || y >= grid.gridSize) {
                    continue;
                }
                for (int dx = -kMaxTreePmCellRadius; dx <= kMaxTreePmCellRadius; ++dx) {
                    if (acceptedNeighbors >= maxLocalNeighbors ||
                        examinedCandidates >= maxExaminedCandidates) {
                        break;
                    }
                    if (abs(dx) > shell || max(max(abs(dx), abs(dy)), abs(dz)) != shell) {
                        continue;
                    }
                    const int x = centerX + dx;
                    if (x < 0 || x >= grid.gridSize) {
                        continue;
                    }
                    const int cell = treePmGridIndex(x, y, z, grid);
                    if (!treePmCellMaskEnabled(cellMask, cell)) {
                        continue;
                    }
                    const int begin = __ldg(&cellStart[cell]);
                    const int end = __ldg(&cellEnd[cell]);
                    if (begin < 0 || end <= begin) {
                        continue;
                    }
                    for (int cursor = begin; cursor < end; ++cursor) {
                        if (acceptedNeighbors >= maxLocalNeighbors ||
                            examinedCandidates >= maxExaminedCandidates) {
                            break;
                        }
                        const int otherIndex = __ldg(&sortedIndex[cursor]);
                        if (otherIndex == selfIndex) {
                            continue;
                        }
                        ++examinedCandidates;
                        const Vector3 sourcePos = sortedPosX != nullptr
                                                       ? Vector3(__ldg(&sortedPosX[cursor]),
                                                                 __ldg(&sortedPosY[cursor]),
                                                                 __ldg(&sortedPosZ[cursor]))
                                                       : octreeLoadParticlePosition(state, otherIndex);
                        const Vector3 diff(selfPos.x - sourcePos.x, selfPos.y - sourcePos.y,
                                           selfPos.z - sourcePos.z);
                        if (dot(diff, diff) > cutoffSquared) {
                            continue;
                        }
                        const float sourceMass = sortedMass != nullptr
                                                     ? __ldg(&sortedMass[cursor])
                                                     : octreeLoadParticleMass(state, otherIndex);
                        ForceLawPolicy shortRangeLaw = forceLaw;
                        shortRangeLaw.treePmShortRangeScale = grid.shortRangeScale;
                        force += blitzarAccelerationFromSource(selfPos, sourcePos, sourceMass,
                                                               shortRangeLaw);
                        ++acceptedNeighbors;
                    }
                }
            }
        }
    }
    return clampAcceleration(force, maxAcceleration);
}

__global__ void updateParticlesTreePmLocalGridKernel(
    ParticleSoAView lastState, ParticleSoAView particlesOut, int numParticles,
    TreePmGridParams grid, IndexConstHandle sortedIndex, IndexConstHandle cellStart,
    IndexConstHandle cellEnd, ForceLawPolicy forceLaw, float deltaTime, float maxAcceleration,
    const float* accelX, const float* accelY, const float* accelZ, const unsigned int* cellMask,
    float cutoffSquared, int cellRadius, int maxLocalNeighbors, const float* sortedPosX,
    const float* sortedPosY, const float* sortedPosZ, const float* sortedMass)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }

    const Vector3 selfPos = octreeLoadParticlePosition(lastState, i);
    const Vector3 force = treePmComputeLocalGridAcceleration(
        lastState, i, grid, sortedIndex, cellStart, cellEnd, forceLaw, maxAcceleration, accelX,
        accelY, accelZ, cellMask, cutoffSquared, cellRadius, maxLocalNeighbors, sortedPosX,
        sortedPosY, sortedPosZ, sortedMass);
    const Vector3 vel = octreeLoadParticleVelocity(lastState, i);
    const Vector3 nextVel = vel + force * deltaTime;
    const Vector3 nextPos = selfPos + nextVel * deltaTime;

    setSoAPressure(particlesOut, i, force * 100.0f);
    setSoAVelocity(particlesOut, i, nextVel);
    setSoAPosition(particlesOut, i, nextPos);

    particlesOut.mass[i] = __ldg(&lastState.mass[i]);
    if (particlesOut.temp != nullptr && lastState.temp != nullptr) {
        particlesOut.temp[i] = __ldg(&lastState.temp[i]);
    }
    if (particlesOut.dens != nullptr && lastState.dens != nullptr) {
        particlesOut.dens[i] = __ldg(&lastState.dens[i]);
    }
}

__device__ __forceinline__ Vector3 treePmComputeAcceleration(
    ParticleSoAView state, int selfIndex, const GpuOctreeNodeHotData* __restrict__ nodeHot,
    const GpuOctreeNodeNavData* __restrict__ nodeNav, IndexConstHandle nodeFirstChild,
    IndexConstHandle leafStarts, IndexConstHandle leafCounts, int rootIndex,
    IndexConstHandle leafIndices, ForceLawPolicy forceLaw, float maxAcceleration,
    int openingCriterion, float cutoffSquared, TreePmGridParams grid, const float* accelX,
    const float* accelY, const float* accelZ)
{
    const Vector3 selfPos = octreeLoadParticlePosition(state, selfIndex);
    const Vector3 pmAcceleration = treePmSampleAcceleration(grid, selfPos, accelX, accelY, accelZ);
    ForceLawPolicy shortRangeLaw = forceLaw;
    shortRangeLaw.treePmShortRangeScale = grid.shortRangeScale;
    const Vector3 treeAcceleration = computeOctreeAccelerationStacklessCompact(
        state, selfIndex, nodeHot, nodeNav, nodeFirstChild, leafStarts, leafCounts, rootIndex,
        leafIndices, shortRangeLaw, maxAcceleration, openingCriterion, cutoffSquared);
    return clampAcceleration(pmAcceleration + treeAcceleration, maxAcceleration);
}

__global__ void treePmRedBlackStepKernel(float* potential, const float* density,
                                          TreePmGridParams grid, int parity)
{
    const int index = blockIdx.x * blockDim.x + threadIdx.x;
    if (index >= grid.totalCells) {
        return;
    }

    const int plane = grid.gridSize * grid.gridSize;
    const int z = index / plane;
    const int rem = index - z * plane;
    const int y = rem / grid.gridSize;
    const int x = rem - y * grid.gridSize;
    if (!grid.periodic &&
        (x == 0 || y == 0 || z == 0 || x == grid.gridSize - 1 || y == grid.gridSize - 1 ||
         z == grid.gridSize - 1)) {
        potential[index] = treePmBoundaryPotential(x, y, z, grid);
        return;
    }
    if (((x + y + z) & 1) != parity) {
        return;
    }

    const int xm = treePmGridIndex(grid.periodic ? treePmWrapIndex(x - 1, grid.gridSize) : x - 1, y, z, grid);
    const int xp = treePmGridIndex(grid.periodic ? treePmWrapIndex(x + 1, grid.gridSize) : x + 1, y, z, grid);
    const int ym = treePmGridIndex(x, grid.periodic ? treePmWrapIndex(y - 1, grid.gridSize) : y - 1, z, grid);
    const int yp = treePmGridIndex(x, grid.periodic ? treePmWrapIndex(y + 1, grid.gridSize) : y + 1, z, grid);
    const int zm = treePmGridIndex(x, y, grid.periodic ? treePmWrapIndex(z - 1, grid.gridSize) : z - 1, grid);
    const int zp = treePmGridIndex(x, y, grid.periodic ? treePmWrapIndex(z + 1, grid.gridSize) : z + 1, grid);
    const float h2 = grid.cellSize * grid.cellSize;
    constexpr float kFourPi = 12.566370614359172f;
    const float rhs = -kFourPi * density[index] * h2;
    potential[index] = (potential[xm] + potential[xp] + potential[ym] + potential[yp] +
                        potential[zm] + potential[zp] + rhs) * (1.0f / 6.0f);
}

__device__ __forceinline__ int treePmCellPopulation(
    const Vector3& selfPos, TreePmGridParams grid, IndexConstHandle cellStart,
    IndexConstHandle cellEnd)
{
    const int x = min(max(static_cast<int>(floorf((selfPos.x - grid.originX) * grid.invCellSize)), 0),
                      grid.gridSize - 1);
    const int y = min(max(static_cast<int>(floorf((selfPos.y - grid.originY) * grid.invCellSize)), 0),
                      grid.gridSize - 1);
    const int z = min(max(static_cast<int>(floorf((selfPos.z - grid.originZ) * grid.invCellSize)), 0),
                      grid.gridSize - 1);
    const int cell = treePmGridIndex(x, y, z, grid);
    const int begin = __ldg(&cellStart[cell]);
    const int end = __ldg(&cellEnd[cell]);
    return begin >= 0 && end > begin ? end - begin : 0;
}

__device__ __forceinline__ Vector3 treePmComputeHybridAcceleration(
    ParticleSoAView state, int selfIndex, const GpuOctreeNodeHotData* nodeHot,
    const GpuOctreeNodeNavData* nodeNav, IndexConstHandle nodeFirstChild,
    IndexConstHandle leafStarts, IndexConstHandle leafCounts, int rootIndex,
    IndexConstHandle leafIndices, ForceLawPolicy forceLaw, float maxAcceleration,
    int openingCriterion, float cutoffSquared, TreePmGridParams grid,
    IndexConstHandle sortedIndex, IndexConstHandle cellStart, IndexConstHandle cellEnd,
    const float* accelX, const float* accelY, const float* accelZ, const unsigned int* cellMask,
    int cellRadius, int maxLocalNeighbors, int denseCellThreshold, const float* sortedPosX,
    const float* sortedPosY, const float* sortedPosZ, const float* sortedMass)
{
    const Vector3 selfPos = octreeLoadParticlePosition(state, selfIndex);
    if (treePmCellPopulation(selfPos, grid, cellStart, cellEnd) >= denseCellThreshold) {
        return treePmComputeAcceleration(
            state, selfIndex, nodeHot, nodeNav, nodeFirstChild, leafStarts, leafCounts, rootIndex,
            leafIndices, forceLaw, maxAcceleration, openingCriterion, cutoffSquared, grid, accelX,
            accelY, accelZ);
    }
    return treePmComputeLocalGridAcceleration(
        state, selfIndex, grid, sortedIndex, cellStart, cellEnd, forceLaw, maxAcceleration, accelX,
        accelY, accelZ, cellMask, cutoffSquared, cellRadius, maxLocalNeighbors, sortedPosX,
        sortedPosY, sortedPosZ, sortedMass);
}

__global__ void updateParticlesTreePmHybridKernel(
    ParticleSoAView lastState, ParticleSoAView particlesOut, int numParticles,
    const GpuOctreeNodeHotData* nodeHot, const GpuOctreeNodeNavData* nodeNav,
    IndexConstHandle nodeFirstChild, IndexConstHandle leafStarts, IndexConstHandle leafCounts,
    int rootIndex, IndexConstHandle leafIndices, ForceLawPolicy forceLaw, float deltaTime,
    float maxAcceleration, int openingCriterion, TreePmGridParams grid,
    IndexConstHandle sortedIndex, IndexConstHandle cellStart, IndexConstHandle cellEnd,
    const float* accelX, const float* accelY, const float* accelZ, const unsigned int* cellMask,
    float cutoffSquared, int cellRadius, int maxLocalNeighbors, int denseCellThreshold,
    const float* sortedPosX, const float* sortedPosY, const float* sortedPosZ,
    const float* sortedMass)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles || rootIndex < 0) {
        return;
    }

    const Vector3 selfPos = octreeLoadParticlePosition(lastState, i);
    const Vector3 force = treePmComputeHybridAcceleration(
        lastState, i, nodeHot, nodeNav, nodeFirstChild, leafStarts, leafCounts, rootIndex,
        leafIndices, forceLaw, maxAcceleration, openingCriterion, cutoffSquared, grid, sortedIndex,
        cellStart, cellEnd, accelX, accelY, accelZ, cellMask, cellRadius, maxLocalNeighbors,
        denseCellThreshold, sortedPosX, sortedPosY, sortedPosZ, sortedMass);
    const Vector3 vel = octreeLoadParticleVelocity(lastState, i);
    const Vector3 nextVel = vel + force * deltaTime;
    const Vector3 nextPos = selfPos + nextVel * deltaTime;

    setSoAPressure(particlesOut, i, force * 100.0f);
    setSoAVelocity(particlesOut, i, nextVel);
    setSoAPosition(particlesOut, i, nextPos);
    particlesOut.mass[i] = __ldg(&lastState.mass[i]);
    if (particlesOut.temp != nullptr && lastState.temp != nullptr) {
        particlesOut.temp[i] = __ldg(&lastState.temp[i]);
    }
    if (particlesOut.dens != nullptr && lastState.dens != nullptr) {
        particlesOut.dens[i] = __ldg(&lastState.dens[i]);
    }
}

__global__ void computeTreePmAccelerationKernel(
    ParticleSoAView state, Vector3Handle outAcceleration, int numParticles,
    const GpuOctreeNodeHotData* nodeHot, const GpuOctreeNodeNavData* nodeNav,
    IndexConstHandle nodeFirstChild, IndexConstHandle leafStarts, IndexConstHandle leafCounts,
    int rootIndex, IndexConstHandle leafIndices, ForceLawPolicy forceLaw, float maxAcceleration,
    int openingCriterion, TreePmGridParams grid, const float* accelX, const float* accelY,
    const float* accelZ, float cutoffSquared)
{
    const int particleIndex = blockIdx.x * blockDim.x + threadIdx.x;
    if (particleIndex >= numParticles || rootIndex < 0) {
        return;
    }

    outAcceleration[particleIndex] = treePmComputeAcceleration(
        state, particleIndex, nodeHot, nodeNav, nodeFirstChild, leafStarts, leafCounts, rootIndex,
        leafIndices, forceLaw, maxAcceleration, openingCriterion, cutoffSquared, grid, accelX,
        accelY, accelZ);
}

__global__ void updateParticlesTreePmKernel(
    ParticleSoAView lastState, ParticleSoAView particlesOut, int numParticles,
    const GpuOctreeNodeHotData* nodeHot, const GpuOctreeNodeNavData* nodeNav,
    IndexConstHandle nodeFirstChild, IndexConstHandle leafStarts, IndexConstHandle leafCounts,
    int rootIndex, IndexConstHandle leafIndices, ForceLawPolicy forceLaw, float deltaTime,
    float maxAcceleration, int openingCriterion, TreePmGridParams grid, const float* accelX,
    const float* accelY, const float* accelZ, float cutoffSquared)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles || rootIndex < 0) {
        return;
    }

    const Vector3 selfPos = octreeLoadParticlePosition(lastState, i);
    const Vector3 force =
        treePmComputeAcceleration(lastState, i, nodeHot, nodeNav, nodeFirstChild, leafStarts,
                                  leafCounts, rootIndex, leafIndices, forceLaw, maxAcceleration,
                                  openingCriterion, cutoffSquared, grid, accelX, accelY, accelZ);
    const Vector3 vel = octreeLoadParticleVelocity(lastState, i);
    const Vector3 nextVel = vel + force * deltaTime;
    const Vector3 nextPos = selfPos + nextVel * deltaTime;

    setSoAPressure(particlesOut, i, force * 100.0f);
    setSoAVelocity(particlesOut, i, nextVel);
    setSoAPosition(particlesOut, i, nextPos);

    particlesOut.mass[i] = __ldg(&lastState.mass[i]);
    if (particlesOut.temp != nullptr && lastState.temp != nullptr) {
        particlesOut.temp[i] = __ldg(&lastState.temp[i]);
    }
    if (particlesOut.dens != nullptr && lastState.dens != nullptr) {
        particlesOut.dens[i] = __ldg(&lastState.dens[i]);
    }
}

} // namespace blitzar_cuda_tree_pm_gpu

int ParticleSystem::treePmLayoutMode()
{
    if (_device._treePmLayoutModeInitialized) {
        if (_device._treePmLayoutMode != kTreePmLayoutAuto) {
            return _device._treePmLayoutMode;
        }
        return _device._treePmAutoLayoutResolved
                   ? (_device._treePmAutoGather
                          ? (_device._treePmAutoMorton ? kTreePmLayoutGatherMorton
                                                       : kTreePmLayoutGatherLinear)
                          : kTreePmLayoutLinear)
                   : kTreePmLayoutAuto;
    }

    _device._treePmLayoutModeInitialized = true;
    _device._treePmLayoutMode = kTreePmLayoutLegacy;
    const auto environmentLayout = bltzr_env::get("BLITZAR_TREEPM_LAYOUT");
    const std::string configured = environmentLayout.has_value() ? *environmentLayout : _treePmLayout;
    if (configured.empty()) {
        const bool legacyFlags = bltzr_env::get("BLITZAR_TREEPM_GATHER").has_value() ||
                                 bltzr_env::get("BLITZAR_TREEPM_MORTON").has_value();
        _device._treePmLayoutMode = legacyFlags ? kTreePmLayoutLegacy : kTreePmLayoutAuto;
        return _device._treePmLayoutMode;
    }
    if (configured == "auto") {
        _device._treePmLayoutMode = kTreePmLayoutAuto;
        return kTreePmLayoutAuto;
    }
    if (configured == "linear") {
        _device._treePmLayoutMode = kTreePmLayoutLinear;
        return kTreePmLayoutLinear;
    }
    if (configured == "gather_linear") {
        _device._treePmLayoutMode = kTreePmLayoutGatherLinear;
        return kTreePmLayoutGatherLinear;
    }
    if (configured == "gather_morton") {
        _device._treePmLayoutMode = kTreePmLayoutGatherMorton;
        return kTreePmLayoutGatherMorton;
    }
    fprintf(stderr, "[treepm] invalid layout=%s fallback=auto\n", configured.c_str());
    _device._treePmLayoutMode = kTreePmLayoutAuto;
    return kTreePmLayoutAuto;
}

__global__ void computeTreePmPmOnlyAccelerationKernel(ParticleSoAView state,
                                                       Vector3Handle outAcceleration,
                                                       int numParticles,
                                                       TreePmGridParams grid,
                                                       const float* accelX,
                                                       const float* accelY,
                                                       const float* accelZ)
{
    const int particleIndex = blockIdx.x * blockDim.x + threadIdx.x;
    if (particleIndex < numParticles) {
        outAcceleration[particleIndex] = treePmSampleAcceleration(
            grid, octreeLoadParticlePosition(state, particleIndex), accelX, accelY, accelZ);
    }
}

bool ParticleSystem::treePmGatherEnabled()
{
    const int layout = treePmLayoutMode();
    if (layout == kTreePmLayoutAuto) {
        return _device._treePmAutoLayoutResolved && _device._treePmAutoGather;
    }
    if (layout == kTreePmLayoutGatherLinear || layout == kTreePmLayoutGatherMorton) {
        return true;
    }
    if (layout == kTreePmLayoutLinear) {
        return false;
    }
    return parseBoolEnv("BLITZAR_TREEPM_GATHER", false);
}

bool ParticleSystem::treePmMortonEnabled()
{
    const int layout = treePmLayoutMode();
    if (layout == kTreePmLayoutAuto) {
        return _device._treePmAutoLayoutResolved && _device._treePmAutoMorton;
    }
    if (layout == kTreePmLayoutGatherMorton) {
        return true;
    }
    if (layout == kTreePmLayoutLinear || layout == kTreePmLayoutGatherLinear) {
        return false;
    }
    return parseBoolEnv("BLITZAR_TREEPM_MORTON", false);
}

bool ParticleSystem::ensureTreePmBoundsCapacity(int numParticles)
{
    if (!_device._cudaRuntimeAvailable || numParticles <= 0) {
        return false;
    }
    const std::size_t blockCount =
        (static_cast<std::size_t>(numParticles) + kTreePmBoundsBlockSize - 1u) /
        kTreePmBoundsBlockSize;
    if (_device.d_treePmBoundsBlockCapacity >= blockCount && _device.d_treePmBoundsPartial != nullptr &&
        _device.d_treePmBounds != nullptr) {
        return true;
    }

    bltzr_x::MemoryPool::deallocate(_device.d_treePmBoundsPartial);
    bltzr_x::MemoryPool::deallocate(_device.d_treePmBounds);
    _device.d_treePmBoundsPartial = static_cast<float*>(bltzr_x::MemoryPool::allocate(
        blockCount * kTreePmBoundsFieldCount * sizeof(float)));
    _device.d_treePmBounds = static_cast<float*>(
        bltzr_x::MemoryPool::allocate(kTreePmBoundsFieldCount * sizeof(float)));
    if (_device.d_treePmBoundsPartial == nullptr || _device.d_treePmBounds == nullptr) {
        bltzr_x::MemoryPool::deallocate(_device.d_treePmBoundsPartial);
        bltzr_x::MemoryPool::deallocate(_device.d_treePmBounds);
        _device.d_treePmBoundsPartial = nullptr;
        _device.d_treePmBounds = nullptr;
        _device.d_treePmBoundsBlockCapacity = 0u;
        return false;
    }
    _device.d_treePmBoundsBlockCapacity = blockCount;
    return true;
}

bool ParticleSystem::ensureTreePmConcentrationCapacity()
{
    if (!_device._cudaRuntimeAvailable) {
        return false;
    }
    if (_device.d_treePmRadialMassHistogram != nullptr) {
        return true;
    }
    _device.d_treePmRadialMassHistogram = static_cast<float*>(
        bltzr_x::MemoryPool::allocate(kTreePmConcentrationBinCount * sizeof(float)));
    return _device.d_treePmRadialMassHistogram != nullptr;
}

bool ParticleSystem::ensureTreePmScratchCapacity(int gridCells, int gridSize)
{
    if (!_device._cudaRuntimeAvailable || gridCells <= 0 || gridSize <= 0) {
        return false;
    }
    const std::size_t cells = static_cast<std::size_t>(gridCells);
    const std::size_t spectrumCells = static_cast<std::size_t>(gridSize) *
                                      static_cast<std::size_t>(gridSize) *
                                      static_cast<std::size_t>(gridSize / 2 + 1);
    const std::size_t maskWords = (cells + 31u) / 32u;
    if (_device.d_treePmCapacity >= cells && _device.d_treePmMaskWordCapacity >= maskWords &&
        _device.d_treePmSpectrumCapacity >= spectrumCells && _device.d_treePmCellMask != nullptr &&
        _device.d_treePmSpectrumZ != nullptr) {
        return true;
    }

    bltzr_x::MemoryPool::deallocate(_device.d_treePmDensity);
    bltzr_x::MemoryPool::deallocate(_device.d_treePmPotentialA);
    bltzr_x::MemoryPool::deallocate(_device.d_treePmPotentialB);
    bltzr_x::MemoryPool::deallocate(_device.d_treePmAccelX);
    bltzr_x::MemoryPool::deallocate(_device.d_treePmAccelY);
    bltzr_x::MemoryPool::deallocate(_device.d_treePmAccelZ);
    bltzr_x::MemoryPool::deallocate(_device.d_treePmSpectrum);
    bltzr_x::MemoryPool::deallocate(_device.d_treePmSpectrumX);
    bltzr_x::MemoryPool::deallocate(_device.d_treePmSpectrumY);
    bltzr_x::MemoryPool::deallocate(_device.d_treePmSpectrumZ);
    bltzr_x::MemoryPool::deallocate(_device.d_treePmCellMask);
    _device.d_treePmDensity = nullptr;
    _device.d_treePmPotentialA = nullptr;
    _device.d_treePmPotentialB = nullptr;
    _device.d_treePmAccelX = nullptr;
    _device.d_treePmAccelY = nullptr;
    _device.d_treePmAccelZ = nullptr;
    _device.d_treePmSpectrum = nullptr;
    _device.d_treePmSpectrumX = nullptr;
    _device.d_treePmSpectrumY = nullptr;
    _device.d_treePmSpectrumZ = nullptr;
    _device.d_treePmCellMask = nullptr;
    _device.d_treePmCapacity = 0u;
    _device.d_treePmSpectrumCapacity = 0u;
    _device.d_treePmMaskWordCapacity = 0u;

    _device.d_treePmDensity =
        static_cast<float*>(bltzr_x::MemoryPool::allocate(cells * sizeof(float)));
    _device.d_treePmPotentialA =
        static_cast<float*>(bltzr_x::MemoryPool::allocate(cells * sizeof(float)));
    _device.d_treePmPotentialB =
        static_cast<float*>(bltzr_x::MemoryPool::allocate(cells * sizeof(float)));
    _device.d_treePmAccelX =
        static_cast<float*>(bltzr_x::MemoryPool::allocate(cells * sizeof(float)));
    _device.d_treePmAccelY =
        static_cast<float*>(bltzr_x::MemoryPool::allocate(cells * sizeof(float)));
    _device.d_treePmAccelZ =
        static_cast<float*>(bltzr_x::MemoryPool::allocate(cells * sizeof(float)));
    _device.d_treePmSpectrum =
        bltzr_x::MemoryPool::allocate(spectrumCells * sizeof(cufftComplex));
    _device.d_treePmSpectrumX =
        bltzr_x::MemoryPool::allocate(spectrumCells * sizeof(cufftComplex));
    _device.d_treePmSpectrumY =
        bltzr_x::MemoryPool::allocate(spectrumCells * sizeof(cufftComplex));
    _device.d_treePmSpectrumZ =
        bltzr_x::MemoryPool::allocate(spectrumCells * sizeof(cufftComplex));
    _device.d_treePmCellMask =
        static_cast<unsigned int*>(bltzr_x::MemoryPool::allocate(maskWords * sizeof(unsigned int)));
    if (!_device.d_treePmDensity || !_device.d_treePmPotentialA || !_device.d_treePmPotentialB ||
        !_device.d_treePmAccelX || !_device.d_treePmAccelY || !_device.d_treePmAccelZ ||
        !_device.d_treePmSpectrum || !_device.d_treePmSpectrumX || !_device.d_treePmSpectrumY ||
        !_device.d_treePmSpectrumZ || !_device.d_treePmCellMask) {
        bltzr_x::MemoryPool::deallocate(_device.d_treePmDensity);
        bltzr_x::MemoryPool::deallocate(_device.d_treePmPotentialA);
        bltzr_x::MemoryPool::deallocate(_device.d_treePmPotentialB);
        bltzr_x::MemoryPool::deallocate(_device.d_treePmAccelX);
        bltzr_x::MemoryPool::deallocate(_device.d_treePmAccelY);
        bltzr_x::MemoryPool::deallocate(_device.d_treePmAccelZ);
        bltzr_x::MemoryPool::deallocate(_device.d_treePmSpectrum);
        bltzr_x::MemoryPool::deallocate(_device.d_treePmSpectrumX);
        bltzr_x::MemoryPool::deallocate(_device.d_treePmSpectrumY);
        bltzr_x::MemoryPool::deallocate(_device.d_treePmSpectrumZ);
        bltzr_x::MemoryPool::deallocate(_device.d_treePmCellMask);
        _device.d_treePmDensity = nullptr;
        _device.d_treePmPotentialA = nullptr;
        _device.d_treePmPotentialB = nullptr;
        _device.d_treePmAccelX = nullptr;
        _device.d_treePmAccelY = nullptr;
        _device.d_treePmAccelZ = nullptr;
        _device.d_treePmSpectrum = nullptr;
        _device.d_treePmSpectrumX = nullptr;
        _device.d_treePmSpectrumY = nullptr;
        _device.d_treePmSpectrumZ = nullptr;
        _device.d_treePmCellMask = nullptr;
        return false;
    }

    _device.d_treePmCapacity = cells;
    _device.d_treePmSpectrumCapacity = spectrumCells;
    _device.d_treePmMaskWordCapacity = maskWords;
    return true;
}

bool ParticleSystem::ensureTreePmNeighborGridCapacity(int numParticles, int totalCells,
                                                       bool gatherParticles)
{
    if (!_device._cudaRuntimeAvailable || numParticles <= 0 || totalCells <= 0) {
        return false;
    }
    const std::size_t particleCapacity = static_cast<std::size_t>(numParticles);
    const bool missingParticleBuffers =
        _device.d_treePmNeighborParticleCapacity < particleCapacity || !_device.d_sphCellHash ||
        !_device.d_sphSortedIndex || !_device.d_treePmSortKeys ||
        !_device.d_treePmSortIndices || !_device.d_treePmSortedCellHash;
    if (missingParticleBuffers) {
        if (_device.d_sphCellHash) {
            bltzr_x::MemoryPool::deallocate(_device.d_sphCellHash);
            _device.d_sphCellHash = nullptr;
        }
        if (_device.d_sphSortedIndex) {
            bltzr_x::MemoryPool::deallocate(_device.d_sphSortedIndex);
            _device.d_sphSortedIndex = nullptr;
        }
        if (_device.d_treePmSortKeys) {
            bltzr_x::MemoryPool::deallocate(_device.d_treePmSortKeys);
            _device.d_treePmSortKeys = nullptr;
        }
        if (_device.d_treePmSortIndices) {
            bltzr_x::MemoryPool::deallocate(_device.d_treePmSortIndices);
            _device.d_treePmSortIndices = nullptr;
        }
        if (_device.d_treePmSortedCellHash) {
            bltzr_x::MemoryPool::deallocate(_device.d_treePmSortedCellHash);
            _device.d_treePmSortedCellHash = nullptr;
        }
        if (_device.d_treePmSortTempStorage) {
            bltzr_x::MemoryPool::deallocate(_device.d_treePmSortTempStorage);
            _device.d_treePmSortTempStorage = nullptr;
            _device.d_treePmSortTempCapacity = 0u;
        }
        const std::size_t bytes = particleCapacity * sizeof(int);
        _device.d_sphCellHash = static_cast<int*>(bltzr_x::MemoryPool::allocate(bytes));
        _device.d_sphSortedIndex = static_cast<int*>(bltzr_x::MemoryPool::allocate(bytes));
        _device.d_treePmSortKeys = static_cast<int*>(bltzr_x::MemoryPool::allocate(bytes));
        _device.d_treePmSortIndices = static_cast<int*>(bltzr_x::MemoryPool::allocate(bytes));
        _device.d_treePmSortedCellHash = static_cast<int*>(bltzr_x::MemoryPool::allocate(bytes));
        if (!_device.d_sphCellHash || !_device.d_sphSortedIndex || !_device.d_treePmSortKeys ||
            !_device.d_treePmSortIndices || !_device.d_treePmSortedCellHash) {
            releaseSphGridBuffers();
            return false;
        }
        _device.d_treePmNeighborParticleCapacity = particleCapacity;
    }

    if (_device.d_treePmSortTempStorage == nullptr) {
        cub::DoubleBuffer<int> keys(_device.d_sphCellHash, _device.d_treePmSortKeys);
        cub::DoubleBuffer<int> values(_device.d_sphSortedIndex, _device.d_treePmSortIndices);
        std::size_t tempBytes = 0u;
        if (!checkCudaStatus(
                cub::DeviceRadixSort::SortPairs(nullptr, tempBytes, keys, values, numParticles, 0,
                                                21),
                "treepm CUB radix sort temp query")) {
            return false;
        }
        _device.d_treePmSortTempStorage = bltzr_x::MemoryPool::allocate(tempBytes);
        if (_device.d_treePmSortTempStorage == nullptr) {
            return false;
        }
        _device.d_treePmSortTempCapacity = tempBytes;
    }

    if (gatherParticles &&
        (_device.d_treePmSortedParticleCapacity < particleCapacity ||
         !_device.d_treePmSortedPosX || !_device.d_treePmSortedPosY ||
         !_device.d_treePmSortedPosZ || !_device.d_treePmSortedMass)) {
        bltzr_x::MemoryPool::deallocate(_device.d_treePmSortedPosX);
        bltzr_x::MemoryPool::deallocate(_device.d_treePmSortedPosY);
        bltzr_x::MemoryPool::deallocate(_device.d_treePmSortedPosZ);
        bltzr_x::MemoryPool::deallocate(_device.d_treePmSortedMass);
        const std::size_t bytes = particleCapacity * sizeof(float);
        _device.d_treePmSortedPosX = static_cast<float*>(bltzr_x::MemoryPool::allocate(bytes));
        _device.d_treePmSortedPosY = static_cast<float*>(bltzr_x::MemoryPool::allocate(bytes));
        _device.d_treePmSortedPosZ = static_cast<float*>(bltzr_x::MemoryPool::allocate(bytes));
        _device.d_treePmSortedMass = static_cast<float*>(bltzr_x::MemoryPool::allocate(bytes));
        if (!_device.d_treePmSortedPosX || !_device.d_treePmSortedPosY ||
            !_device.d_treePmSortedPosZ || !_device.d_treePmSortedMass) {
            releaseSphGridBuffers();
            return false;
        }
        _device.d_treePmSortedParticleCapacity = particleCapacity;
    }

    const std::size_t cellCapacity = static_cast<std::size_t>(totalCells);
    if (_device.d_treePmNeighborCellCapacity < cellCapacity || !_device.d_sphCellStart ||
        !_device.d_sphCellEnd) {
        if (_device.d_sphCellStart) {
            bltzr_x::MemoryPool::deallocate(_device.d_sphCellStart);
            _device.d_sphCellStart = nullptr;
        }
        if (_device.d_sphCellEnd) {
            bltzr_x::MemoryPool::deallocate(_device.d_sphCellEnd);
            _device.d_sphCellEnd = nullptr;
        }
        const std::size_t bytes = cellCapacity * sizeof(int);
        _device.d_sphCellStart = static_cast<int*>(bltzr_x::MemoryPool::allocate(bytes));
        _device.d_sphCellEnd = static_cast<int*>(bltzr_x::MemoryPool::allocate(bytes));
        if (!_device.d_sphCellStart || !_device.d_sphCellEnd) {
            releaseSphGridBuffers();
            return false;
        }
        _device.d_treePmNeighborCellCapacity = cellCapacity;
    }
    return true;
}

bool ParticleSystem::buildTreePmNeighborGrid(ParticleSoAView currentView, int numParticles,
                                             const TreePmGridParams& grid)
{
    const bool gatherParticles = treePmGatherEnabled();
    if (!ensureTreePmNeighborGridCapacity(numParticles, grid.totalCells, gatherParticles)) {
        return false;
    }

    const int numBlocks =
        (numParticles + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    const bool useMorton = treePmMortonEnabled();
    treePmBuildCellHashKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
        currentView, numParticles, grid, _device.d_sphCellHash, _device.d_sphSortedIndex,
        useMorton ? 1 : 0);
    if (!checkCudaStatus(cudaGetLastError(), "treePmBuildCellHashKernel launch")) {
        return false;
    }

    cub::DoubleBuffer<int> keys(_device.d_sphCellHash, _device.d_treePmSortKeys);
    cub::DoubleBuffer<int> values(_device.d_sphSortedIndex, _device.d_treePmSortIndices);
    const int endBit = 21;
    if (!checkCudaStatus(
            cub::DeviceRadixSort::SortPairs(_device.d_treePmSortTempStorage,
                                            _device.d_treePmSortTempCapacity, keys, values,
                                            numParticles, 0, endBit),
            "treepm CUB radix sort")) {
        return false;
    }
    _device.d_sphCellHash = keys.Current();
    _device.d_treePmSortKeys = keys.Alternate();
    _device.d_sphSortedIndex = values.Current();
    _device.d_treePmSortIndices = values.Alternate();

    IndexConstHandle sortedHash = _device.d_sphCellHash;
    if (useMorton) {
        treePmBuildSortedCellHashKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView, _device.d_sphSortedIndex, _device.d_treePmSortedCellHash, numParticles,
            grid);
        if (!checkCudaStatus(cudaGetLastError(), "treePmBuildSortedCellHashKernel launch")) {
            return false;
        }
        sortedHash = _device.d_treePmSortedCellHash;
    }

    if (gatherParticles) {
        treePmGatherSortedParticlesKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView, _device.d_sphSortedIndex, _device.d_treePmSortedPosX,
            _device.d_treePmSortedPosY, _device.d_treePmSortedPosZ, _device.d_treePmSortedMass,
            numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "treePmGatherSortedParticlesKernel launch")) {
            return false;
        }
    }

    const int cellBlocks =
        (grid.totalCells + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    treePmResetCellBoundsKernel<<<cellBlocks, Particle::kDefaultCudaBlockSize>>>(
        _device.d_sphCellStart, _device.d_sphCellEnd, grid.totalCells);
    if (!checkCudaStatus(cudaGetLastError(), "treePmResetCellBoundsKernel launch")) {
        return false;
    }
    treePmFindCellBoundsKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
        sortedHash, _device.d_sphCellStart, _device.d_sphCellEnd, numParticles);
    if (!checkCudaStatus(cudaGetLastError(), "treePmFindCellBoundsKernel launch")) {
        return false;
    }
    return checkCudaStatus(cudaDeviceSynchronize(), "treepm neighbor grid sync");
}

bool ParticleSystem::buildTreePmFftField(const TreePmGridParams& grid)
{
    _device._treePmFftActive = false;
    const int gridSize = grid.gridSize;
    if (_device._treePmFftPlan == 0 || _device._treePmFftPlanGridSize != gridSize) {
        if (_device._treePmFftPlan != 0) {
            cufftDestroy(static_cast<cufftHandle>(_device._treePmFftPlan));
            _device._treePmFftPlan = 0;
        }
        if (_device._treePmFftInversePlan != 0) {
            cufftDestroy(static_cast<cufftHandle>(_device._treePmFftInversePlan));
            _device._treePmFftInversePlan = 0;
        }
        _device._treePmFftPlanGridSize = 0;
        cufftHandle plan = 0;
        if (!checkTreePmFftStatus(cufftPlan3d(&plan, gridSize, gridSize, gridSize, CUFFT_R2C),
                                  "cufftPlan3d")) {
            return false;
        }
        _device._treePmFftPlan = static_cast<int>(plan);
        cufftHandle inversePlan = 0;
        if (!checkTreePmFftStatus(
                cufftPlan3d(&inversePlan, gridSize, gridSize, gridSize, CUFFT_C2R),
                "cufftPlan3d inverse")) {
            cufftDestroy(plan);
            _device._treePmFftPlan = 0;
            return false;
        }
        _device._treePmFftInversePlan = static_cast<int>(inversePlan);
        _device._treePmFftPlanGridSize = gridSize;
    }

    auto* density = reinterpret_cast<cufftReal*>(_device.d_treePmDensity);
    auto* densitySpectrum = reinterpret_cast<cufftComplex*>(_device.d_treePmSpectrum);
    if (!checkTreePmFftStatus(
            cufftExecR2C(static_cast<cufftHandle>(_device._treePmFftPlan), density,
                         densitySpectrum),
            "cufftExecR2C")) {
        return false;
    }

    const int spectrumCells = gridSize * gridSize * (gridSize / 2 + 1);
    const int spectrumBlocks =
        (spectrumCells + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    treePmApplyFftKernel<<<spectrumBlocks, Particle::kDefaultCudaBlockSize>>>(
        densitySpectrum, reinterpret_cast<cufftComplex*>(_device.d_treePmSpectrumX), grid,
        std::max(_octreeSoftening, _physicsMinSoftening));
    if (!checkCudaStatus(cudaGetLastError(), "treePmApplyFftKernel launch")) {
        return false;
    }

    const cufftHandle inversePlan = static_cast<cufftHandle>(_device._treePmFftInversePlan);
    if (!checkTreePmFftStatus(
            cufftExecC2R(inversePlan, reinterpret_cast<cufftComplex*>(_device.d_treePmSpectrumX),
                         reinterpret_cast<cufftReal*>(_device.d_treePmPotentialA)),
            "cufftExecC2R potential")) {
        return false;
    }

    const int fieldBlocks =
        (grid.totalCells + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    treePmNormalizeFftFieldKernel<<<fieldBlocks, Particle::kDefaultCudaBlockSize>>>(
        _device.d_treePmPotentialA, grid.totalCells);
    treePmPotentialGradientKernel<<<fieldBlocks, Particle::kDefaultCudaBlockSize>>>(
        _device.d_treePmPotentialA, _device.d_treePmAccelX, _device.d_treePmAccelY,
        _device.d_treePmAccelZ, grid);
    if (!checkCudaStatus(cudaGetLastError(), "treepm potential gradient launch")) {
        return false;
    }
    if (!checkCudaStatus(cudaDeviceSynchronize(), "treepm FFT field sync")) {
        return false;
    }
    _device._treePmFftActive = true;
    return true;
}

bool ParticleSystem::captureTreePmGraph(int slot, ParticleSoAView currentView,
                                        ParticleSoAView nextView, int numParticles,
                                        int particleLimit, const TreePmGridParams& grid,
                                        float cutoffSquared, ForceLawPolicy forceLaw,
                                        float deltaTime, float maxAcceleration)
{
    if (slot < 0 || slot > 1 || numParticles <= 0 || particleLimit <= 0 ||
        !_device._treePmFftActive || _device._treePmFftPlan == 0 ||
        _device._treePmFftInversePlan == 0) {
        return false;
    }
    cudaGraph_t graph = nullptr;
    cudaGraphExec_t executable = nullptr;
    cudaStream_t stream = static_cast<cudaStream_t>(_device._treePmGraphStream);
    if (stream == nullptr &&
        cudaStreamCreateWithFlags(&stream, cudaStreamNonBlocking) == cudaSuccess) {
        _device._treePmGraphStream = stream;
    }
    if (stream == nullptr) {
        return false;
    }
    const cufftHandle forwardPlan = static_cast<cufftHandle>(_device._treePmFftPlan);
    const cufftHandle inversePlan = static_cast<cufftHandle>(_device._treePmFftInversePlan);
    if (cufftSetStream(forwardPlan, stream) != CUFFT_SUCCESS ||
        cufftSetStream(inversePlan, stream) != CUFFT_SUCCESS) {
        cufftSetStream(forwardPlan, nullptr);
        cufftSetStream(inversePlan, nullptr);
        return false;
    }
    if (cudaStreamBeginCapture(stream, cudaStreamCaptureModeGlobal) != cudaSuccess) {
        cufftSetStream(forwardPlan, nullptr);
        cufftSetStream(inversePlan, nullptr);
        return false;
    }
    auto abortCapture = [&]() {
        cudaGraph_t abandoned = nullptr;
        cudaStreamEndCapture(stream, &abandoned);
        if (abandoned != nullptr) {
            cudaGraphDestroy(abandoned);
        }
        cufftSetStream(forwardPlan, nullptr);
        cufftSetStream(inversePlan, nullptr);
        return false;
    };

    const std::size_t densityBytes = static_cast<std::size_t>(grid.totalCells) * sizeof(float);
    const std::size_t maskBytes =
        (static_cast<std::size_t>(grid.totalCells) + 31u) / 32u * sizeof(unsigned int);
    if (cudaMemsetAsync(_device.d_treePmDensity, 0, densityBytes, stream) != cudaSuccess ||
        cudaMemsetAsync(_device.d_treePmCellMask, 0, maskBytes, stream) != cudaSuccess) {
        return abortCapture();
    }
    const int depositBlocks =
        (particleLimit + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    treePmDepositMassKernel<<<depositBlocks, Particle::kDefaultCudaBlockSize, 0, stream>>>(
        currentView, numParticles, particleLimit, grid, _device.d_treePmDensity,
        _device.d_treePmCellMask);
    if (cudaGetLastError() != cudaSuccess) {
        return abortCapture();
    }

    if (cufftExecR2C(forwardPlan, reinterpret_cast<cufftReal*>(_device.d_treePmDensity),
                     reinterpret_cast<cufftComplex*>(_device.d_treePmSpectrum)) != CUFFT_SUCCESS) {
        return abortCapture();
    }

    const int spectrumCells = grid.gridSize * grid.gridSize * (grid.gridSize / 2 + 1);
    const int spectrumBlocks =
        (spectrumCells + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    treePmApplyFftKernel<<<spectrumBlocks, Particle::kDefaultCudaBlockSize, 0, stream>>>(
        reinterpret_cast<const cufftComplex*>(_device.d_treePmSpectrum),
        reinterpret_cast<cufftComplex*>(_device.d_treePmSpectrumX), grid,
        std::max(_octreeSoftening, _physicsMinSoftening));
    if (cudaGetLastError() != cudaSuccess ||
        cufftExecC2R(inversePlan, reinterpret_cast<cufftComplex*>(_device.d_treePmSpectrumX),
                     reinterpret_cast<cufftReal*>(_device.d_treePmPotentialA)) != CUFFT_SUCCESS) {
        return abortCapture();
    }

    const int fieldBlocks =
        (grid.totalCells + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    treePmNormalizeFftFieldKernel<<<fieldBlocks, Particle::kDefaultCudaBlockSize, 0, stream>>>(
        _device.d_treePmPotentialA, grid.totalCells);
    treePmPotentialGradientKernel<<<fieldBlocks, Particle::kDefaultCudaBlockSize, 0, stream>>>(
        _device.d_treePmPotentialA, _device.d_treePmAccelX, _device.d_treePmAccelY,
        _device.d_treePmAccelZ, grid);
    const int updateBlocks =
        (numParticles + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    updateParticlesTreePmLocalGridKernel<<<updateBlocks, Particle::kDefaultCudaBlockSize, 0,
                                           stream>>>(
        currentView, nextView, numParticles, grid, nullptr, nullptr, nullptr, forceLaw, deltaTime,
        maxAcceleration, _device.d_treePmAccelX, _device.d_treePmAccelY, _device.d_treePmAccelZ,
        _device.d_treePmCellMask, cutoffSquared, 1, 0, nullptr, nullptr, nullptr, nullptr);
    const cudaError_t launchStatus = cudaGetLastError();
    const cudaError_t captureStatus = cudaStreamEndCapture(stream, &graph);
    if (launchStatus != cudaSuccess || captureStatus != cudaSuccess || graph == nullptr ||
        cudaGraphInstantiate(&executable, graph, nullptr, nullptr, 0) != cudaSuccess) {
        if (graph != nullptr) {
            cudaGraphDestroy(graph);
        }
        if (executable != nullptr) {
            cudaGraphExecDestroy(executable);
        }
        cufftSetStream(forwardPlan, nullptr);
        cufftSetStream(inversePlan, nullptr);
        return false;
    }
    cudaGraphDestroy(graph);
    cufftSetStream(forwardPlan, nullptr);
    cufftSetStream(inversePlan, nullptr);
    if (_device._treePmGraphExec[slot] != nullptr) {
        cudaGraphExecDestroy(static_cast<cudaGraphExec_t>(_device._treePmGraphExec[slot]));
    }
    _device._treePmGraphExec[slot] = executable;
    _device._treePmGraphCaptured[slot] = true;
    return true;
}

bool ParticleSystem::launchTreePmGraph(int slot)
{
    if (slot < 0 || slot > 1 || !_device._treePmGraphCaptured[slot] ||
        _device._treePmGraphExec[slot] == nullptr) {
        return false;
    }
    return checkCudaStatus(
        cudaGraphLaunch(static_cast<cudaGraphExec_t>(_device._treePmGraphExec[slot]), nullptr),
        "cudaGraphLaunch(treepm)");
}

void ParticleSystem::releaseTreePmGraph()
{
    for (int slot = 0; slot < 2; ++slot) {
        if (_device._treePmGraphExec[slot] != nullptr) {
            cudaGraphExecDestroy(static_cast<cudaGraphExec_t>(_device._treePmGraphExec[slot]));
            _device._treePmGraphExec[slot] = nullptr;
        }
        _device._treePmGraphCaptured[slot] = false;
    }
    if (_device._treePmGraphStream != nullptr) {
        cudaStreamDestroy(static_cast<cudaStream_t>(_device._treePmGraphStream));
        _device._treePmGraphStream = nullptr;
    }
    _device._treePmGraphSlot = 0;
}

bool ParticleSystem::buildTreePmGrid(ParticleSoAView currentView, int numParticles,
                                     TreePmGridParams* outGrid, float* outCutoffSquared)
{
    if (!_device._cudaRuntimeAvailable || !outGrid || !outCutoffSquared || numParticles <= 0 ||
        !_device.d_soaPosX || !_device.d_soaPosY || !_device.d_soaPosZ) {
        return false;
    }

    if (isComovingCosmology(_cosmology)) {
        const int gridSize = std::clamp(_treePmGridSize, 32, 128);
        const int totalCells = gridSize * gridSize * gridSize;
        const float boxLength = 2.0f * _cosmology.boxHalfExtent;
        if (boxLength <= 0.0f || !ensureTreePmScratchCapacity(totalCells, gridSize)) {
            return false;
        }
        float totalMass = 0.0f;
        for (const Particle& particle : _particles) {
            totalMass += particle.getMass();
        }
        if (totalMass <= 0.0f) {
            return false;
        }
        if (!checkCudaStatus(cudaMemset(_device.d_treePmDensity, 0,
                                        static_cast<std::size_t>(totalCells) * sizeof(float)),
                             "cudaMemset(cosmology density)") ||
            !checkCudaStatus(cudaMemset(_device.d_treePmPotentialA, 0,
                                        static_cast<std::size_t>(totalCells) * sizeof(float)),
                             "cudaMemset(cosmology potential)")) {
            return false;
        }
        const std::size_t maskWords = (static_cast<std::size_t>(totalCells) + 31u) / 32u;
        if (!checkCudaStatus(cudaMemset(_device.d_treePmCellMask, 0,
                                        maskWords * sizeof(unsigned int)),
                             "cudaMemset(cosmology cell mask)")) {
            return false;
        }
        TreePmGridParams grid{};
        grid.gridSize = gridSize;
        grid.totalCells = totalCells;
        grid.assignment = 1; // TSC is mandatory for the cosmology transfer function.
        grid.periodic = 1;
        grid.cellSize = boxLength / static_cast<float>(gridSize);
        grid.invCellSize = 1.0f / grid.cellSize;
        grid.originX = 0.0f;
        grid.originY = 0.0f;
        grid.originZ = 0.0f;
        grid.shortRangeScale = 0.0f;
        grid.densityScale = boxLength * boxLength * boxLength / totalMass;
        grid.poissonCoefficient = 1.5f * _cosmology.hubbleH0 * _cosmology.hubbleH0 *
                                  _cosmology.omegaMatter /
                                  std::max(_cosmologyScaleFactor, 1.0e-6f);
        const int blocks = (numParticles + Particle::kDefaultCudaBlockSize - 1) /
                           Particle::kDefaultCudaBlockSize;
        treePmDepositMassKernel<<<blocks, Particle::kDefaultCudaBlockSize>>>(
            currentView, numParticles, numParticles, grid, _device.d_treePmDensity,
            _device.d_treePmCellMask);
        treePmBuildDensityContrastKernel<<<(totalCells + Particle::kDefaultCudaBlockSize - 1) /
                                              Particle::kDefaultCudaBlockSize,
                                          Particle::kDefaultCudaBlockSize>>>(
            _device.d_treePmDensity, totalCells);
        if (!checkCudaStatus(cudaGetLastError(), "cosmology TSC deposit launch") ||
            !buildTreePmFftField(grid)) {
            return false;
        }
        _device._treePmGridSize = gridSize;
        _device._treePmTotalCells = totalCells;
        *outGrid = grid;
        *outCutoffSquared = 0.0f;
        return true;
    }

    if (!ensureTreePmBoundsCapacity(numParticles)) {
        return false;
    }
    const int boundsBlocks =
        (numParticles + kTreePmBoundsBlockSize - 1) / kTreePmBoundsBlockSize;
    treePmReduceBoundsKernel<<<boundsBlocks, kTreePmBoundsBlockSize>>>(
        currentView, numParticles, _device.d_treePmBoundsPartial);
    if (!checkCudaStatus(cudaGetLastError(), "treePmReduceBoundsKernel launch")) {
        return false;
    }
    treePmFinalizeBoundsKernel<<<1, kTreePmBoundsBlockSize>>>(
        _device.d_treePmBoundsPartial, boundsBlocks, _device.d_treePmBounds);
    if (!checkCudaStatus(cudaGetLastError(), "treePmFinalizeBoundsKernel launch")) {
        return false;
    }
    float bounds[kTreePmBoundsFieldCount]{};
    if (!checkCudaStatus(cudaMemcpy(bounds, _device.d_treePmBounds,
                                    sizeof(bounds), cudaMemcpyDeviceToHost),
                         "cudaMemcpy(treepm bounds)")) {
        return false;
    }
    const float minX = bounds[0];
    const float minY = bounds[1];
    const float minZ = bounds[2];
    const float maxX = bounds[3];
    const float maxY = bounds[4];
    const float maxZ = bounds[5];
    const float totalMass = bounds[6];
    const Vector3 weightedCenter(bounds[7], bounds[8], bounds[9]);

    if (treePmLayoutMode() == kTreePmLayoutAuto && !_device._treePmAutoLayoutResolved) {
        bool autoLayoutReady = false;
        if (ensureTreePmConcentrationCapacity()) {
            if (!checkCudaStatus(
                    cudaMemset(_device.d_treePmRadialMassHistogram, 0,
                               kTreePmConcentrationBinCount * sizeof(float)),
                    "cudaMemset(treepm concentration histogram)")) {
                return false;
            }
            const int histogramBlocks =
                (numParticles + Particle::kDefaultCudaBlockSize - 1) /
                Particle::kDefaultCudaBlockSize;
            treePmRadialMassHistogramKernel<<<histogramBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, numParticles, _device.d_treePmBounds,
                _device.d_treePmRadialMassHistogram);
            if (!checkCudaStatus(cudaGetLastError(),
                                 "treePmRadialMassHistogramKernel launch") ||
                !checkCudaStatus(cudaDeviceSynchronize(),
                                 "treePmRadialMassHistogramKernel sync")) {
                return false;
            }
            float histogram[kTreePmConcentrationBinCount]{};
            if (!checkCudaStatus(
                    cudaMemcpy(histogram, _device.d_treePmRadialMassHistogram,
                               sizeof(histogram), cudaMemcpyDeviceToHost),
                    "cudaMemcpy(treepm concentration histogram)")) {
                return false;
            }

            const float targetMass = std::max(0.0f, 0.8f * totalMass);
            float cumulativeMass = 0.0f;
            int r80Bin = kTreePmConcentrationBinCount - 1;
            for (int bin = 0; bin < kTreePmConcentrationBinCount; ++bin) {
                cumulativeMass += histogram[bin];
                if (cumulativeMass >= targetMass) {
                    r80Bin = bin;
                    break;
                }
            }
            const float r80Ratio = static_cast<float>(r80Bin + 1) /
                                   static_cast<float>(kTreePmConcentrationBinCount);
            const float threshold = std::clamp(
                parseFloatEnv("BLITZAR_TREEPM_AUTO_R80_THRESHOLD", 0.35f), 0.05f, 0.95f);
            _device._treePmAutoR80Ratio = r80Ratio;
            _device._treePmAutoGather = r80Ratio >= threshold;
            _device._treePmAutoMorton = _device._treePmAutoGather;
            autoLayoutReady = true;
            fprintf(stderr,
                    "[treepm] auto_layout r80_ratio=%.4f threshold=%.4f selection=%s\n",
                    r80Ratio, threshold,
                    _device._treePmAutoGather ? "gather_morton" : "linear");
        }
        if (!autoLayoutReady) {
            _device._treePmAutoR80Ratio = -1.0f;
            _device._treePmAutoGather = false;
            _device._treePmAutoMorton = false;
            fprintf(stderr, "[treepm] auto_layout concentration_unavailable fallback=linear\n");
        }
        _device._treePmAutoLayoutResolved = true;
    }

    const int requestedGridSize = std::clamp(_treePmGridSize, 32, 128);
    const float extent = std::max({maxX - minX, maxY - minY, maxZ - minZ, _octreeSoftening});
    const float cellSize = std::max(0.25f, extent / static_cast<float>(requestedGridSize - 2));
    // Double the physical domain for the FFT so the periodic convolution is isolated
    // from the particle region instead of wrapping through the opposite face.
    const int gridSize = requestedGridSize * 2;
    const int totalCells = gridSize * gridSize * gridSize;
    const float invCellSize = 1.0f / cellSize;

    if (!ensureTreePmScratchCapacity(totalCells, gridSize)) {
        return false;
    }

    if (!checkCudaStatus(cudaMemset(_device.d_treePmDensity, 0,
                                    static_cast<std::size_t>(totalCells) * sizeof(float)),
                         "cudaMemset(treepm density)")) {
        return false;
    }
    if (!checkCudaStatus(cudaMemset(_device.d_treePmPotentialA, 0,
                                    static_cast<std::size_t>(totalCells) * sizeof(float)),
                         "cudaMemset(treepm potential A)")) {
        return false;
    }
    if (!checkCudaStatus(cudaMemset(_device.d_treePmPotentialB, 0,
                                    static_cast<std::size_t>(totalCells) * sizeof(float)),
                         "cudaMemset(treepm potential B)")) {
        return false;
    }
    if (!checkCudaStatus(cudaMemset(_device.d_treePmAccelX, 0,
                                    static_cast<std::size_t>(totalCells) * sizeof(float)),
                         "cudaMemset(treepm accel X)")) {
        return false;
    }
    if (!checkCudaStatus(cudaMemset(_device.d_treePmAccelY, 0,
                                    static_cast<std::size_t>(totalCells) * sizeof(float)),
                         "cudaMemset(treepm accel Y)")) {
        return false;
    }
    if (!checkCudaStatus(cudaMemset(_device.d_treePmAccelZ, 0,
                                    static_cast<std::size_t>(totalCells) * sizeof(float)),
                         "cudaMemset(treepm accel Z)")) {
        return false;
    }
    const std::size_t maskWords = (static_cast<std::size_t>(totalCells) + 31u) / 32u;
    if (!checkCudaStatus(cudaMemset(_device.d_treePmCellMask, 0, maskWords * sizeof(unsigned int)),
                         "cudaMemset(treepm cell mask)")) {
        return false;
    }

    TreePmGridParams grid{};
    grid.gridSize = gridSize;
    grid.totalCells = totalCells;
    grid.assignment = _treePmAssignment == "tsc" ? 1 : _treePmAssignment == "pcs" ? 2 : 0;
    grid.periodic = 0;
    grid.cellSize = cellSize;
    grid.invCellSize = invCellSize;
    // Center the cubic mesh on the particle bounds; anchoring every axis at its minimum
    // places thin or planar scenes against a Dirichlet boundary and creates false forces.
    const float halfGridExtent = 0.5f * static_cast<float>(gridSize) * cellSize;
    grid.originX = 0.5f * (minX + maxX) - halfGridExtent;
    grid.originY = 0.5f * (minY + maxY) - halfGridExtent;
    grid.originZ = 0.5f * (minZ + maxZ) - halfGridExtent;
    const float inverseMass = totalMass > 0.0f ? 1.0f / totalMass : 0.0f;
    const Vector3 centerOfMass = weightedCenter * inverseMass;
    grid.boundaryMass = totalMass;
    grid.boundaryCenterX = centerOfMass.x;
    grid.boundaryCenterY = centerOfMass.y;
    grid.boundaryCenterZ = centerOfMass.z;
    grid.boundarySoftening = std::max(_octreeSoftening, _physicsMinSoftening);
    const float cutoffFactor = std::clamp(_treePmCutoffFactor, 1.0f, 2.0f);
    const float cutoff = cutoffFactor * cellSize;
    grid.shortRangeScale = cutoff / 4.5f;
    grid.densityScale = 1.0f;
    grid.poissonCoefficient = 0.0f;

    // A PM solve must deposit every mass source. A partial deposit breaks the
    // Poisson problem and cannot be repaired by the short-range correction.
    const int particleLimit = numParticles;
    const int numBlocks =
        (particleLimit + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    treePmDepositMassKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
        currentView, numParticles, particleLimit, grid, _device.d_treePmDensity,
        _device.d_treePmCellMask);
    if (!checkCudaStatus(cudaGetLastError(), "treePmDepositMassKernel launch")) {
        return false;
    }

    if (buildTreePmFftField(grid)) {
        _device._treePmGridSize = gridSize;
        _device._treePmTotalCells = totalCells;
        *outGrid = grid;
        *outCutoffSquared = cutoff * cutoff;
        return true;
    }
    fprintf(stderr, "[treepm] FFT field unavailable; using red-black finite-difference fallback\n");

    const int gridBlocks =
        (totalCells + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    const int iterationCount = std::clamp(_treePmJacobiIterations, 4, 64);
    float* currentPotential = _device.d_treePmPotentialA;
    for (int iteration = 0; iteration < iterationCount; ++iteration) {
        for (int parity = 0; parity < 2; ++parity) {
            treePmRedBlackStepKernel<<<gridBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentPotential, _device.d_treePmDensity, grid, parity);
            if (!checkCudaStatus(cudaGetLastError(), "treePmRedBlackStepKernel launch")) {
                return false;
            }
        }
    }

    treePmPotentialGradientKernel<<<gridBlocks, Particle::kDefaultCudaBlockSize>>>(
        currentPotential, _device.d_treePmAccelX, _device.d_treePmAccelY, _device.d_treePmAccelZ,
        grid);
    if (!checkCudaStatus(cudaGetLastError(), "treePmPotentialGradientKernel launch")) {
        return false;
    }
    if (!checkCudaStatus(cudaDeviceSynchronize(), "treepm mesh solve sync")) {
        return false;
    }

    _device._treePmGridSize = gridSize;
    _device._treePmTotalCells = totalCells;
    *outGrid = grid;
    *outCutoffSquared = cutoff * cutoff;
    return true;
}
