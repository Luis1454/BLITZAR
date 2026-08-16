/*
 * @file engine/src/cuda/fragments/integration/AdaptiveReference.inl
 * @project BLITZAR
 * @brief CPU reference scheduler for dyadic individual time steps.
 */

#include <algorithm>
#include <cmath>
#include <cstdio>

/*
 * @brief Advances the CPU reference path with dyadic individual time steps.
 * @param deltaTime Outer integration interval.
 * @param forceLaw Resolved force policy shared by the selected backend.
 * @param thermalActive Whether thermal post-processing is enabled.
 * @return True when all reference substeps complete successfully.
 */
bool ParticleSystem::updateAdaptiveTimeSteps(float deltaTime, const ForceLawPolicy& forceLaw,
                                             bool thermalActive)
{
    if (!syncHostState()) {
        return false;
    }
    if (!_adaptiveTimeStepMarkerPrinted) {
        fprintf(stderr,
                "[adaptive] backend=cpu_reference scheduler=dyadic max_level=%u eta=%.4f\n",
                _adaptiveTimeStepMaxLevel, _adaptiveTimeStepEta);
        _adaptiveTimeStepMarkerPrinted = true;
    }

    const std::size_t count = _particles.size();
    const std::uint32_t levelCount = std::min<std::uint32_t>(_adaptiveTimeStepMaxLevel, 12u);
    const std::uint32_t sliceCount = 1u << levelCount;
    const float quantum = deltaTime / static_cast<float>(sliceCount);
    if (quantum <= 0.0f) {
        return false;
    }
    const bool stateChanged = _adaptiveTimeStepLevels.size() != count ||
                              _adaptiveTimeStepAccelerations.size() != count ||
                              _adaptiveTimeStepLastForceTicks.size() != count ||
                              std::abs(_adaptiveTimeStepQuantum - quantum) > 1e-12f;

    auto clampVelocityLocal = [&](Vector3 velocity) {
        const float limit = _sphMaxSpeed;
        const float magnitude = velocity.norm();
        return limit > 0.0f && magnitude > limit ? velocity * (limit / magnitude) : velocity;
    };
    auto chooseLevel = [&](Vector3 acceleration, Vector3 velocity) {
        const float accelerationMagnitude = acceleration.norm();
        const float velocityMagnitude = velocity.norm();
        const float softening = std::max(_octreeSoftening, _physicsMinSoftening);
        const float accelerationDt =
            accelerationMagnitude > 1e-6f
                ? _adaptiveTimeStepEta * std::sqrt(softening / accelerationMagnitude)
                : deltaTime;
        const float velocityDt = velocityMagnitude > 1e-6f
                                     ? _adaptiveTimeStepEta * softening / velocityMagnitude
                                     : deltaTime;
        const float stableDt = std::min(deltaTime, std::min(accelerationDt, velocityDt));
        std::uint8_t selected = static_cast<std::uint8_t>(levelCount);
        for (std::uint32_t level = 0u; level <= levelCount; ++level) {
            if (deltaTime / static_cast<float>(1u << level) <= stableDt) {
                selected = static_cast<std::uint8_t>(level);
                break;
            }
        }
        return selected;
    };

    auto computeCpuAccelerationForIndices = [&](const std::vector<Particle>& state,
                                                 const std::vector<int>& activeIndices,
                                                 std::vector<Vector3>& output) -> bool {
        if (activeIndices.empty()) {
            return true;
        }
        const bool cpuFp64Reference =
            _treePmEnabled && _treePmModel == "exact_tree" && _treePmPrecision == "fp64";
        const bool cpuTreePm = _treePmEnabled && _treePmModel != "exact_tree";
        if (cpuTreePm) {
            CpuTreePmParameters parameters;
            parameters.model = _treePmModel;
            parameters.localGrid = _treePmLocalGrid;
            parameters.gridSize = _treePmGridSize;
            parameters.cutoffFactor = _treePmCutoffFactor;
            parameters.maxLocalNeighbors = _treePmMaxLocalNeighbors;
            parameters.particleLimit = _treePmParticleLimit;
            parameters.precision = _treePmPrecision;
            parameters.assignment = _treePmAssignment;
            if (_treePmPrecision == "fp64") {
                if (!_cpuTreePmFp64Workspace) {
                    _cpuTreePmFp64Workspace = std::make_unique<CpuTreePmFp64Workspace>();
                }
                return computeCpuTreePmForcesSelectiveFp64(
                    state, activeIndices, forceLaw, parameters, *_cpuTreePmFp64Workspace,
                    _octree, _octreeOpeningCriterion, output);
            }
            if (!_cpuTreePmWorkspace) {
                _cpuTreePmWorkspace = std::make_unique<CpuTreePmWorkspace>();
            }
            return computeCpuTreePmForcesSelective(state, activeIndices, forceLaw, parameters,
                                                   *_cpuTreePmWorkspace, _octree,
                                                   _octreeOpeningCriterion, output);
        }

        if (cpuFp64Reference) {
#pragma omp parallel for schedule(static) if (!_deterministicMode)
            for (std::ptrdiff_t active = 0;
                 active < static_cast<std::ptrdiff_t>(activeIndices.size()); ++active) {
                const std::size_t target =
                    static_cast<std::size_t>(activeIndices[static_cast<std::size_t>(active)]);
                const Vector3 position = state[target].getPosition();
                Vector3 acceleration;
                for (std::size_t source = 0u; source < state.size(); ++source) {
                    if (source != target) {
                        const Vector3 delta = state[source].getPosition() - position;
                        const float distanceSquared = delta.x * delta.x + delta.y * delta.y +
                                                      delta.z * delta.z +
                                                      forceLaw.softening * forceLaw.softening;
                        if (distanceSquared > forceLaw.minDistance2) {
                            const float inverseDistance = 1.0f / std::sqrt(distanceSquared);
                            const float scale = state[source].getMass() * inverseDistance *
                                                inverseDistance * inverseDistance;
                            acceleration += delta * scale;
                        }
                    }
                }
                output[target] = clampAcceleration(acceleration, _physicsMaxAcceleration);
            }
            return true;
        }

        return computeCpuAcceleration(state, forceLaw, output);
    };

    std::vector<Vector3> refreshedAccelerations(count, Vector3());
    if (!computeCpuAcceleration(_particles, forceLaw, refreshedAccelerations)) {
        return false;
    }
    _adaptiveTimeStepAccelerations = std::move(refreshedAccelerations);
    _adaptiveTimeStepQuantum = quantum;
    _adaptiveTimeStepLevels.resize(count);
    if (stateChanged || _adaptiveTimeStepTick == 0u) {
        _adaptiveTimeStepLastForceTicks.assign(count, 0u);
    }
    for (std::size_t i = 0u; i < count; ++i) {
        _adaptiveTimeStepLevels[i] =
            chooseLevel(_adaptiveTimeStepAccelerations[i], _particles[i].getVelocity());
    }

    std::vector<Vector3> nextAccelerations(count, Vector3());
    std::vector<int> activeIndices;
    activeIndices.reserve(count);
    for (std::uint32_t slice = 0u; slice < sliceCount; ++slice) {
        for (std::size_t i = 0u; i < count; ++i) {
            const Vector3 velocity = _particles[i].getVelocity();
            const Vector3 acceleration = _adaptiveTimeStepAccelerations[i];
            _particles[i].setPosition(_particles[i].getPosition() + velocity * quantum +
                                      acceleration * (0.5f * quantum * quantum));
            _particles[i].setVelocity(clampVelocityLocal(velocity + acceleration * quantum));
        }
        activeIndices.clear();
        const std::uint64_t targetTick = _adaptiveTimeStepTick + 1u;
        for (std::size_t i = 0u; i < count; ++i) {
            const std::uint32_t cadence = 1u << (levelCount - _adaptiveTimeStepLevels[i]);
            if ((targetTick % cadence) == 0u) {
                activeIndices.push_back(static_cast<int>(i));
            }
        }
        if (!computeCpuAccelerationForIndices(_particles, activeIndices, nextAccelerations)) {
            return false;
        }
        for (const int activeIndex : activeIndices) {
            const std::size_t i = static_cast<std::size_t>(activeIndex);
            const float localDt =
                static_cast<float>(targetTick - _adaptiveTimeStepLastForceTicks[i]) * quantum;
            const Vector3 correction = nextAccelerations[i] - _adaptiveTimeStepAccelerations[i];
            _particles[i].setPosition(_particles[i].getPosition() +
                                      correction * (0.5f * localDt * localDt));
            _particles[i].setVelocity(
                clampVelocityLocal(_particles[i].getVelocity() + correction * localDt));
            _adaptiveTimeStepAccelerations[i] = nextAccelerations[i];
            _adaptiveTimeStepLastForceTicks[i] = targetTick;
            _adaptiveTimeStepLevels[i] =
                chooseLevel(nextAccelerations[i], _particles[i].getVelocity());
            _particles[i].setPressure(nextAccelerations[i] * 100.0f);
        }
        _adaptiveTimeStepTick = targetTick;
    }

    if (thermalActive) {
        applyThermalModel(deltaTime);
    }
    float scaleRatio = 1.0f;
    float previousHubble = 0.0f;
    float nextHubble = 0.0f;
    if (prepareCosmologyStep(deltaTime, scaleRatio, previousHubble, nextHubble)) {
        applyCosmologyExpansionHost(scaleRatio, previousHubble, nextHubble);
    }
    syncDeviceState();
    _device._hostStateDirty = false;
    return true;
}
