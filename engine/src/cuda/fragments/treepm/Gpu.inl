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

namespace {

__device__ __forceinline__ int treePmGridIndex(int x, int y, int z, const TreePmGridParams& grid)
{
    return (z * grid.gridSize + y) * grid.gridSize + x;
}

__device__ __forceinline__ void treePmSetCellMask(unsigned int* cellMask, int cell)
{
    atomicOr(&cellMask[cell >> 5], 1u << (cell & 31));
}

__device__ __forceinline__ bool treePmCellMaskEnabled(const unsigned int* cellMask, int cell)
{
    return (cellMask[cell >> 5] & (1u << (cell & 31))) != 0u;
}

__device__ __forceinline__ float treePmSampleField(const float* field, const TreePmGridParams& grid,
                                                   const Vector3& pos)
{
    const float scaledX = (pos.x - grid.originX) * grid.invCellSize;
    const float scaledY = (pos.y - grid.originY) * grid.invCellSize;
    const float scaledZ = (pos.z - grid.originZ) * grid.invCellSize;

    const float clampedX = fminf(fmaxf(scaledX, 0.0f), static_cast<float>(grid.gridSize - 1));
    const float clampedY = fminf(fmaxf(scaledY, 0.0f), static_cast<float>(grid.gridSize - 1));
    const float clampedZ = fminf(fmaxf(scaledZ, 0.0f), static_cast<float>(grid.gridSize - 1));

    const int x0 = min(max(static_cast<int>(floorf(clampedX)), 0), grid.gridSize - 1);
    const int y0 = min(max(static_cast<int>(floorf(clampedY)), 0), grid.gridSize - 1);
    const int z0 = min(max(static_cast<int>(floorf(clampedZ)), 0), grid.gridSize - 1);
    const int x1 = min(x0 + 1, grid.gridSize - 1);
    const int y1 = min(y0 + 1, grid.gridSize - 1);
    const int z1 = min(z0 + 1, grid.gridSize - 1);

    const float tx = clampedX - static_cast<float>(x0);
    const float ty = clampedY - static_cast<float>(y0);
    const float tz = clampedZ - static_cast<float>(z0);

    const int i000 = treePmGridIndex(x0, y0, z0, grid);
    const int i100 = treePmGridIndex(x1, y0, z0, grid);
    const int i010 = treePmGridIndex(x0, y1, z0, grid);
    const int i110 = treePmGridIndex(x1, y1, z0, grid);
    const int i001 = treePmGridIndex(x0, y0, z1, grid);
    const int i101 = treePmGridIndex(x1, y0, z1, grid);
    const int i011 = treePmGridIndex(x0, y1, z1, grid);
    const int i111 = treePmGridIndex(x1, y1, z1, grid);

    const float c00 = field[i000] * (1.0f - tx) + field[i100] * tx;
    const float c10 = field[i010] * (1.0f - tx) + field[i110] * tx;
    const float c01 = field[i001] * (1.0f - tx) + field[i101] * tx;
    const float c11 = field[i011] * (1.0f - tx) + field[i111] * tx;
    const float c0 = c00 * (1.0f - ty) + c10 * ty;
    const float c1 = c01 * (1.0f - ty) + c11 * ty;
    return c0 * (1.0f - tz) + c1 * tz;
}

__device__ __forceinline__ Vector3 treePmSampleAcceleration(const TreePmGridParams& grid,
                                                            const Vector3& pos,
                                                            const float* accelX,
                                                            const float* accelY,
                                                            const float* accelZ)
{
    return Vector3(treePmSampleField(accelX, grid, pos), treePmSampleField(accelY, grid, pos),
                   treePmSampleField(accelZ, grid, pos));
}

__global__ void treePmDepositMassKernel(ParticleSoAView state, int numParticles,
                                        TreePmGridParams grid, float* density,
                                        unsigned int* cellMask)
{
    const int particleIndex = blockIdx.x * blockDim.x + threadIdx.x;
    if (particleIndex >= numParticles) {
        return;
    }

    const Vector3 pos = octreeLoadParticlePosition(state, particleIndex);
    const float scaledX = (pos.x - grid.originX) * grid.invCellSize;
    const float scaledY = (pos.y - grid.originY) * grid.invCellSize;
    const float scaledZ = (pos.z - grid.originZ) * grid.invCellSize;

    const float clampedX = fminf(fmaxf(scaledX, 0.0f), static_cast<float>(grid.gridSize - 1));
    const float clampedY = fminf(fmaxf(scaledY, 0.0f), static_cast<float>(grid.gridSize - 1));
    const float clampedZ = fminf(fmaxf(scaledZ, 0.0f), static_cast<float>(grid.gridSize - 1));

    const int x0 = min(max(static_cast<int>(floorf(clampedX)), 0), grid.gridSize - 1);
    const int y0 = min(max(static_cast<int>(floorf(clampedY)), 0), grid.gridSize - 1);
    const int z0 = min(max(static_cast<int>(floorf(clampedZ)), 0), grid.gridSize - 1);
    const int x1 = min(x0 + 1, grid.gridSize - 1);
    const int y1 = min(y0 + 1, grid.gridSize - 1);
    const int z1 = min(z0 + 1, grid.gridSize - 1);

    const float tx = clampedX - static_cast<float>(x0);
    const float ty = clampedY - static_cast<float>(y0);
    const float tz = clampedZ - static_cast<float>(z0);
    const float oneMinusTx = 1.0f - tx;
    const float oneMinusTy = 1.0f - ty;
    const float oneMinusTz = 1.0f - tz;
    const float volumeInv = 1.0f / (grid.cellSize * grid.cellSize * grid.cellSize);
    const float massDensity = octreeLoadParticleMass(state, particleIndex) * volumeInv;

    const float w000 = oneMinusTx * oneMinusTy * oneMinusTz;
    const float w100 = tx * oneMinusTy * oneMinusTz;
    const float w010 = oneMinusTx * ty * oneMinusTz;
    const float w110 = tx * ty * oneMinusTz;
    const float w001 = oneMinusTx * oneMinusTy * tz;
    const float w101 = tx * oneMinusTy * tz;
    const float w011 = oneMinusTx * ty * tz;
    const float w111 = tx * ty * tz;

    const int i000 = treePmGridIndex(x0, y0, z0, grid);
    const int i100 = treePmGridIndex(x1, y0, z0, grid);
    const int i010 = treePmGridIndex(x0, y1, z0, grid);
    const int i110 = treePmGridIndex(x1, y1, z0, grid);
    const int i001 = treePmGridIndex(x0, y0, z1, grid);
    const int i101 = treePmGridIndex(x1, y0, z1, grid);
    const int i011 = treePmGridIndex(x0, y1, z1, grid);
    const int i111 = treePmGridIndex(x1, y1, z1, grid);

    atomicAdd(&density[i000], massDensity * w000);
    atomicAdd(&density[i100], massDensity * w100);
    atomicAdd(&density[i010], massDensity * w010);
    atomicAdd(&density[i110], massDensity * w110);
    atomicAdd(&density[i001], massDensity * w001);
    atomicAdd(&density[i101], massDensity * w101);
    atomicAdd(&density[i011], massDensity * w011);
    atomicAdd(&density[i111], massDensity * w111);

    treePmSetCellMask(cellMask, i000);
    treePmSetCellMask(cellMask, i100);
    treePmSetCellMask(cellMask, i010);
    treePmSetCellMask(cellMask, i110);
    treePmSetCellMask(cellMask, i001);
    treePmSetCellMask(cellMask, i101);
    treePmSetCellMask(cellMask, i011);
    treePmSetCellMask(cellMask, i111);
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
        potentialOut[index] = 0.0f;
        return;
    }

    const int xm = treePmGridIndex(x - 1, y, z, grid);
    const int xp = treePmGridIndex(x + 1, y, z, grid);
    const int ym = treePmGridIndex(x, y - 1, z, grid);
    const int yp = treePmGridIndex(x, y + 1, z, grid);
    const int zm = treePmGridIndex(x, y, z - 1, grid);
    const int zp = treePmGridIndex(x, y, z + 1, grid);
    const float h2 = grid.cellSize * grid.cellSize;
    const float rhs = -density[index] * h2;
    potentialOut[index] = (potentialIn[xm] + potentialIn[xp] + potentialIn[ym] + potentialIn[yp] +
                           potentialIn[zm] + potentialIn[zp] + rhs) * (1.0f / 6.0f);
}

__global__ void treePmPotentialGradientKernel(const float* potential, float* accelX,
                                              float* accelY, float* accelZ,
                                              TreePmGridParams grid)
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
        accelX[index] = 0.0f;
        accelY[index] = 0.0f;
        accelZ[index] = 0.0f;
        return;
    }

    const int xm = treePmGridIndex(x - 1, y, z, grid);
    const int xp = treePmGridIndex(x + 1, y, z, grid);
    const int ym = treePmGridIndex(x, y - 1, z, grid);
    const int yp = treePmGridIndex(x, y + 1, z, grid);
    const int zm = treePmGridIndex(x, y, z - 1, grid);
    const int zp = treePmGridIndex(x, y, z + 1, grid);
    const float invTwoH = 0.5f * grid.invCellSize;
    accelX[index] = -(potential[xp] - potential[xm]) * invTwoH;
    accelY[index] = -(potential[yp] - potential[ym]) * invTwoH;
    accelZ[index] = -(potential[zp] - potential[zm]) * invTwoH;
}

__global__ void treePmBuildCellHashKernel(ParticleSoAView state, int numParticles,
                                          TreePmGridParams grid, IndexHandle cellHash,
                                          IndexHandle particleIndex)
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
    cellHash[i] = treePmGridIndex(x, y, z, grid);
    particleIndex[i] = i;
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
    const unsigned int* cellMask, float cutoffSquared, int cellRadius, int maxLocalNeighbors)
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
                    if (abs(dx) > shell ||
                        max(max(abs(dx), abs(dy)), abs(dz)) != shell) {
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
                        const Vector3 sourcePos = octreeLoadParticlePosition(state, otherIndex);
                        const Vector3 diff(selfPos.x - sourcePos.x, selfPos.y - sourcePos.y,
                                           selfPos.z - sourcePos.z);
                        if (softenedDistanceSquared(diff, forceLaw) > cutoffSquared) {
                            continue;
                        }
                        force += blitzarAccelerationFromSource(
                            selfPos, sourcePos, octreeLoadParticleMass(state, otherIndex), forceLaw);
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
    float cutoffSquared, int cellRadius, int maxLocalNeighbors)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }

    const Vector3 selfPos = octreeLoadParticlePosition(lastState, i);
    const Vector3 force = treePmComputeLocalGridAcceleration(
        lastState, i, grid, sortedIndex, cellStart, cellEnd, forceLaw, maxAcceleration, accelX,
        accelY, accelZ, cellMask, cutoffSquared, cellRadius, maxLocalNeighbors);
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
    const Vector3 treeAcceleration = computeOctreeAccelerationStacklessCompact(
        state, selfIndex, nodeHot, nodeNav, nodeFirstChild, leafStarts, leafCounts, rootIndex,
        leafIndices, forceLaw, maxAcceleration, openingCriterion, cutoffSquared);
    return clampAcceleration(pmAcceleration + treeAcceleration, maxAcceleration);
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
    const Vector3 force = treePmComputeAcceleration(lastState, i, nodeHot, nodeNav, nodeFirstChild,
                                                    leafStarts, leafCounts, rootIndex, leafIndices,
                                                    forceLaw, maxAcceleration, openingCriterion,
                                                    cutoffSquared, grid, accelX, accelY, accelZ);
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

} // namespace

bool ParticleSystem::ensureTreePmScratchCapacity(int gridCells)
{
    if (!_device._cudaRuntimeAvailable || gridCells <= 0) {
        return false;
    }
    const std::size_t cells = static_cast<std::size_t>(gridCells);
    const std::size_t maskWords = (cells + 31u) / 32u;
    if (_device.d_treePmCapacity >= cells &&
        _device.d_treePmMaskWordCapacity >= maskWords &&
        _device.d_treePmCellMask != nullptr) {
        return true;
    }

    bltzr_x::MemoryPool::deallocate(_device.d_treePmDensity);
    bltzr_x::MemoryPool::deallocate(_device.d_treePmPotentialA);
    bltzr_x::MemoryPool::deallocate(_device.d_treePmPotentialB);
    bltzr_x::MemoryPool::deallocate(_device.d_treePmAccelX);
    bltzr_x::MemoryPool::deallocate(_device.d_treePmAccelY);
    bltzr_x::MemoryPool::deallocate(_device.d_treePmAccelZ);
    bltzr_x::MemoryPool::deallocate(_device.d_treePmCellMask);
    _device.d_treePmDensity = nullptr;
    _device.d_treePmPotentialA = nullptr;
    _device.d_treePmPotentialB = nullptr;
    _device.d_treePmAccelX = nullptr;
    _device.d_treePmAccelY = nullptr;
    _device.d_treePmAccelZ = nullptr;
    _device.d_treePmCellMask = nullptr;
    _device.d_treePmCapacity = 0u;
    _device.d_treePmMaskWordCapacity = 0u;

    _device.d_treePmDensity = static_cast<float*>(bltzr_x::MemoryPool::allocate(cells * sizeof(float)));
    _device.d_treePmPotentialA = static_cast<float*>(bltzr_x::MemoryPool::allocate(cells * sizeof(float)));
    _device.d_treePmPotentialB = static_cast<float*>(bltzr_x::MemoryPool::allocate(cells * sizeof(float)));
    _device.d_treePmAccelX = static_cast<float*>(bltzr_x::MemoryPool::allocate(cells * sizeof(float)));
    _device.d_treePmAccelY = static_cast<float*>(bltzr_x::MemoryPool::allocate(cells * sizeof(float)));
    _device.d_treePmAccelZ = static_cast<float*>(bltzr_x::MemoryPool::allocate(cells * sizeof(float)));
    _device.d_treePmCellMask = static_cast<unsigned int*>(
        bltzr_x::MemoryPool::allocate(maskWords * sizeof(unsigned int)));
    if (!_device.d_treePmDensity || !_device.d_treePmPotentialA || !_device.d_treePmPotentialB ||
        !_device.d_treePmAccelX || !_device.d_treePmAccelY || !_device.d_treePmAccelZ ||
        !_device.d_treePmCellMask) {
        bltzr_x::MemoryPool::deallocate(_device.d_treePmDensity);
        bltzr_x::MemoryPool::deallocate(_device.d_treePmPotentialA);
        bltzr_x::MemoryPool::deallocate(_device.d_treePmPotentialB);
        bltzr_x::MemoryPool::deallocate(_device.d_treePmAccelX);
        bltzr_x::MemoryPool::deallocate(_device.d_treePmAccelY);
        bltzr_x::MemoryPool::deallocate(_device.d_treePmAccelZ);
        bltzr_x::MemoryPool::deallocate(_device.d_treePmCellMask);
        _device.d_treePmDensity = nullptr;
        _device.d_treePmPotentialA = nullptr;
        _device.d_treePmPotentialB = nullptr;
        _device.d_treePmAccelX = nullptr;
        _device.d_treePmAccelY = nullptr;
        _device.d_treePmAccelZ = nullptr;
        _device.d_treePmCellMask = nullptr;
        return false;
    }

    _device.d_treePmCapacity = cells;
    _device.d_treePmMaskWordCapacity = maskWords;
    return true;
}

bool ParticleSystem::ensureTreePmNeighborGridCapacity(int numParticles, int totalCells)
{
    if (!_device._cudaRuntimeAvailable || numParticles <= 0 || totalCells <= 0) {
        return false;
    }
    const std::size_t particleCapacity = static_cast<std::size_t>(numParticles);
    if (_device.d_treePmNeighborParticleCapacity < particleCapacity || !_device.d_sphCellHash ||
        !_device.d_sphSortedIndex) {
        if (_device.d_sphCellHash) {
            bltzr_x::MemoryPool::deallocate(_device.d_sphCellHash);
            _device.d_sphCellHash = nullptr;
        }
        if (_device.d_sphSortedIndex) {
            bltzr_x::MemoryPool::deallocate(_device.d_sphSortedIndex);
            _device.d_sphSortedIndex = nullptr;
        }
        const std::size_t bytes = particleCapacity * sizeof(int);
        _device.d_sphCellHash = static_cast<int*>(bltzr_x::MemoryPool::allocate(bytes));
        _device.d_sphSortedIndex = static_cast<int*>(bltzr_x::MemoryPool::allocate(bytes));
        if (!_device.d_sphCellHash || !_device.d_sphSortedIndex) {
            releaseSphGridBuffers();
            return false;
        }
        _device.d_treePmNeighborParticleCapacity = particleCapacity;
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
    if (!ensureTreePmNeighborGridCapacity(numParticles, grid.totalCells)) {
        return false;
    }

    const int numBlocks = (numParticles + Particle::kDefaultCudaBlockSize - 1) /
                          Particle::kDefaultCudaBlockSize;
    treePmBuildCellHashKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
        currentView, numParticles, grid, _device.d_sphCellHash, _device.d_sphSortedIndex);
    if (!checkCudaStatus(cudaGetLastError(), "treePmBuildCellHashKernel launch")) {
        return false;
    }

    ThrustPoolAllocator thrustAllocator;
    auto exec = thrust::cuda::par(thrustAllocator);
    thrust::device_ptr<int> hashes(_device.d_sphCellHash);
    thrust::device_ptr<int> indices(_device.d_sphSortedIndex);
    thrust::sort_by_key(exec, hashes, hashes + numParticles, indices);
    if (!checkCudaStatus(cudaGetLastError(), "treepm neighbor sort_by_key")) {
        return false;
    }

    const int cellBlocks = (grid.totalCells + Particle::kDefaultCudaBlockSize - 1) /
                           Particle::kDefaultCudaBlockSize;
    treePmResetCellBoundsKernel<<<cellBlocks, Particle::kDefaultCudaBlockSize>>>(
        _device.d_sphCellStart, _device.d_sphCellEnd, grid.totalCells);
    if (!checkCudaStatus(cudaGetLastError(), "treePmResetCellBoundsKernel launch")) {
        return false;
    }
    treePmFindCellBoundsKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
        _device.d_sphCellHash, _device.d_sphCellStart, _device.d_sphCellEnd, numParticles);
    if (!checkCudaStatus(cudaGetLastError(), "treePmFindCellBoundsKernel launch")) {
        return false;
    }
    return checkCudaStatus(cudaDeviceSynchronize(), "treepm neighbor grid sync");
}

bool ParticleSystem::buildTreePmGrid(ParticleSoAView currentView, int numParticles,
                                    TreePmGridParams* outGrid, float* outCutoffSquared)
{
    if (!_device._cudaRuntimeAvailable || !outGrid || !outCutoffSquared || numParticles <= 0 ||
        !_device.d_soaPosX || !_device.d_soaPosY || !_device.d_soaPosZ) {
        return false;
    }

    float minX = _particles[0].getPosition().x;
    float minY = _particles[0].getPosition().y;
    float minZ = _particles[0].getPosition().z;
    float maxX = minX;
    float maxY = minY;
    float maxZ = minZ;
    for (int i = 1; i < numParticles; ++i) {
        const Vector3 p = _particles[static_cast<std::size_t>(i)].getPosition();
        if (p.x < minX) {
            minX = p.x;
        }
        if (p.y < minY) {
            minY = p.y;
        }
        if (p.z < minZ) {
            minZ = p.z;
        }
        if (p.x > maxX) {
            maxX = p.x;
        }
        if (p.y > maxY) {
            maxY = p.y;
        }
        if (p.z > maxZ) {
            maxZ = p.z;
        }
    }

    const int requestedGridSize = std::clamp(
        static_cast<int>(parseFloatEnv("BLITZAR_TREEPM_GRID_SIZE", 64.0f)), 32, 128);
    const float extent = std::max({maxX - minX, maxY - minY, maxZ - minZ, _octreeSoftening});
    const float cellSize = std::max(0.25f, extent / static_cast<float>(requestedGridSize - 2));
    const int gridSize = std::clamp(requestedGridSize, 32, 128);
    const int totalCells = gridSize * gridSize * gridSize;
    const float invCellSize = 1.0f / cellSize;

    if (!ensureTreePmScratchCapacity(totalCells)) {
        return false;
    }

    if (!checkCudaStatus(cudaMemset(_device.d_treePmDensity, 0, static_cast<std::size_t>(totalCells) * sizeof(float)),
                         "cudaMemset(treepm density)")) {
        return false;
    }
    if (!checkCudaStatus(cudaMemset(_device.d_treePmPotentialA, 0, static_cast<std::size_t>(totalCells) * sizeof(float)),
                         "cudaMemset(treepm potential A)")) {
        return false;
    }
    if (!checkCudaStatus(cudaMemset(_device.d_treePmPotentialB, 0, static_cast<std::size_t>(totalCells) * sizeof(float)),
                         "cudaMemset(treepm potential B)")) {
        return false;
    }
    if (!checkCudaStatus(cudaMemset(_device.d_treePmAccelX, 0, static_cast<std::size_t>(totalCells) * sizeof(float)),
                         "cudaMemset(treepm accel X)")) {
        return false;
    }
    if (!checkCudaStatus(cudaMemset(_device.d_treePmAccelY, 0, static_cast<std::size_t>(totalCells) * sizeof(float)),
                         "cudaMemset(treepm accel Y)")) {
        return false;
    }
    if (!checkCudaStatus(cudaMemset(_device.d_treePmAccelZ, 0, static_cast<std::size_t>(totalCells) * sizeof(float)),
                         "cudaMemset(treepm accel Z)")) {
        return false;
    }
    const std::size_t maskWords = (static_cast<std::size_t>(totalCells) + 31u) / 32u;
    if (!checkCudaStatus(cudaMemset(_device.d_treePmCellMask, 0,
                                    maskWords * sizeof(unsigned int)),
                         "cudaMemset(treepm cell mask)")) {
        return false;
    }

    TreePmGridParams grid;
    grid.gridSize = gridSize;
    grid.totalCells = totalCells;
    grid.cellSize = cellSize;
    grid.invCellSize = invCellSize;
    grid.originX = minX - cellSize;
    grid.originY = minY - cellSize;
    grid.originZ = minZ - cellSize;

    const int numBlocks = (numParticles + Particle::kDefaultCudaBlockSize - 1) /
                          Particle::kDefaultCudaBlockSize;
    treePmDepositMassKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
        currentView, numParticles, grid, _device.d_treePmDensity, _device.d_treePmCellMask);
    if (!checkCudaStatus(cudaGetLastError(), "treePmDepositMassKernel launch")) {
        return false;
    }

    const int gridBlocks = (totalCells + Particle::kDefaultCudaBlockSize - 1) /
                           Particle::kDefaultCudaBlockSize;
    const int iterationCount = std::clamp(
        static_cast<int>(parseFloatEnv("BLITZAR_TREEPM_JACOBI_ITERS", 12.0f)), 4, 64);
    float* currentPotential = _device.d_treePmPotentialA;
    float* nextPotential = _device.d_treePmPotentialB;
    for (int iteration = 0; iteration < iterationCount; ++iteration) {
        treePmJacobiStepKernel<<<gridBlocks, Particle::kDefaultCudaBlockSize>>>(
            _device.d_treePmDensity, currentPotential, nextPotential, grid);
        if (!checkCudaStatus(cudaGetLastError(), "treePmJacobiStepKernel launch")) {
            return false;
        }
        std::swap(currentPotential, nextPotential);
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
    const float cutoffFactor = std::clamp(parseFloatEnv("BLITZAR_TREEPM_CUTOFF_FACTOR", 1.0f),
                                          1.0f, 2.0f);
    *outCutoffSquared = cutoffFactor * cutoffFactor * cellSize * cellSize;
    return true;
}
