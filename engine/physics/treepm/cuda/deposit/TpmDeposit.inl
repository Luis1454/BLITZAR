/*
 * @file engine/physics/treepm/cuda/deposit/TpmDeposit.inl
 * @project BLITZAR
 * @brief TreePM mass deposition kernels.
 */

namespace blitzar_cuda_tree_pm_gpu {

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

} // namespace blitzar_cuda_tree_pm_gpu
