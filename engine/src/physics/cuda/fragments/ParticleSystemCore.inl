/*
 * Module: physics/cuda
 * Responsibility: Implement particle-system construction and core mode setters.
 */

/// Description: Executes the buildBootstrapState operation.
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
        p.setPosition(Vector3{
            rand() / (float)RAND_MAX * 10.0f + 1.5f,
            rand() / (float)RAND_MAX * 10.0f + 1.5f,
            0.0f
        });
        const float angle = rand() / (float)RAND_MAX * 2.0f * kPi;
        p.setPosition(Vector3{
            p.getPosition().x * cosf(angle) - p.getPosition().y * sinf(angle),
            p.getPosition().x * sinf(angle) + p.getPosition().y * cosf(angle),
            0.0f
        });
        p.setMass(diskMassPerParticle);
        const float radius = std::max(p.getPosition().norm(), 1e-4f);
        const float enclosedFraction = std::clamp((radius * radius - radiusMin * radiusMin) / radiusRange2, 0.0f, 1.0f);
        const float enclosedMass = massTerre + diskMass * enclosedFraction;
        const float orbitalSpeed = sqrtf(enclosedMass / radius);
        p.setVelocity(Vector3{
            -p.getPosition().y * orbitalSpeed / radius,
            p.getPosition().x * orbitalSpeed / radius,
            0.0f
        });
        _particles.push_back(p);
    }
}

/// Description: Executes the ParticleSystem operation.
ParticleSystem::ParticleSystem(int numParticles, bool bootstrapInitialState) {
    const int clampedParticles = std::max(2, numParticles);
    /// Description: Executes the initializeRuntimeState operation.
    initializeRuntimeState(static_cast<std::size_t>(clampedParticles));
    if (bootstrapInitialState) {
        /// Description: Executes the buildBootstrapState operation.
        buildBootstrapState(clampedParticles);
    } else {
        _particles.assign(static_cast<std::size_t>(clampedParticles), Particle{});
    }
    if (_sphEnabled) {
        fprintf(stdout, "[sph] enabled h=%f restDensity=%f gas=%f viscosity=%f\n",
            _sphSmoothingLength, _sphRestDensity, _sphGasConstant, _sphViscosity);
    }
    if (!allocateParticleBuffers(static_cast<std::size_t>(clampedParticles))) return;
    if (bootstrapInitialState && !seedDeviceState()) return;

    if (_solverMode == SolverMode::OctreeGpu) {
        if (!ensureLinearOctreeScratchCapacity(clampedParticles)) {
            fprintf(stderr,
                    "[octree-gpu] scratch preallocation failed, falling back to pairwise_cuda\n");
            _solverMode = SolverMode::PairwiseCuda;
        }
    }

    if (!allocateSphBuffers(clampedParticles) || !allocateSphGridBuffers(clampedParticles)) {
        /// Description: Executes the fprintf operation.
        fprintf(stderr, "[sph] buffers allocation failed, SPH disabled\n");
        _sphEnabled = false;
    }
    if (_solverMode != SolverMode::OctreeCpu &&
        (_integratorMode == IntegratorMode::Rk4 || _integratorMode == IntegratorMode::Leapfrog)) {
        if (!allocateRk4Buffers(clampedParticles)) {
            /// Description: Executes the runtime_error operation.
            throw std::runtime_error("[integrator] failed to allocate required RK4/Leapfrog buffers");
        }
    }
}

/// Description: Executes the ParticleSystem operation.
ParticleSystem::ParticleSystem(std::vector<Particle> initialParticles)
{
    const std::size_t particleCapacity = std::max<std::size_t>(2u, initialParticles.size());
    /// Description: Executes the initializeRuntimeState operation.
    initializeRuntimeState(particleCapacity);
    _particles = std::move(initialParticles);
    if (_particles.size() < particleCapacity) _particles.resize(particleCapacity);

    if (_sphEnabled) {
        fprintf(stdout, "[sph] enabled h=%f restDensity=%f gas=%f viscosity=%f\n",
            _sphSmoothingLength, _sphRestDensity, _sphGasConstant, _sphViscosity);
    }
    if (!allocateParticleBuffers(particleCapacity)) return;
    if (!seedDeviceState()) return;

    if (_solverMode == SolverMode::OctreeGpu) {
        if (!ensureLinearOctreeScratchCapacity(static_cast<int>(particleCapacity))) {
            fprintf(stderr,
                    "[octree-gpu] scratch preallocation failed, falling back to pairwise_cuda\n");
            _solverMode = SolverMode::PairwiseCuda;
        }
    }

    const int clampedParticles = static_cast<int>(particleCapacity);
    if (!allocateSphBuffers(clampedParticles) || !allocateSphGridBuffers(clampedParticles)) {
        _sphEnabled = false;
    }
    if (_solverMode != SolverMode::OctreeCpu &&
        (_integratorMode == IntegratorMode::Rk4 || _integratorMode == IntegratorMode::Leapfrog)) {
        if (!allocateRk4Buffers(clampedParticles)) {
            /// Description: Executes the runtime_error operation.
            throw std::runtime_error("[integrator] failed to allocate required RK4/Leapfrog buffers");
        }
    }
}

/// Description: Releases resources owned by ParticleSystem.
ParticleSystem::~ParticleSystem() {
    /// Description: Executes the releaseParticleBuffers operation.
    releaseParticleBuffers();
}

const std::vector<Particle> &ParticleSystem::getParticles() const { return _particles; }

const GpuSystemMetrics *ParticleSystem::getMappedGpuMetrics() const
{
    return _mappedMetricsHost;
}

/// Description: Executes the setParticles operation.
bool ParticleSystem::setParticles(std::vector<Particle> particles)
{
    if (particles.empty() || particles.size() != _deviceParticleCapacity) return false;
    _particles = std::move(particles);
    _hostStateDirty = false;
    _leapfrogPrimed = false;
    if (_solverMode == SolverMode::OctreeGpu) {
        if (!ensureLinearOctreeScratchCapacity(static_cast<int>(_particles.size()))) {
            fprintf(stderr,
                    "[octree-gpu] scratch preallocation failed after setParticles, falling back to pairwise_cuda\n");
            _solverMode = SolverMode::PairwiseCuda;
        }
    }
    return true;
}

/// Description: Executes the setUseOctree operation.
void ParticleSystem::setUseOctree(bool enabled)
{
    if (!enabled) {
        _solverMode = SolverMode::PairwiseCuda;
        return;
    }
    if (!ensureLinearOctreeScratchCapacity(static_cast<int>(_particles.size()))) {
        fprintf(stderr,
                "[octree-gpu] scratch preallocation failed, keeping current solver\n");
        return;
    }
    _solverMode = SolverMode::OctreeGpu;
}
/// Description: Executes the usesOctree operation.
bool ParticleSystem::usesOctree() const { return _solverMode != SolverMode::PairwiseCuda; }
/// Description: Executes the setOctreeTheta operation.
void ParticleSystem::setOctreeTheta(float theta) { if (theta > 0.01f) _octreeTheta = theta; }
/// Description: Executes the setOctreeOpeningCriterion operation.
void ParticleSystem::setOctreeOpeningCriterion(OctreeOpeningCriterion criterion) { _octreeOpeningCriterion = criterion; }
/// Description: Executes the setOctreeSoftening operation.
void ParticleSystem::setOctreeSoftening(float softening) { if (softening > 1e-5f) _octreeSoftening = softening; }
/// Description: Executes the setSphEnabled operation.
void ParticleSystem::setSphEnabled(bool enabled) { _sphEnabled = enabled; }
/// Description: Executes the isSphEnabled operation.
bool ParticleSystem::isSphEnabled() const { return _sphEnabled; }
/// Description: Executes the setSphParameters operation.
void ParticleSystem::setSphParameters(float h, float rho, float k, float mu) {
    if (h > 0.05f) _sphSmoothingLength = h;
    if (rho > 0.01f) _sphRestDensity = rho;
    if (k > 0.01f) _sphGasConstant = k;
    if (mu >= 0.0f) _sphViscosity = mu;
}
/// Description: Executes the setPhysicsStabilityConstants operation.
void ParticleSystem::setPhysicsStabilityConstants(float maxA, float minS, float minD2, float minT) {
    if (maxA > 0.0f) _physicsMaxAcceleration = maxA;
    if (minS >= 0.0f) _physicsMinSoftening = minS;
    if (minD2 >= 0.0f) _physicsMinDistance2 = minD2;
    if (minT >= 0.0f) _physicsMinTheta = minT;
}
/// Description: Executes the setSphCaps operation.
void ParticleSystem::setSphCaps(float maxA, float maxS) {
    if (maxA > 0.0f) _sphMaxAcceleration = maxA;
    if (maxS > 0.0f) _sphMaxSpeed = maxS;
}
/// Description: Executes the getCumulativeRadiatedEnergy operation.
float ParticleSystem::getCumulativeRadiatedEnergy() const { return _cumulativeRadiatedEnergy; }
/// Description: Executes the getThermalSpecificHeat operation.
float ParticleSystem::getThermalSpecificHeat() const { return _thermalSpecificHeat; }
/// Description: Executes the setSolverMode operation.
void ParticleSystem::setSolverMode(SolverMode mode)
{
    if (mode == SolverMode::OctreeGpu) {
        if (!ensureLinearOctreeScratchCapacity(static_cast<int>(_particles.size()))) {
            fprintf(stderr,
                    "[octree-gpu] scratch preallocation failed, keeping current solver\n");
            return;
        }
    }
    _solverMode = mode;
}
/// Description: Executes the getSolverMode operation.
ParticleSystem::SolverMode ParticleSystem::getSolverMode() const { return _solverMode; }
/// Description: Executes the setIntegratorMode operation.
void ParticleSystem::setIntegratorMode(IntegratorMode mode) {
    if (_solverMode != SolverMode::OctreeCpu
        && (mode == IntegratorMode::Rk4 || mode == IntegratorMode::Leapfrog)
        && !allocateRk4Buffers(static_cast<int>(_particles.size()))) {
        /// Description: Executes the runtime_error operation.
        throw std::runtime_error("[integrator] failed to allocate required RK4/Leapfrog buffers");
    }
    _integratorMode = mode;
    _leapfrogPrimed = false;

    std::size_t baseAndIntegratorBytes = 0u;
    std::size_t sphBytes = 0u;
    std::size_t octreeBytes = 0u;
    const std::size_t totalBytes = estimateMemoryUsage(
        _particles.size(),
        _sphEnabled,
        _solverMode,
        _integratorMode,
        65536u,
        0,
        &baseAndIntegratorBytes,
        &sphBytes,
        &octreeBytes);
    const std::string breakdown = formatMemoryBreakdown(
        baseAndIntegratorBytes,
        sphBytes,
        octreeBytes,
        totalBytes,
        6656ull * 1024ull * 1024ull);
    /// Description: Executes the fprintf operation.
    fprintf(stdout, "%s\n", breakdown.c_str());
}
/// Description: Executes the getIntegratorMode operation.
ParticleSystem::IntegratorMode ParticleSystem::getIntegratorMode() const { return _integratorMode; }
