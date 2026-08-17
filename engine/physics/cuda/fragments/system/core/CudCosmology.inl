/*
 * @file engine/physics/cuda/fragments/system/core/CudCosmology.inl
 * @project BLITZAR
 * @brief Particle-system CUDA core implementation fragment.
 */

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
