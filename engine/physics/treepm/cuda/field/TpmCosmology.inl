/*
 * @file engine/physics/treepm/cuda/field/TpmCosmology.inl
 * @project BLITZAR
 * @brief TreePM cosmological periodic-grid construction.
 */

bool ParticleSystem::buildTreePmGrid(ParticleSoAView currentView, int numParticles,
                                     TreePmGridParams* outGrid, float* outCutoffSquared)
{
    if (!_device->_cudaRuntimeAvailable || !outGrid || !outCutoffSquared || numParticles <= 0 ||
        !_device->d_soaPosX || !_device->d_soaPosY || !_device->d_soaPosZ) {
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
        if (!checkCudaStatus(cudaMemset(_device->d_treePmDensity, 0,
                                        static_cast<std::size_t>(totalCells) * sizeof(float)),
                             "cudaMemset(cosmology density)") ||
            !checkCudaStatus(cudaMemset(_device->d_treePmPotentialA, 0,
                                        static_cast<std::size_t>(totalCells) * sizeof(float)),
                             "cudaMemset(cosmology potential)")) {
            return false;
        }
        const std::size_t maskWords = (static_cast<std::size_t>(totalCells) + 31u) / 32u;
        if (!checkCudaStatus(cudaMemset(_device->d_treePmCellMask, 0,
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
        treepm::treePmDepositMassKernel<<<blocks, Particle::kDefaultCudaBlockSize>>>(
            currentView, numParticles, numParticles, grid, _device->d_treePmDensity,
            _device->d_treePmCellMask);
    treepm::treePmBuildDensityContrastKernel<<<(totalCells + Particle::kDefaultCudaBlockSize - 1) /
                                              Particle::kDefaultCudaBlockSize,
                                          Particle::kDefaultCudaBlockSize>>>(
            _device->d_treePmDensity, totalCells);
        if (!checkCudaStatus(cudaGetLastError(), "cosmology TSC deposit launch") ||
            !buildTreePmFftField(grid)) {
            return false;
        }
        _device->_treePmGridSize = gridSize;
        _device->_treePmTotalCells = totalCells;
        *outGrid = grid;
        *outCutoffSquared = 0.0f;
        return true;
    }
