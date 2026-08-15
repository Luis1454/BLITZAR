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
    return _device._mappedMetricsHost;
}

/*
 * @brief Documents the set particles operation contract.
 * @param particles Input value used by this contract.
 * @return bool ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool ParticleSystem::setParticles(std::vector<Particle> particles)
{
    if (particles.empty() || particles.size() != _device._deviceParticleCapacity)
        return false;
    _particles = std::move(particles);
    _adaptiveTimeStepTick = 0u;
    _adaptiveTimeStepQuantum = 0.0f;
    _adaptiveTimeStepLevels.clear();
    _adaptiveTimeStepLastForceTicks.clear();
    _adaptiveTimeStepAccelerations.clear();
    _device._hostStateDirty = false;
    _device._leapfrogPrimed = false;
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

static float cosmologyHubbleRateCuda(const CosmologyConfig& config, float scaleFactor)
{
    const float a = std::max(scaleFactor, 1.0e-6f);
    const float radiation = config.omegaRadiation / std::pow(a, 4.0f);
    const float matter = config.omegaMatter / std::pow(a, 3.0f);
    return std::max(0.0f, config.hubbleH0) *
           std::sqrt(std::max(0.0f, radiation + matter + config.omegaLambda));
}

void ParticleSystem::setCosmologyParameters(const CosmologyConfig& config)
{
    _cosmology = config;
    _cosmology.geometry = config.geometry == "cube" ? "cube" : "sphere";
    _cosmology.boxHalfExtent = std::max(1.0e-6f, config.boxHalfExtent);
    _cosmology.sphereRadius = std::max(1.0e-6f, config.sphereRadius);
    _cosmology.hubbleH0 = std::max(0.0f, config.hubbleH0);
    _cosmology.omegaMatter = std::max(0.0f, config.omegaMatter);
    _cosmology.omegaLambda = std::max(0.0f, config.omegaLambda);
    _cosmology.omegaRadiation = std::max(0.0f, config.omegaRadiation);
    _cosmology.initialScaleFactor = std::max(config.initialScaleFactor, 1.0e-6f);
    _cosmology.perturbationAmplitude = std::clamp(config.perturbationAmplitude, 0.0f, 1.0f);
    _cosmology.peculiarVelocityScale = std::max(0.0f, config.peculiarVelocityScale);
    _cosmologyScaleFactor = _cosmology.enabled ? _cosmology.initialScaleFactor : 1.0f;
    _cosmologyTime = 0.0f;
    _cosmologyMarkerPrinted = false;
}

float ParticleSystem::getCosmologyScaleFactor() const
{
    return _cosmologyScaleFactor;
}

bool ParticleSystem::prepareCosmologyStep(float deltaTime, float& scaleRatio, float& previousHubble,
                                          float& nextHubble)
{
    if (!_cosmology.enabled || deltaTime <= 0.0f || _cosmology.hubbleH0 <= 0.0f) {
        return false;
    }
    if (!_cosmologyMarkerPrinted) {
        fprintf(stderr,
                "[cosmology] enabled geometry=%s model=flat_friedmann operator_split a0=%.6g\n",
                _cosmology.geometry.c_str(), _cosmology.initialScaleFactor);
        _cosmologyMarkerPrinted = true;
    }
    const float previousScale = std::max(_cosmologyScaleFactor, 1.0e-6f);
    previousHubble = cosmologyHubbleRateCuda(_cosmology, previousScale);
    const float midpointScale =
        std::max(previousScale + 0.5f * previousScale * previousHubble * deltaTime, 1.0e-6f);
    const float midpointHubble = cosmologyHubbleRateCuda(_cosmology, midpointScale);
    const float nextScale =
        std::max(previousScale + midpointScale * midpointHubble * deltaTime, previousScale);
    _cosmologyScaleFactor = nextScale;
    _cosmologyTime += deltaTime;
    nextHubble = cosmologyHubbleRateCuda(_cosmology, nextScale);
    scaleRatio = nextScale / previousScale;
    return scaleRatio > 1.0e-7f;
}

void ParticleSystem::applyCosmologyExpansionHost(float scaleRatio, float previousHubble,
                                                 float nextHubble)
{
    (void)previousHubble;
    (void)nextHubble;
    const float inverseScaleRatio = 1.0f / std::max(scaleRatio, 1.0e-6f);
    for (Particle& particle : _particles) {
        const Vector3 position = particle.getPosition();
        const Vector3 nextPosition = position * scaleRatio;
        const Vector3 nextVelocity = particle.getVelocity() * inverseScaleRatio;
        particle.setPosition(nextPosition);
        particle.setVelocity(nextVelocity);
    }
}

bool ParticleSystem::updateComovingCosmology(float deltaTime)
{
    if (_cosmology.geometry != "cube" || !_treePmEnabled || _treePmModel != "pm_only" ||
        _sphEnabled) {
        return false;
    }
    const float a0 = std::max(_cosmologyScaleFactor, 1.0e-6f);
    const float h0 = cosmologyHubbleRateCuda(_cosmology, a0);
    const float predictedMidpoint = std::max(a0 + 0.5f * a0 * h0 * deltaTime, 1.0e-6f);
    const float a1 =
        std::max(a0, a0 + predictedMidpoint *
                              cosmologyHubbleRateCuda(_cosmology, predictedMidpoint) * deltaTime);
    const float amid = 0.5f * (a0 + a1);
    const auto driftIntegrand = [this](float a) {
        return 1.0f / (a * a * a * std::max(cosmologyHubbleRateCuda(_cosmology, a), 1.0e-12f));
    };
    const float drift =
        (a1 - a0) * (driftIntegrand(a0) + 4.0f * driftIntegrand(amid) + driftIntegrand(a1)) / 6.0f;
    const float boxLength = 2.0f * _cosmology.boxHalfExtent;
    if (a1 <= a0 || drift <= 0.0f || boxLength <= 0.0f) {
        return false;
    }
    auto computePmField = [&](float scaleFactor, std::vector<Vector3>& acceleration) {
        CpuTreePmParameters parameters;
        parameters.model = "pm_only";
        parameters.gridSize = _treePmGridSize;
        parameters.assignment = "tsc";
        parameters.periodic = true;
        parameters.densityContrast = true;
        parameters.boxLength = boxLength;
        parameters.poissonCoefficient = 1.5f * _cosmology.hubbleH0 * _cosmology.hubbleH0 *
                                        _cosmology.omegaMatter / std::max(scaleFactor, 1.0e-6f);
        if (!_cpuTreePmWorkspace) {
            _cpuTreePmWorkspace = std::make_unique<CpuTreePmWorkspace>();
        }
        return computeCpuTreePmForces(_particles, ForceLawPolicy{}, parameters,
                                      *_cpuTreePmWorkspace, _octree, _octreeOpeningCriterion,
                                      acceleration);
    };
    std::vector<Vector3> firstAcceleration(_particles.size(), Vector3());
    _cpuTreePmWorkspace.reset();
    if (!computePmField(amid, firstAcceleration)) {
        return false;
    }
    for (std::size_t index = 0; index < _particles.size(); ++index) {
        const Vector3 momentum =
            _particles[index].getVelocity() + firstAcceleration[index] * (0.5f * deltaTime);
        const Vector3 position = _particles[index].getPosition() + momentum * drift;
        const auto wrap = [boxLength](float value) {
            const float wrapped = std::fmod(value, boxLength);
            return wrapped < 0.0f ? wrapped + boxLength : wrapped;
        };
        _particles[index].setVelocity(momentum);
        _particles[index].setPosition(
            Vector3(wrap(position.x), wrap(position.y), wrap(position.z)));
    }
    std::vector<Vector3> secondAcceleration(_particles.size(), Vector3());
    _cpuTreePmWorkspace.reset();
    if (!computePmField(a1, secondAcceleration)) {
        return false;
    }
    for (std::size_t index = 0; index < _particles.size(); ++index) {
        _particles[index].setVelocity(_particles[index].getVelocity() +
                                      secondAcceleration[index] * (0.5f * deltaTime));
        _particles[index].setPressure(secondAcceleration[index] * 100.0f);
    }
    _cosmologyScaleFactor = a1;
    _cosmologyTime += deltaTime;
    return true;
}

/*
 * @brief Documents the set sph parameters operation contract.
 * @param h Input value used by this contract.
 * @param rho Input value used by this contract.
 * @param k Input value used by this contract.
 * @param mu Input value used by this contract.
 * @return void ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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
    _device._leapfrogPrimed = false;

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
