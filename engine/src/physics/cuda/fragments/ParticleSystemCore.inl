/*
 * Module: physics/cuda
 * Responsibility: Implement particle-system construction and core mode setters.
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

ParticleSystem::ParticleSystem(int numParticles, bool bootstrapInitialState) {
    const int clampedParticles = std::max(2, numParticles);
    initializeRuntimeState(static_cast<std::size_t>(clampedParticles));
    if (bootstrapInitialState) {
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
        fprintf(stderr, "[sph] buffers allocation failed, SPH disabled\n");
        _sphEnabled = false;
    }
    if (_integratorMode == IntegratorMode::Rk4 || _integratorMode == IntegratorMode::Leapfrog) {
        if (!allocateRk4Buffers(clampedParticles)) {
            throw std::runtime_error("[integrator] failed to allocate required RK4/Leapfrog buffers");
        }
    }
}

ParticleSystem::ParticleSystem(std::vector<Particle> initialParticles)
{
    const std::size_t particleCapacity = std::max<std::size_t>(2u, initialParticles.size());
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
    if (_integratorMode == IntegratorMode::Rk4 || _integratorMode == IntegratorMode::Leapfrog) {
        if (!allocateRk4Buffers(clampedParticles)) {
            throw std::runtime_error("[integrator] failed to allocate required RK4/Leapfrog buffers");
        }
    }
}

ParticleSystem::~ParticleSystem() {
    releaseParticleBuffers();
}

const std::vector<Particle> &ParticleSystem::getParticles() const { return _particles; }

const GpuSystemMetrics *ParticleSystem::getMappedGpuMetrics() const
{
    return _mappedMetricsHost;
}

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
bool ParticleSystem::usesOctree() const { return _solverMode != SolverMode::PairwiseCuda; }
void ParticleSystem::setOctreeTheta(float theta) { if (theta > 0.01f) _octreeTheta = theta; }
void ParticleSystem::setOctreeOpeningCriterion(OctreeOpeningCriterion criterion) { _octreeOpeningCriterion = criterion; }
void ParticleSystem::setOctreeSoftening(float softening) { if (softening > 1e-5f) _octreeSoftening = softening; }
void ParticleSystem::setSphEnabled(bool enabled) { _sphEnabled = enabled; }
bool ParticleSystem::isSphEnabled() const { return _sphEnabled; }
void ParticleSystem::setSphParameters(float h, float rho, float k, float mu) {
    if (h > 0.05f) _sphSmoothingLength = h;
    if (rho > 0.01f) _sphRestDensity = rho;
    if (k > 0.01f) _sphGasConstant = k;
    if (mu >= 0.0f) _sphViscosity = mu;
}
void ParticleSystem::setPhysicsStabilityConstants(float maxA, float minS, float minD2, float minT) {
    if (maxA > 0.0f) _physicsMaxAcceleration = maxA;
    if (minS >= 0.0f) _physicsMinSoftening = minS;
    if (minD2 >= 0.0f) _physicsMinDistance2 = minD2;
    if (minT >= 0.0f) _physicsMinTheta = minT;
}
void ParticleSystem::setSphCaps(float maxA, float maxS) {
    if (maxA > 0.0f) _sphMaxAcceleration = maxA;
    if (maxS > 0.0f) _sphMaxSpeed = maxS;
}
float ParticleSystem::getCumulativeRadiatedEnergy() const { return _cumulativeRadiatedEnergy; }
float ParticleSystem::getThermalSpecificHeat() const { return _thermalSpecificHeat; }
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
ParticleSystem::SolverMode ParticleSystem::getSolverMode() const { return _solverMode; }
void ParticleSystem::setIntegratorMode(IntegratorMode mode) {
    if ((mode == IntegratorMode::Rk4 || mode == IntegratorMode::Leapfrog)
        && !allocateRk4Buffers(static_cast<int>(_particles.size()))) {
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
    fprintf(stdout, "%s\n", breakdown.c_str());
}
ParticleSystem::IntegratorMode ParticleSystem::getIntegratorMode() const { return _integratorMode; }
