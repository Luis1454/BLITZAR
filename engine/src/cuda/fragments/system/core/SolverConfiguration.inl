/*
 * @file engine/src/cuda/fragments/system/core/SolverConfiguration.inl
 * @project BLITZAR
 * @brief Particle-system CUDA core implementation fragment.
 */

void ParticleSystem::setUseOctree(bool enabled)
{
    if (!enabled) {
        _solverMode = SolverMode::PairwiseCuda;
        return;
    }
    if (!treePmFastPathBypassesOctreeScratch(_integratorMode == IntegratorMode::Euler) &&
        !ensureLinearOctreeScratchCapacity(static_cast<int>(_particles.size()))) {
        fprintf(stderr, "[octree-gpu] scratch preallocation failed, keeping current solver\n");
        return;
    }
    _solverMode = SolverMode::OctreeGpu;
}

/*
 * @brief Documents the uses octree operation contract.
 * @param None This contract does not take explicit parameters.
 * @return bool ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool ParticleSystem::usesOctree() const
{
    return _solverMode != SolverMode::PairwiseCuda;
}

void ParticleSystem::setOctreeTheta(float theta)
{
    if (theta > 0.01f)
        _octreeTheta = theta;
}

/*
 * @brief Documents the set octree opening criterion operation contract.
 * @param criterion Input value used by this contract.
 * @return void ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void ParticleSystem::setOctreeOpeningCriterion(OctreeOpeningCriterion criterion)
{
    _octreeOpeningCriterion = criterion;
}

void ParticleSystem::setOctreeSoftening(float softening)
{
    if (softening > 1e-5f)
        _octreeSoftening = softening;
}

void ParticleSystem::setTreePmParameters(bool enabled, const std::string& model,
                                         const std::string& layout, const std::string& precision,
                                         const std::string& assignment, bool localGrid,
                                         int gridSize, int jacobiIterations, float cutoffFactor,
                                         int maxLocalNeighbors, int particleLimit,
                                         int denseCellThreshold, bool gravityOnlyBuffers)
{
    _treePmEnabled = enabled;
    _treePmModel = model;
    _treePmLayout = layout;
    _treePmPrecision = precision == "fp64" ? "fp64" : "fp32";
    _treePmAssignment = assignment == "tsc" || assignment == "pcs" ? assignment : "cic";
    _treePmLocalGrid = localGrid;
    _treePmGridSize = std::clamp(gridSize, 32, 128);
    _treePmJacobiIterations = std::clamp(jacobiIterations, 4, 64);
    _treePmCutoffFactor = std::clamp(cutoffFactor, 1.0f, 2.0f);
    _treePmMaxLocalNeighbors = std::clamp(maxLocalNeighbors, 0, 256);
    _treePmParticleLimit = std::max(particleLimit, 0);
    _treePmDenseCellThreshold = std::max(denseCellThreshold, 1);
    _treePmGravityOnlyBuffers = gravityOnlyBuffers;
}

void ParticleSystem::setAdaptiveTimeStepParameters(bool enabled, std::uint32_t maxLevel, float eta)
{
    const std::uint32_t safeLevel = std::min<std::uint32_t>(maxLevel, 12u);
    const float safeEta = std::clamp(eta, 0.01f, 1.0f);
    if (_adaptiveTimeStepsEnabled != enabled || _adaptiveTimeStepMaxLevel != safeLevel ||
        std::abs(_adaptiveTimeStepEta - safeEta) > 1e-6f) {
        _adaptiveTimeStepTick = 0u;
        _adaptiveTimeStepQuantum = 0.0f;
        _adaptiveTimeStepMarkerPrinted = false;
        _adaptiveTimeStepLevels.clear();
        _adaptiveTimeStepLastForceTicks.clear();
        _adaptiveTimeStepAccelerations.clear();
    }
    _adaptiveTimeStepsEnabled = enabled;
    _adaptiveTimeStepMaxLevel = safeLevel;
    _adaptiveTimeStepEta = safeEta;
}

void ParticleSystem::setAdaptiveTimeStepCostGuard(bool enabled)
{
    if (_adaptiveTimeStepCostGuard != enabled) {
        _adaptiveTimeStepMarkerPrinted = false;
    }
    _adaptiveTimeStepCostGuard = enabled;
}

void ParticleSystem::setLinearOctreeLeafCapacity(int capacity)
{
    _fmmLeafCapacity = std::clamp(capacity, 1, 1024);
    _device._linearOctreeLeafCapacity = std::max(16, capacity);
}

void ParticleSystem::setCudaCachePreference(const std::string& preference)
{
    if (preference == "default" || preference == "l1" || preference == "shared") {
        _cudaCachePreference = preference;
    }
}

bool ParticleSystem::reconfigureRuntimeBuffers()
{
    if (!_device._cudaRuntimeAvailable) {
        return true;
    }

    const std::size_t particleCapacity = std::max<std::size_t>(2u, _particles.size());
    releaseParticleBuffers();
    _device._deviceParticleCapacity = particleCapacity;
    if (!allocateParticleBuffers(particleCapacity) || !seedDeviceState()) {
        return false;
    }
    if (_solverMode == SolverMode::OctreeGpu &&
        !treePmFastPathBypassesOctreeScratch(_integratorMode == IntegratorMode::Euler) &&
        !ensureLinearOctreeScratchCapacity(static_cast<int>(particleCapacity))) {
        return false;
    }
    if (_sphEnabled && (!allocateSphBuffers(static_cast<int>(particleCapacity)) ||
                        !allocateSphGridBuffers(static_cast<int>(particleCapacity)))) {
        return false;
    }
    if (_solverMode != SolverMode::OctreeCpu && _solverMode != SolverMode::FmmCpu &&
        (_integratorMode == IntegratorMode::Rk4 || _integratorMode == IntegratorMode::Leapfrog) &&
        !allocateRk4Buffers(static_cast<int>(particleCapacity))) {
        return false;
    }
    if (!allocateMappedMetrics()) {
        return false;
    }
    std::size_t baseAndIntegratorBytes = 0u;
    std::size_t sphBytes = 0u;
    std::size_t octreeBytes = 0u;
    const std::size_t totalBytes = estimateMemoryUsage(
        particleCapacity, _sphEnabled, _solverMode, _integratorMode, 65536u,
        _device._linearOctreeLeafCapacity, &baseAndIntegratorBytes, &sphBytes, &octreeBytes);
    fprintf(stdout, "[info] [memory] configured runtime plan\n%s\n",
            formatMemoryBreakdown(baseAndIntegratorBytes, sphBytes, octreeBytes, totalBytes,
                                  6656ull * 1024ull * 1024ull)
                .c_str());
    return true;
}

/*
 * @brief Documents the set sph enabled operation contract.
 * @param enabled Input value used by this contract.
 * @return void ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void ParticleSystem::setSphEnabled(bool enabled)
{
    _sphEnabled = enabled;
}

/*
 * @brief Documents the is sph enabled operation contract.
 * @param None This contract does not take explicit parameters.
 * @return bool ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool ParticleSystem::isSphEnabled() const
{
    return _sphEnabled;
}
