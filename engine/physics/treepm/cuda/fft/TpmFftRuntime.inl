/*
 * @file engine/physics/treepm/cuda/fft/TpmFftRuntime.inl
 * @project BLITZAR
 * @brief TreePM FFT plan and field execution.
 */

bool ParticleSystem::buildTreePmFftField(const TreePmGridParams& grid)
{
    _device->_treePmFftActive = false;
    const int gridSize = grid.gridSize;
    if (_device->_treePmFftPlan.get() == 0 || _device->_treePmFftPlanGridSize != gridSize) {
        _device->_treePmFftPlan.reset();
        _device->_treePmFftInversePlan.reset();
        _device->_treePmFftPlanGridSize = 0;
        cufftHandle plan = 0;
        if (!treepm::checkTreePmFftStatus(cufftPlan3d(&plan, gridSize, gridSize, gridSize, CUFFT_R2C),
                                  "cufftPlan3d")) {
            return false;
        }
        _device->_treePmFftPlan = static_cast<int>(plan);
        cufftHandle inversePlan = 0;
        if (!treepm::checkTreePmFftStatus(
                cufftPlan3d(&inversePlan, gridSize, gridSize, gridSize, CUFFT_C2R),
                "cufftPlan3d inverse")) {
            cufftDestroy(plan);
            _device->_treePmFftPlan.reset();
            return false;
        }
        _device->_treePmFftInversePlan = static_cast<int>(inversePlan);
        _device->_treePmFftPlanGridSize = gridSize;
    }

    auto* density = reinterpret_cast<cufftReal*>(_device->d_treePmDensity.get());
    auto* densitySpectrum = reinterpret_cast<cufftComplex*>(_device->d_treePmSpectrum.get());
    if (!treepm::checkTreePmFftStatus(
            cufftExecR2C(static_cast<cufftHandle>(_device->_treePmFftPlan.get()), density,
                         densitySpectrum),
            "cufftExecR2C")) {
        return false;
    }

    const int spectrumCells = gridSize * gridSize * (gridSize / 2 + 1);
    const int spectrumBlocks =
        (spectrumCells + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    treepm::treePmApplyFftKernel<<<spectrumBlocks, Particle::kDefaultCudaBlockSize>>>(
        densitySpectrum, reinterpret_cast<cufftComplex*>(_device->d_treePmSpectrumX.get()), grid,
        std::max(_octreeSoftening, _physicsMinSoftening));
    if (!checkCudaStatus(cudaGetLastError(), "treePmApplyFftKernel launch")) {
        return false;
    }

    const cufftHandle inversePlan = static_cast<cufftHandle>(_device->_treePmFftInversePlan.get());
    if (!treepm::checkTreePmFftStatus(
            cufftExecC2R(inversePlan,
                         reinterpret_cast<cufftComplex*>(_device->d_treePmSpectrumX.get()),
                         reinterpret_cast<cufftReal*>(_device->d_treePmPotentialA.get())),
            "cufftExecC2R potential")) {
        return false;
    }

    const int fieldBlocks =
        (grid.totalCells + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    treepm::treePmNormalizeFftFieldKernel<<<fieldBlocks, Particle::kDefaultCudaBlockSize>>>(
        _device->d_treePmPotentialA, grid.totalCells);
    treepm::treePmPotentialGradientKernel<<<fieldBlocks, Particle::kDefaultCudaBlockSize>>>(
        _device->d_treePmPotentialA, _device->d_treePmAccelX, _device->d_treePmAccelY,
        _device->d_treePmAccelZ, grid);
    if (!checkCudaStatus(cudaGetLastError(), "treepm potential gradient launch")) {
        return false;
    }
    if (!checkCudaStatus(cudaDeviceSynchronize(), "treepm FFT field sync")) {
        return false;
    }
    _device->_treePmFftActive = true;
    return true;
}
