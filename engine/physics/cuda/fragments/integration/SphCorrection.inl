/*
 * @file engine/physics/cuda/fragments/integration/SphCorrection.inl
 * @project BLITZAR
 * @brief CUDA SPH correction stage for one particle-system update.
 */

#include <algorithm>

/*
 * @brief Applies the grid-based SPH density, pressure, and integration stage.
 * @param deltaTime Integration interval.
 * @param uploadHostState Whether the current host state must be uploaded first.
 * @return True when the correction completed successfully.
 */
bool ParticleSystem::applySphCorrection(float deltaTime, bool uploadHostState)
{
    if (!_sphEnabled || !_device->_cudaRuntimeAvailable) {
        return true;
    }
    if (!_device->d_soaPosX || !_device->d_sphDensity || !_device->d_sphPressure) {
        return false;
    }

    const int numParticles = static_cast<int>(_particles.size());
    if (numParticles < 2) {
        return true;
    }
    const int numBlocks =
        (numParticles + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;

    if (uploadHostState) {
        syncDeviceState();
    }
    if (!buildSphGrid(numParticles)) {
        return false;
    }

    SphGridParams grid;
    grid.gridSize = _device->_sphGridSize;
    grid.totalCells = _device->_sphGridTotalCells;
    grid.cellSize = std::max(0.01f, _sphSmoothingLength);

    ParticleSoAView currentView = getSoAView(false);
    ParticleSoAView nextView = getSoAView(true);
    computeSphDensityPressureGridKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
        currentView, _device->d_sphDensity, _device->d_sphPressure, numParticles,
        _sphSmoothingLength, _sphRestDensity, _sphGasConstant, _device->d_sphCellHash,
        _device->d_sphSortedIndex, _device->d_sphCellStart, _device->d_sphCellEnd, grid);
    if (!checkCudaStatus(cudaGetLastError(), "computeSphDensityPressureGrid kernel launch")) {
        return false;
    }

    constexpr float kSphCorrectionScale = 0.22f;
    integrateSphGridKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
        currentView, nextView, _device->d_sphDensity, _device->d_sphPressure, numParticles,
        _sphSmoothingLength, _sphViscosity, deltaTime, kSphCorrectionScale,
        _device->d_sphCellHash, _device->d_sphSortedIndex, _device->d_sphCellStart,
        _device->d_sphCellEnd, grid, _sphMaxAcceleration, _sphMaxSpeed);
    if (!checkCudaStatus(cudaGetLastError(), "integrateSphGrid kernel launch")) {
        return false;
    }
    if (!checkCudaStatus(cudaDeviceSynchronize(), "sph grid kernels sync")) {
        return false;
    }

    std::swap(_device->d_soaPosX, _device->d_soaNextPosX);
    std::swap(_device->d_soaPosY, _device->d_soaNextPosY);
    std::swap(_device->d_soaPosZ, _device->d_soaNextPosZ);
    std::swap(_device->d_soaVelX, _device->d_soaNextVelX);
    std::swap(_device->d_soaVelY, _device->d_soaNextVelY);
    std::swap(_device->d_soaVelZ, _device->d_soaNextVelZ);
    _device->_hostStateDirty = true;
    return true;
}
