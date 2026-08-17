/*
 * @file engine/physics/core/cuda/integration/CudCpuUpdate.inl
 * @author Luis1454
 * @project BLITZAR
 * @brief Advance CPU and FMM solver states for one simulation step.
 */

/*
 * Module: cuda
 * Responsibility: Advance CPU and FMM solver states.
 */

bool ParticleSystem::updateCpuSolvers(float deltaTime, const ForceLawPolicy& forceLaw,
                                      bool thermalActive)
{
    if (!syncHostState()) {
        return false;
    }
    if (isComovingCosmology(_cosmology)) {
        if (!updateComovingCosmology(deltaTime)) {
            return false;
        }
        syncDeviceState();
        _device->_hostStateDirty = false;
        return true;
    }
    if (_integratorMode == IntegratorMode::Euler) {
        if (!computeCpuAcceleration(_particles, forceLaw, _octreeForces)) {
            return false;
        }
        for (std::size_t i = 0; i < _particles.size(); ++i) {
            _octreeForces[i] = clampAcceleration(_octreeForces[i], _physicsMaxAcceleration);
            _particles[i].setPressure(_octreeForces[i] * 100.0f);
        }
#pragma omp parallel for schedule(static) if (!_deterministicMode)
        for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(_particles.size()); ++i) {
            _particles[i].setVelocity(_particles[i].getVelocity() + _octreeForces[i] * deltaTime);
            _particles[i].setPosition(_particles[i].getPosition() +
                                      _particles[i].getVelocity() * deltaTime);
        }
        if (!this->applySphCorrection(deltaTime, true)) {
            return false;
        }
        if (_sphEnabled && !syncHostState()) {
            return false;
        }
        if (thermalActive) {
            applyThermalModel(deltaTime);
        }
        applyHostCosmologyStep(deltaTime);
        return true;
    }

    if (_integratorMode == IntegratorMode::Leapfrog) {
        const size_t n = _particles.size();
        std::vector<Vector3> accStart(n);
        std::vector<Vector3> accEnd(n);
        std::vector<Particle> stage = _particles;

        if (!computeCpuAcceleration(_particles, forceLaw, accStart)) {
            return false;
        }

#pragma omp parallel for schedule(static) if (!_deterministicMode)
        for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(n); ++i) {
            const Vector3 velHalf = _particles[i].getVelocity() + accStart[i] * (0.5f * deltaTime);
            stage[i].setVelocity(velHalf);
            stage[i].setPosition(_particles[i].getPosition() + velHalf * deltaTime);
        }

        if (!computeCpuAcceleration(stage, forceLaw, accEnd)) {
            return false;
        }

#pragma omp parallel for schedule(static) if (!_deterministicMode)
        for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(n); ++i) {
            const Vector3 velHalf = _particles[i].getVelocity() + accStart[i] * (0.5f * deltaTime);
            const Vector3 nextVel = velHalf + accEnd[i] * (0.5f * deltaTime);
            _particles[i].setPosition(stage[i].getPosition());
            _particles[i].setVelocity(nextVel);
            _particles[i].setPressure(accEnd[i] * 100.0f);
        }
        if (!this->applySphCorrection(deltaTime, true)) {
            return false;
        }
        if (_sphEnabled && !syncHostState()) {
            return false;
        }
        if (thermalActive) {
            applyThermalModel(deltaTime);
        }
        applyHostCosmologyStep(deltaTime);
        return true;
    }

    const size_t n = _particles.size();
    std::vector<Vector3> k1x(n), k2x(n), k3x(n), k4x(n);

    std::vector<Vector3> k1v(n), k2v(n), k3v(n), k4v(n);
    std::vector<Particle> stage(n);
    auto resetStage = [&]() {
#pragma omp parallel for schedule(static) if (!_deterministicMode)
        for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(n); ++i) {
            stage[i] = _particles[i];
        }
    };

    auto computeOctreeAcceleration = [&](const std::vector<Particle>& state,
                                         std::vector<Vector3>& outAcc) {
        if (!computeCpuAcceleration(state, forceLaw, outAcc)) {
            outAcc.assign(state.size(), Vector3());
        }
    };

#pragma omp parallel for schedule(static) if (!_deterministicMode)
    for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(n); ++i) {
        k1x[i] = _particles[i].getVelocity();
    }
    computeOctreeAcceleration(_particles, k1v);

    resetStage();
#pragma omp parallel for schedule(static) if (!_deterministicMode)
    for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(n); ++i) {
        stage[i].setPosition(_particles[i].getPosition() + k1x[i] * (0.5f * deltaTime));
        stage[i].setVelocity(_particles[i].getVelocity() + k1v[i] * (0.5f * deltaTime));
        k2x[i] = stage[i].getVelocity();
    }
    computeOctreeAcceleration(stage, k2v);

    resetStage();
#pragma omp parallel for schedule(static) if (!_deterministicMode)
    for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(n); ++i) {
        stage[i].setPosition(_particles[i].getPosition() + k2x[i] * (0.5f * deltaTime));
        stage[i].setVelocity(_particles[i].getVelocity() + k2v[i] * (0.5f * deltaTime));
        k3x[i] = stage[i].getVelocity();
    }
    computeOctreeAcceleration(stage, k3v);

    resetStage();
#pragma omp parallel for schedule(static) if (!_deterministicMode)
    for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(n); ++i) {
        stage[i].setPosition(_particles[i].getPosition() + k3x[i] * deltaTime);
        stage[i].setVelocity(_particles[i].getVelocity() + k3v[i] * deltaTime);
        k4x[i] = stage[i].getVelocity();
    }
    computeOctreeAcceleration(stage, k4v);

#pragma omp parallel for schedule(static) if (!_deterministicMode)
    for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(n); ++i) {
        const Vector3 weightedVel = (k1v[i] + k2v[i] * 2.0f + k3v[i] * 2.0f + k4v[i]) / 6.0f;
        const Vector3 weightedPos = (k1x[i] + k2x[i] * 2.0f + k3x[i] * 2.0f + k4x[i]) / 6.0f;
        _particles[i].setVelocity(_particles[i].getVelocity() + weightedVel * deltaTime);
        _particles[i].setPosition(_particles[i].getPosition() + weightedPos * deltaTime);
        _particles[i].setPressure(weightedVel * 100.0f);
    }
    if (!this->applySphCorrection(deltaTime, true)) {
        return false;
    }
    if (_sphEnabled && !syncHostState()) {
        return false;
    }
    if (thermalActive) {
        applyThermalModel(deltaTime);
    }
    applyHostCosmologyStep(deltaTime);
    return true;
}
