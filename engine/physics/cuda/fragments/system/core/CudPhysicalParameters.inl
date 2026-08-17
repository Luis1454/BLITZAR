/*
 * @file engine/physics/cuda/fragments/system/core/CudPhysicalParameters.inl
 * @project BLITZAR
 * @brief Particle-system CUDA core implementation fragment.
 */

void ParticleSystem::setSphParameters(float h, float rho, float k, float mu)
{
    if (h > 0.05f)
        _sphSmoothingLength = h;
    if (rho > 0.01f)
        _sphRestDensity = rho;
    if (k > 0.01f)
        _sphGasConstant = k;
    if (mu >= 0.0f)
        _sphViscosity = mu;
}

/*
 * @brief Documents the set physics stability constants operation contract.
 * @param maxA Input value used by this contract.
 * @param minS Input value used by this contract.
 * @param minD2 Input value used by this contract.
 * @param minT Input value used by this contract.
 * @return void ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void ParticleSystem::setPhysicsStabilityConstants(float maxA, float minS, float minD2, float minT)
{
    if (maxA > 0.0f)
        _physicsMaxAcceleration = maxA;
    if (minS >= 0.0f)
        _physicsMinSoftening = minS;
    if (minD2 >= 0.0f)
        _physicsMinDistance2 = minD2;
    if (minT >= 0.0f)
        _physicsMinTheta = minT;
}

/*
 * @brief Documents the set sph caps operation contract.
 * @param maxA Input value used by this contract.
 * @param maxS Input value used by this contract.
 * @return void ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void ParticleSystem::setSphCaps(float maxA, float maxS)
{
    if (maxA > 0.0f)
        _sphMaxAcceleration = maxA;
    if (maxS > 0.0f)
        _sphMaxSpeed = maxS;
}

/*
 * @brief Documents the get cumulative radiated energy operation contract.
 * @param None This contract does not take explicit parameters.
 * @return float ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
float ParticleSystem::getCumulativeRadiatedEnergy() const
{
    return _cumulativeRadiatedEnergy;
}

/*
 * @brief Documents the get thermal specific heat operation contract.
 * @param None This contract does not take explicit parameters.
 * @return float ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
float ParticleSystem::getThermalSpecificHeat() const
{
    return _thermalSpecificHeat;
}

/*
 * @brief Documents the set solver mode operation contract.
 * @param mode Input value used by this contract.
 * @return void ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void ParticleSystem::setSolverMode(SolverMode mode)
{
    if (mode == SolverMode::OctreeGpu &&
        !treePmFastPathBypassesOctreeScratch(_integratorMode == IntegratorMode::Euler)) {
        if (!ensureLinearOctreeScratchCapacity(static_cast<int>(_particles.size()))) {
            fprintf(stderr, "[octree-gpu] scratch preallocation failed, keeping current solver\n");
            return;
        }
    }
    _solverMode = mode;
}

/*
 * @brief Documents the get solver mode operation contract.
 * @param None This contract does not take explicit parameters.
 * @return ParticleSystem::SolverMode ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
ParticleSystem::SolverMode ParticleSystem::getSolverMode() const
{
    return _solverMode;
}

/*
 * @brief Documents the set integrator mode operation contract.
 * @param mode Input value used by this contract.
 * @return void ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void ParticleSystem::setIntegratorMode(IntegratorMode mode)
{
    if (_integratorMode == mode) {
        return;
    }
    if (_solverMode != SolverMode::OctreeCpu && _solverMode != SolverMode::FmmCpu &&
        (mode == IntegratorMode::Rk4 || mode == IntegratorMode::Leapfrog) &&
        !allocateRk4Buffers(static_cast<int>(_particles.size()))) {
        throw std::runtime_error("[integrator] failed to allocate required RK4/Leapfrog buffers");
    }
    _integratorMode = mode;
    _device->_leapfrogPrimed = false;

    std::size_t baseAndIntegratorBytes = 0u;
    std::size_t sphBytes = 0u;
    std::size_t octreeBytes = 0u;
    const std::size_t totalBytes =
        estimateMemoryUsage(_particles.size(), _sphEnabled, _solverMode, _integratorMode, 65536u, 0,
                            &baseAndIntegratorBytes, &sphBytes, &octreeBytes);
    const std::string breakdown = formatMemoryBreakdown(
        baseAndIntegratorBytes, sphBytes, octreeBytes, totalBytes, 6656ull * 1024ull * 1024ull);
    fprintf(stdout, "%s\n", breakdown.c_str());
}

/*
 * @brief Documents the get integrator mode operation contract.
 * @param None This contract does not take explicit parameters.
 * @return ParticleSystem::IntegratorMode ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
ParticleSystem::IntegratorMode ParticleSystem::getIntegratorMode() const
{
    return _integratorMode;
}
