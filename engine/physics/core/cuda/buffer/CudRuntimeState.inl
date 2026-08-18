/*
 * @file engine/physics/core/cuda/buffer/CudRuntimeState.inl
 * @project BLITZAR
 * @brief Particle-system buffer lifecycle implementation fragment.
 */

void ParticleSystem::initializeRuntimeState(std::size_t particleCapacity, bool enableCudaRuntime)
{
    _device = std::make_unique<ParticleSystemDeviceState>();
    _device->_cudaRuntimeAvailable = enableCudaRuntime && cudaRuntimeAvailable();
    if (_device->_cudaRuntimeAvailable) {
        const cudaError_t mapHostStatus = cudaSetDeviceFlags(cudaDeviceMapHost);
        if (mapHostStatus != cudaSuccess && mapHostStatus != cudaErrorSetOnActiveProcess) {
            checkCudaStatus(mapHostStatus, "cudaSetDeviceFlags(cudaDeviceMapHost)");
            _device->_cudaRuntimeAvailable = false;
        }
    }
    if (_device->_cudaRuntimeAvailable) {
        bltzr_x::MemoryPool::initialize();
        _device->_cudaJit = std::make_unique<CudaJitRuntime>();
        if (!_device->_cudaJit->available()) {
            _device->_cudaJit.reset();
        }
    }
    _solverMode = solverModeFromEnv();
    _integratorMode = integratorModeFromEnv();
    _octreeTheta = parseFloatEnv("BLITZAR_OCTREE_THETA", 1.2f);
    _octreeOpeningCriterion = OctreeOpeningCriterion::CenterOfMass;
    _octreeSoftening = parseFloatEnv("BLITZAR_OCTREE_SOFTENING", 2.5f);
    _sphEnabled = parseBoolEnv("BLITZAR_SPH_ENABLED", false);
    _sphSmoothingLength = parseFloatEnv("BLITZAR_SPH_H", 1.25f);
    _sphRestDensity = parseFloatEnv("BLITZAR_SPH_REST_DENSITY", 1.0f);
    _sphGasConstant = parseFloatEnv("BLITZAR_SPH_GAS_CONSTANT", 4.0f);
    _sphViscosity = parseFloatEnv("BLITZAR_SPH_VISCOSITY", 0.08f);
    _thermalAmbientTemperature = parseFloatEnv("BLITZAR_THERMAL_AMBIENT", 0.0f);
    _thermalSpecificHeat = parseFloatEnv("BLITZAR_THERMAL_SPECIFIC_HEAT", 1.0f);
    _thermalHeatingCoeff = parseFloatEnv("BLITZAR_THERMAL_HEATING", 0.0002f);
    _thermalRadiationCoeff = parseFloatEnv("BLITZAR_THERMAL_RADIATION", 0.00000001f);
    _physicsMaxAcceleration = kPhysicsMaxAccelerationDefault;
    _physicsMinSoftening = kPhysicsMinSofteningDefault;
    _physicsMinDistance2 = kPhysicsMinDistance2Default;
    _physicsMinTheta = kPhysicsMinTheta;
    _sphMaxAcceleration = kSphMaxAccelerationDefault;
    _sphMaxSpeed = kSphMaxSpeedDefault;
    _cumulativeRadiatedEnergy = 0.0f;
    _device->_sphGridSize = 0;
    _device->_sphGridTotalCells = 0;
    _device->_treePmMarkerPrinted = false;
    _device->_treePmGraphCaptured[0] = false;
    _device->_treePmGraphCaptured[1] = false;
    _device->_treePmGraphMarkerPrinted = false;
    _device->_treePmGraphSlot = 0;
    _device->_treePmGraphExec[0] = nullptr;
    _device->_treePmGraphExec[1] = nullptr;
    _device->_treePmGraphStream = nullptr;
    _device->_cudaJitMarkerPrinted = false;
    _device->_cudaJitForceMarkerPrinted = false;
    _device->_cudaE2eTotalMs = 0.0;
    _device->_cudaE2eSamples = 0u;
    _device->_treePmGridSize = 0;
    _device->_treePmTotalCells = 0;
    _device->_deviceParticleCapacity = particleCapacity;
    _device->_hostStateDirty = false;
    _device->d_soaPosX = nullptr;
    _device->d_soaPosY = nullptr;
    _device->d_soaPosZ = nullptr;
    _device->d_soaVelX = nullptr;
    _device->d_soaVelY = nullptr;
    _device->d_soaVelZ = nullptr;
    _device->d_soaPressX = nullptr;
    _device->d_soaPressY = nullptr;
    _device->d_soaPressZ = nullptr;
    _device->d_soaMass = nullptr;
    _device->d_soaTemp = nullptr;
    _device->d_soaDens = nullptr;
    _device->d_soaNextPosX = nullptr;
    _device->d_soaNextPosY = nullptr;
    _device->d_soaNextPosZ = nullptr;
    _device->d_soaNextVelX = nullptr;
    _device->d_soaNextVelY = nullptr;
    _device->d_soaNextVelZ = nullptr;
    _device->d_stage = nullptr;
    _device->d_k1x = nullptr;
    _device->d_k2x = nullptr;
    _device->d_k3x = nullptr;
    _device->d_k4x = nullptr;
    _device->d_k1v = nullptr;
    _device->d_k2v = nullptr;
    _device->d_k3v = nullptr;
    _device->d_k4v = nullptr;
    _device->d_vHalf = nullptr;
    _device->_leapfrogPrimed = false;
    _device->d_sphDensity = nullptr;
    _device->d_sphPressure = nullptr;
    _device->d_sphCellHash = nullptr;
    _device->d_sphSortedIndex = nullptr;
    _device->d_sphCellStart = nullptr;
    _device->d_sphCellEnd = nullptr;
    _device->d_treePmSortKeys = nullptr;
    _device->d_treePmSortIndices = nullptr;
    _device->d_treePmSortedCellHash = nullptr;
    _device->d_treePmSortTempStorage = nullptr;
    _device->d_treePmSortedPosX = nullptr;
    _device->d_treePmSortedPosY = nullptr;
    _device->d_treePmSortedPosZ = nullptr;
    _device->d_treePmSortedMass = nullptr;
    _device->d_treePmDensity = nullptr;
    _device->d_treePmPotentialA = nullptr;
    _device->d_treePmPotentialB = nullptr;
    _device->d_treePmAccelX = nullptr;
    _device->d_treePmAccelY = nullptr;
    _device->d_treePmAccelZ = nullptr;
    _device->d_treePmBoundsPartial = nullptr;
    _device->d_treePmBounds = nullptr;
    _device->d_adaptiveAcceleration = nullptr;
    _device->d_adaptiveLevels = nullptr;
    _device->d_adaptiveLastForceTicks = nullptr;
    _device->d_treePmCellMask = nullptr;
    _device->g_dOctreeNodes = nullptr;
    _device->g_dOctreeLeafIndices = nullptr;
    _device->d_octreeMortonKeys = nullptr;
    _device->d_octreePrefixesA = nullptr;
    _device->d_octreePrefixesB = nullptr;
    _device->d_octreeLevelIndicesA = nullptr;
    _device->d_octreeLevelIndicesB = nullptr;
    _device->d_octreeParentCounts = nullptr;
    _device->d_octreeParentOffsets = nullptr;
    _device->d_octreeNodeHot = nullptr;
    _device->d_octreeNodeNav = nullptr;
    _device->d_octreeFirstChild = nullptr;
    _device->d_octreeLeafStarts = nullptr;
    _device->d_octreeLeafCounts = nullptr;
    _device->d_energyKineticBlocks = nullptr;
    _device->d_energyThermalBlocks = nullptr;
    _device->d_energyPotentialPartials = nullptr;
    _device->g_dOctreeNodeCapacity = 0;
    _device->g_dOctreeLeafCapacity = 0;
    _device->d_octreeMortonCapacity = 0;
    _device->d_octreePrefixCapacity = 0;
    _device->d_octreeLevelCapacity = 0;
    _device->d_treePmCapacity = 0;
    _device->d_treePmMaskWordCapacity = 0;
    _device->d_treePmNeighborParticleCapacity = 0;
    _device->d_treePmNeighborCellCapacity = 0;
    _device->d_treePmSortTempCapacity = 0;
    _device->d_treePmSortedParticleCapacity = 0;
    _device->d_adaptiveCapacity = 0;
    _device->d_energyBlockCapacity = 0;
    _device->d_energySampleCapacity = 0;
    _device->_gpuOctreeRootIndex = -1;
    _device->_gpuOctreeNodeCount = 0;
    _device->_gpuOctreeLeafCount = 0;
    _device->_mappedMetricsHost = nullptr;
    _device->_mappedMetricsDevice = 0u;
    _device->_metricsStepId = 0u;
    _device->_metricsSimTime = 0.0f;
    _device->_metricsPublishCounter = 0u;
    _device->_linearOctreeLeafCapacity = kDefaultOctreeLeafCapacity;
    if (!_device->_cudaRuntimeAvailable) {
        return;
    }

    const bool strictMemoryMode = parseBoolEnv("BLITZAR_STRICT_MEMORY_MODE", false);
    std::size_t selectedEnergySampleLimit = kDefaultEnergySampleLimit;
    int selectedOctreeLeafCapacity = _device->_linearOctreeLeafCapacity;

    std::size_t baseAndIntegratorBytes = 0u;
    std::size_t sphBytes = 0u;
    std::size_t octreeBytes = 0u;
    std::size_t totalBytes = estimateMemoryUsage(
        particleCapacity, _sphEnabled, _solverMode, _integratorMode, selectedEnergySampleLimit,
        selectedOctreeLeafCapacity, &baseAndIntegratorBytes, &sphBytes, &octreeBytes);

    std::cout << formatMemoryBreakdown(baseAndIntegratorBytes, sphBytes, octreeBytes, totalBytes,
                                       kVramBudgetBytes)
              << "\n";

    const bool treePmFastPath =
        treePmFastPathBypassesOctreeScratch(_integratorMode == IntegratorMode::Euler);
    if (_solverMode == SolverMode::OctreeGpu && !treePmFastPath && totalBytes > kVramBudgetBytes &&
        selectedOctreeLeafCapacity < kPlanBOctreeLeafCapacity) {
        selectedOctreeLeafCapacity = kPlanBOctreeLeafCapacity;
        _device->_linearOctreeLeafCapacity = selectedOctreeLeafCapacity;
        totalBytes = estimateMemoryUsage(
            particleCapacity, _sphEnabled, _solverMode, _integratorMode, selectedEnergySampleLimit,
            selectedOctreeLeafCapacity, &baseAndIntegratorBytes, &sphBytes, &octreeBytes);
        std::cout << "[info] dynamic octree leaf capacity bump to " << selectedOctreeLeafCapacity
                  << " after VRAM estimate exceeded budget\n";
        std::cout << formatMemoryBreakdown(baseAndIntegratorBytes, sphBytes, octreeBytes,
                                           totalBytes, kVramBudgetBytes)
                  << "\n";
    }

    if (strictMemoryMode && totalBytes > kVramBudgetBytes) {
        selectedEnergySampleLimit = kPlanAEnergySampleLimit;
        totalBytes = estimateMemoryUsage(
            particleCapacity, _sphEnabled, _solverMode, _integratorMode, selectedEnergySampleLimit,
            selectedOctreeLeafCapacity, &baseAndIntegratorBytes, &sphBytes, &octreeBytes);
        std::cout << "[info] strict memory plan A (energy sample limit="
                  << selectedEnergySampleLimit << ")\n";
        std::cout << formatMemoryBreakdown(baseAndIntegratorBytes, sphBytes, octreeBytes,
                                           totalBytes, kVramBudgetBytes)
                  << "\n";
    }

    if (strictMemoryMode && totalBytes > kVramBudgetBytes && _solverMode == SolverMode::OctreeGpu &&
        !treePmFastPath) {
        selectedOctreeLeafCapacity = kPlanBOctreeLeafCapacity;
        totalBytes = estimateMemoryUsage(
            particleCapacity, _sphEnabled, _solverMode, _integratorMode, selectedEnergySampleLimit,
            selectedOctreeLeafCapacity, &baseAndIntegratorBytes, &sphBytes, &octreeBytes);
        std::cout << "[info] strict memory plan B (octree leaf capacity="
                  << selectedOctreeLeafCapacity << ")\n";
        std::cout << formatMemoryBreakdown(baseAndIntegratorBytes, sphBytes, octreeBytes,
                                           totalBytes, kVramBudgetBytes)
                  << "\n";
    }

    if (strictMemoryMode && totalBytes > kVramBudgetBytes) {
        throw std::runtime_error(
            std::string("[memory] strict admission-control rejected configuration\n") +
            formatMemoryBreakdown(baseAndIntegratorBytes, sphBytes, octreeBytes, totalBytes,
                                  kVramBudgetBytes) +
            "\n[memory] plan Z: strict mode abort");
    }

    if (_device->_cudaRuntimeAvailable) {
        allocateMappedMetrics();
    }
}

/*
 * @brief Documents the allocate particle buffers operation contract.
 * @param particleCapacity Input value used by this contract.
 * @return bool ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
