/*
 * @file engine/physics/treepm/cuda/tree_force/TpmTreeForce.inl
 * @project BLITZAR
 * @brief TreePM octree and hybrid force evaluation.
 */

namespace blitzar_cuda_tree_pm_gpu {

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
