/*
 * @file engine/physics/octree/adaptive/OctAdaptive.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Individual dyadic adaptive time-step integration on the host.
 */

#include "physics/octree/model/OctHostMath.hpp"
#include "physics/octree/force/OctreeForce.hpp"
#include "physics/octree/model/OctParticleSystemDeviceState.hpp"
#include "physics/core/particle/PhyParticleSystem.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

bool ParticleSystem::updateAdaptiveTimeSteps(float deltaTime)
{
    if (!_adaptiveTimeStepMarkerPrinted) {
        fprintf(stderr, "[adaptive] backend=cpu_reference scheduler=dyadic max_level=%u eta=%.4f\n",
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

    auto chooseLevel = [&](Vector3 acceleration, Vector3 velocity) -> std::uint8_t {
        const float accelerationMagnitude =
            std::sqrt(blitzar_physics_particle_system_host::squaredLength(acceleration));
        const float velocityMagnitude =
            std::sqrt(blitzar_physics_particle_system_host::squaredLength(velocity));
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

    const bool stateChanged = _adaptiveTimeStepLevels.size() != count ||
                              _adaptiveTimeStepAccelerations.size() != count ||
                              _adaptiveTimeStepLastForceTicks.size() != count ||
                              std::abs(_adaptiveTimeStepQuantum - quantum) > 1e-12f;
    std::vector<Vector3> refreshedAccelerations(count, Vector3());
    if (!computeHostAccelerations(refreshedAccelerations)) {
        return false;
    }
    if (stateChanged || _adaptiveTimeStepTick == 0u) {
        _adaptiveTimeStepLevels.resize(count);
        _adaptiveTimeStepLastForceTicks.assign(count, 0u);
        _adaptiveTimeStepAccelerations = std::move(refreshedAccelerations);
        _adaptiveTimeStepQuantum = quantum;
        for (std::size_t index = 0; index < count; ++index) {
            _adaptiveTimeStepLevels[index] =
                chooseLevel(_adaptiveTimeStepAccelerations[index], _particles[index].getVelocity());
            _particles[index].setPressure(_adaptiveTimeStepAccelerations[index] * 100.0f);
        }
    }
    else {
        _adaptiveTimeStepAccelerations = std::move(refreshedAccelerations);
    }

    std::vector<Vector3> nextAccelerations = _adaptiveTimeStepAccelerations;
    std::vector<int> activeIndices;
    activeIndices.reserve(count);
    for (std::uint32_t slice = 0u; slice < sliceCount; ++slice) {
        for (std::size_t index = 0; index < count; ++index) {
            const Vector3 velocity = _particles[index].getVelocity();
            const Vector3 acceleration = _adaptiveTimeStepAccelerations[index];
            _particles[index].setPosition(_particles[index].getPosition() + velocity * quantum +
                                          acceleration * (0.5f * quantum * quantum));
            _particles[index].setVelocity(blitzar_physics_particle_system_host::clampedVector(
                velocity + acceleration * quantum, _sphMaxSpeed));
        }

        const std::uint64_t targetTick = _adaptiveTimeStepTick + 1u;
        activeIndices.clear();
        for (std::size_t index = 0u; index < count; ++index) {
            const std::uint32_t cadence = 1u << (levelCount - _adaptiveTimeStepLevels[index]);
            if ((targetTick % cadence) == 0u) {
                activeIndices.push_back(static_cast<int>(index));
            }
        }
        if (!computeHostAccelerationsForIndices(activeIndices, nextAccelerations)) {
            return false;
        }
        for (const int activeIndex : activeIndices) {
            const std::size_t index = static_cast<std::size_t>(activeIndex);
            const float localDt =
                static_cast<float>(targetTick - _adaptiveTimeStepLastForceTicks[index]) * quantum;
            const Vector3 correction =
                nextAccelerations[index] - _adaptiveTimeStepAccelerations[index];
            _particles[index].setPosition(_particles[index].getPosition() +
                                          correction * (0.5f * localDt * localDt));
            _particles[index].setVelocity(blitzar_physics_particle_system_host::clampedVector(
                _particles[index].getVelocity() + correction * localDt, _sphMaxSpeed));
            _adaptiveTimeStepAccelerations[index] = nextAccelerations[index];
            _adaptiveTimeStepLastForceTicks[index] = targetTick;
            _adaptiveTimeStepLevels[index] =
                chooseLevel(nextAccelerations[index], _particles[index].getVelocity());
        }
        for (std::size_t index = 0u; index < count; ++index) {
            _particles[index].setPressure(_adaptiveTimeStepAccelerations[index] * 100.0f);
        }
        _adaptiveTimeStepTick = targetTick;
    }

    float scaleRatio = 1.0f;
    float previousHubble = 0.0f;
    float nextHubble = 0.0f;
    if (prepareCosmologyStep(deltaTime, scaleRatio, previousHubble, nextHubble)) {
        applyCosmologyExpansionHost(scaleRatio, previousHubble, nextHubble);
    }
    _cumulativeRadiatedEnergy += applyThermalModel(deltaTime);
    _device->_hostStateDirty = false;
    return true;
}
