/*
 * @file engine/physics/treepm/cuda/fft/TpmFft.inl
 * @project BLITZAR
 * @brief TreePM FFT operators and finite-difference fallback kernels.
 */

namespace blitzar_cuda_tree_pm_gpu {

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

} // namespace blitzar_cuda_tree_pm_gpu
