/*
 * @file engine/physics/treepm/cuda/neighbor/TpmNeighbor.inl
 * @project BLITZAR
 * @brief TreePM spatial hash and cell-boundary kernels.
 */

namespace blitzar_cuda_tree_pm_gpu {

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

} // namespace blitzar_cuda_tree_pm_gpu
