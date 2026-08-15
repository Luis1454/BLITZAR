/*
 * @file engine/src/cuda/fragments/treepm/GridBuild.inl
 * @project BLITZAR
 * @brief TreePM isolated-grid construction and fallback solve.
 */

if (!ensureTreePmBoundsCapacity(numParticles)) {
        return false;
    }
    const int boundsBlocks =
        (numParticles + treepm::kTreePmBoundsBlockSize - 1) / treepm::kTreePmBoundsBlockSize;
    treepm::treePmReduceBoundsKernel<<<boundsBlocks, treepm::kTreePmBoundsBlockSize>>>(
        currentView, numParticles, _device.d_treePmBoundsPartial);
    if (!checkCudaStatus(cudaGetLastError(), "treePmReduceBoundsKernel launch")) {
        return false;
    }
    treepm::treePmFinalizeBoundsKernel<<<1, treepm::kTreePmBoundsBlockSize>>>(
        _device.d_treePmBoundsPartial, boundsBlocks, _device.d_treePmBounds);
    if (!checkCudaStatus(cudaGetLastError(), "treePmFinalizeBoundsKernel launch")) {
        return false;
    }
    float bounds[treepm::kTreePmBoundsFieldCount]{};
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

    if (treePmLayoutMode() == treepm::kTreePmLayoutAuto && !_device._treePmAutoLayoutResolved) {
        bool autoLayoutReady = false;
        if (ensureTreePmConcentrationCapacity()) {
            if (!checkCudaStatus(
                    cudaMemset(_device.d_treePmRadialMassHistogram, 0,
                               treepm::kTreePmConcentrationBinCount * sizeof(float)),
                    "cudaMemset(treepm concentration histogram)")) {
                return false;
            }
            const int histogramBlocks =
                (numParticles + Particle::kDefaultCudaBlockSize - 1) /
                Particle::kDefaultCudaBlockSize;
            treepm::treePmRadialMassHistogramKernel<<<histogramBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, numParticles, _device.d_treePmBounds,
                _device.d_treePmRadialMassHistogram);
            if (!checkCudaStatus(cudaGetLastError(),
                                 "treePmRadialMassHistogramKernel launch") ||
                !checkCudaStatus(cudaDeviceSynchronize(),
                                 "treePmRadialMassHistogramKernel sync")) {
                return false;
            }
            float histogram[treepm::kTreePmConcentrationBinCount]{};
            if (!checkCudaStatus(
                    cudaMemcpy(histogram, _device.d_treePmRadialMassHistogram,
                               sizeof(histogram), cudaMemcpyDeviceToHost),
                    "cudaMemcpy(treepm concentration histogram)")) {
                return false;
            }

            const float targetMass = std::max(0.0f, 0.8f * totalMass);
            float cumulativeMass = 0.0f;
            int r80Bin = treepm::kTreePmConcentrationBinCount - 1;
            for (int bin = 0; bin < treepm::kTreePmConcentrationBinCount; ++bin) {
                cumulativeMass += histogram[bin];
                if (cumulativeMass >= targetMass) {
                    r80Bin = bin;
                    break;
                }
            }
            const float r80Ratio = static_cast<float>(r80Bin + 1) /
                                   static_cast<float>(treepm::kTreePmConcentrationBinCount);
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
    treepm::treePmDepositMassKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
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
            treepm::treePmRedBlackStepKernel<<<gridBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentPotential, _device.d_treePmDensity, grid, parity);
            if (!checkCudaStatus(cudaGetLastError(), "treePmRedBlackStepKernel launch")) {
                return false;
            }
        }
    }

    treepm::treePmPotentialGradientKernel<<<gridBlocks, Particle::kDefaultCudaBlockSize>>>(
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
