/*
 * @file engine/physics/octree/runtime/OctParticleSystemHost.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief CPU host orchestration for the particle system.
 */

#include "physics/octree/math/OctHostMath.hpp"
#include "physics/octree/runtime/OctParticleSystemDeviceState.hpp"
#include "physics/core/particle/PhyParticleSystem.hpp"
#include "physics/fmm/model/FmmCpu.hpp"
#include "physics/treepm/cpu/TpmCpu.hpp"

#include <algorithm>
#include <omp.h>

ParticleSystem::ParticleSystem(int numParticles, bool bootstrapInitialState)
{
    initializeRuntimeState(static_cast<std::size_t>(std::max(0, numParticles)));
    if (bootstrapInitialState) {
        buildBootstrapState(numParticles);
    }
}

ParticleSystem::ParticleSystem(std::vector<Particle> initialParticles)
    : ParticleSystem(std::move(initialParticles), false)
{
}

ParticleSystem::ParticleSystem(std::vector<Particle> initialParticles, bool enableCudaRuntime)
{
    initializeRuntimeState(initialParticles.size(), enableCudaRuntime);
    _particles = std::move(initialParticles);
}

ParticleSystem::~ParticleSystem()
{
    releaseParticleBuffers();
}

bool ParticleSystem::update(float deltaTime)
{
    if (_particles.empty() || deltaTime <= 0.0f) {
        return false;
    }
    if (isComovingCosmology(_cosmology)) {
        return updateComovingCosmology(deltaTime);
    }
    if (_adaptiveTimeStepsEnabled && !_adaptiveTimeStepCostGuard) {
        return updateAdaptiveTimeSteps(deltaTime);
    }
    if (_deterministicMode) {
        omp_set_dynamic(0);
    }

    const std::size_t count = _particles.size();
    std::vector<Vector3> accelerations(count, Vector3());
    if (!computeHostAccelerations(accelerations)) {
        return false;
    }
    for (std::size_t index = 0; index < count; ++index) {
        _particles[index].setPressure(accelerations[index] * 100.0f);
    }
    for (std::size_t index = 0; index < count; ++index) {
        Vector3 velocity = _particles[index].getVelocity() + accelerations[index] * deltaTime;
        velocity = blitzar_physics_particle_system_host::clampedVector(velocity, _sphMaxSpeed);
        _particles[index].setVelocity(velocity);
        _particles[index].setPosition(_particles[index].getPosition() + velocity * deltaTime);
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
