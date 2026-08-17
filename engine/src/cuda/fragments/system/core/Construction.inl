/*
 * @file engine/src/cuda/fragments/system/core/Construction.inl
 * @project BLITZAR
 * @brief Particle-system CUDA core implementation fragment.
 */

/*
 * @file engine/src/cuda/fragments/system/Core.inl
 * @author Luis1454
 * @project BLITZAR
 * @brief Physics and CUDA implementation for the deterministic simulation core.
 */

/*
 * Module: cuda
 * Responsibility: Implement particle-system construction and core mode setters.
 */

/*
 * @brief Documents the build bootstrap state operation contract.
 * @param particleCount Input value used by this contract.
 * @return void ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void ParticleSystem::buildBootstrapState(int particleCount)
{
    Particle p;
    p.setVelocity(Vector3{0, 0, 0});
    float massTerre = 1.0f;
    float diskMass = 0.75f * massTerre;
    const int diskParticleCount = std::max(1, particleCount - 1);
    const float diskMassPerParticle = diskMass / static_cast<float>(diskParticleCount);
    const float radiusMin = 1.5f;
    const float radiusMax = 11.5f;
    const float radiusRange2 = std::max(1e-6f, radiusMax * radiusMax - radiusMin * radiusMin);
    p.setMass(massTerre);
    p.setPosition(Vector3{0, 0, 0});
    _particles.push_back(p);
    for (int i = 1; i < particleCount; ++i) {
        p.setPosition(Vector3{rand() / (float)RAND_MAX * 10.0f + 1.5f,
                              rand() / (float)RAND_MAX * 10.0f + 1.5f, 0.0f});
        const float angle = rand() / (float)RAND_MAX * 2.0f * kPi;
        p.setPosition(Vector3{p.getPosition().x * cosf(angle) - p.getPosition().y * sinf(angle),
                              p.getPosition().x * sinf(angle) + p.getPosition().y * cosf(angle),
                              0.0f});
        p.setMass(diskMassPerParticle);
        const float radius = std::max(p.getPosition().norm(), 1e-4f);
        const float enclosedFraction =
            std::clamp((radius * radius - radiusMin * radiusMin) / radiusRange2, 0.0f, 1.0f);
        const float enclosedMass = massTerre + diskMass * enclosedFraction;
        const float orbitalSpeed = sqrtf(enclosedMass / radius);
        p.setVelocity(Vector3{-p.getPosition().y * orbitalSpeed / radius,
                              p.getPosition().x * orbitalSpeed / radius, 0.0f});
        _particles.push_back(p);
    }
}

/*
 * @brief Documents the particle system operation contract.
 * @param numParticles Input value used by this contract.
 * @param bootstrapInitialState Input value used by this contract.
 * @return ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
ParticleSystem::ParticleSystem(int numParticles, bool bootstrapInitialState)
{
    const int clampedParticles = std::max(2, numParticles);
    initializeRuntimeState(static_cast<std::size_t>(clampedParticles));
    if (bootstrapInitialState) {
        buildBootstrapState(clampedParticles);
    }
    else {
        _particles.assign(static_cast<std::size_t>(clampedParticles), Particle{});
    }
    if (_sphEnabled) {
        fprintf(stdout, "[sph] enabled h=%f restDensity=%f gas=%f viscosity=%f\n",
                _sphSmoothingLength, _sphRestDensity, _sphGasConstant, _sphViscosity);
    }
    if (!allocateParticleBuffers(static_cast<std::size_t>(clampedParticles)))
        return;
    if (bootstrapInitialState && !seedDeviceState())
        return;

    if (_solverMode == SolverMode::OctreeGpu &&
        !treePmFastPathBypassesOctreeScratch(_integratorMode == IntegratorMode::Euler)) {
        if (!ensureLinearOctreeScratchCapacity(clampedParticles)) {
            fprintf(stderr,
                    "[octree-gpu] scratch preallocation failed, falling back to pairwise_cuda\n");
            _solverMode = SolverMode::PairwiseCuda;
        }
    }

    if (_sphEnabled &&
        (!allocateSphBuffers(clampedParticles) || !allocateSphGridBuffers(clampedParticles))) {
        fprintf(stderr, "[sph] buffers allocation failed, SPH disabled\n");
        _sphEnabled = false;
    }
    if (_solverMode != SolverMode::OctreeCpu && _solverMode != SolverMode::FmmCpu &&
        (_integratorMode == IntegratorMode::Rk4 || _integratorMode == IntegratorMode::Leapfrog)) {
        if (!allocateRk4Buffers(clampedParticles)) {
            throw std::runtime_error(
                "[integrator] failed to allocate required RK4/Leapfrog buffers");
        }
    }
}

/*
 * @brief Documents the particle system operation contract.
 * @param initialParticles Input value used by this contract.
 * @return ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
ParticleSystem::ParticleSystem(std::vector<Particle> initialParticles)
    : ParticleSystem(std::move(initialParticles), true)
{
}

ParticleSystem::ParticleSystem(std::vector<Particle> initialParticles, bool enableCudaRuntime)
{
    const std::size_t particleCapacity = std::max<std::size_t>(2u, initialParticles.size());
    initializeRuntimeState(particleCapacity, enableCudaRuntime);
    _particles = std::move(initialParticles);
    if (_particles.size() < particleCapacity)
        _particles.resize(particleCapacity);

    if (_sphEnabled) {
        fprintf(stdout, "[sph] enabled h=%f restDensity=%f gas=%f viscosity=%f\n",
                _sphSmoothingLength, _sphRestDensity, _sphGasConstant, _sphViscosity);
    }
    if (!allocateParticleBuffers(particleCapacity))
        return;
    if (!seedDeviceState())
        return;

    if (_solverMode == SolverMode::OctreeGpu &&
        !treePmFastPathBypassesOctreeScratch(_integratorMode == IntegratorMode::Euler)) {
        if (!ensureLinearOctreeScratchCapacity(static_cast<int>(particleCapacity))) {
            fprintf(stderr,
                    "[octree-gpu] scratch preallocation failed, falling back to pairwise_cuda\n");
            _solverMode = SolverMode::PairwiseCuda;
        }
    }

    const int clampedParticles = static_cast<int>(particleCapacity);
    if (_sphEnabled &&
        (!allocateSphBuffers(clampedParticles) || !allocateSphGridBuffers(clampedParticles))) {
        _sphEnabled = false;
    }
    if (_solverMode != SolverMode::OctreeCpu && _solverMode != SolverMode::FmmCpu &&
        (_integratorMode == IntegratorMode::Rk4 || _integratorMode == IntegratorMode::Leapfrog)) {
        if (!allocateRk4Buffers(clampedParticles)) {
            throw std::runtime_error(
                "[integrator] failed to allocate required RK4/Leapfrog buffers");
        }
    }
}

/*
 * @brief Documents the ~particle system operation contract.
 * @param None This contract does not take explicit parameters.
 * @return ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
ParticleSystem::~ParticleSystem()
{
    releaseParticleBuffers();
}

/*
 * @brief Documents the get particles operation contract.
 * @param None This contract does not take explicit parameters.
 * @return const std::vector<Particle> &ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
const std::vector<Particle>& ParticleSystem::getParticles() const
{
    return _particles;
}

/*
 * @brief Documents the get mapped gpu metrics operation contract.
 * @param None This contract does not take explicit parameters.
 * @return const GpuSystemMetrics *ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
const GpuSystemMetrics* ParticleSystem::getMappedGpuMetrics() const
{
    return _device->_mappedMetricsHost;
}

/*
 * @brief Documents the set particles operation contract.
 * @param particles Input value used by this contract.
 * @return bool ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool ParticleSystem::setParticles(std::vector<Particle> particles)
{
    if (particles.empty() || particles.size() != _device->_deviceParticleCapacity)
        return false;
    _particles = std::move(particles);
    _adaptiveTimeStepTick = 0u;
    _adaptiveTimeStepQuantum = 0.0f;
    _adaptiveTimeStepLevels.clear();
    _adaptiveTimeStepLastForceTicks.clear();
    _adaptiveTimeStepAccelerations.clear();
    _device->_hostStateDirty = false;
    _device->_leapfrogPrimed = false;
    if (_solverMode == SolverMode::OctreeGpu &&
        !treePmFastPathBypassesOctreeScratch(_integratorMode == IntegratorMode::Euler)) {
        if (!ensureLinearOctreeScratchCapacity(static_cast<int>(_particles.size()))) {
            fprintf(stderr, "[octree-gpu] scratch preallocation failed after setParticles, falling "
                            "back to pairwise_cuda\n");
            _solverMode = SolverMode::PairwiseCuda;
        }
    }
    return true;
}

/*
 * @brief Documents the set use octree operation contract.
 * @param enabled Input value used by this contract.
 * @return void ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
