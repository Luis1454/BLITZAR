/*
 * @file engine/src/physics/octree/ParticleSystemHost.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief CPU fallback implementation for the particle system.
 */

#include "Constants.hpp"
#include "OctreeForce.hpp"
#include "physics/core/ParticleSystem.hpp"
#include "physics/fmm/FmmCpu.hpp"
#include "physics/treepm/TreePmCpu.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <omp.h>
#include <sstream>
#ifdef __SSE__
#include <xmmintrin.h>
#endif

namespace blitzar_physics_particle_system_host {
float cosmologyHubbleRate(const CosmologyConfig& config, float scaleFactor)
{
    const float a = std::max(scaleFactor, 1.0e-6f);
    const float radiation = config.omegaRadiation / std::pow(a, 4.0f);
    const float matter = config.omegaMatter / std::pow(a, 3.0f);
    const float lambda = config.omegaLambda;
    return std::max(0.0f, config.hubbleH0) * std::sqrt(std::max(0.0f, radiation + matter + lambda));
}

float advanceCosmologyScaleFactor(const CosmologyConfig& config, float scaleFactor, float deltaTime)
{
    const float a0 = std::max(scaleFactor, 1.0e-6f);
    const float h0 = cosmologyHubbleRate(config, a0);
    const float midpoint = std::max(a0 + 0.5f * a0 * h0 * deltaTime, 1.0e-6f);
    return std::max(a0, a0 + midpoint * cosmologyHubbleRate(config, midpoint) * deltaTime);
}

float comovingDriftFactor(const CosmologyConfig& config, float a0, float a1)
{
    const float midpoint = 0.5f * (a0 + a1);
    const auto integrand = [&config](float a) {
        const float safeA = std::max(a, 1.0e-6f);
        return 1.0f /
               (safeA * safeA * safeA * std::max(cosmologyHubbleRate(config, safeA), 1.0e-12f));
    };
    return (a1 - a0) * (integrand(a0) + 4.0f * integrand(midpoint) + integrand(a1)) / 6.0f;
}

float wrapComovingCoordinate(float value, float boxLength)
{
    const float wrapped = std::fmod(value, boxLength);
    return wrapped < 0.0f ? wrapped + boxLength : wrapped;
}

Vector3 clampedVector(Vector3 value, float limit)
{
    const float speed = value.norm();
    if (limit <= 0.0f || speed <= limit || speed <= 1e-6f)
        return value;
    return value * (limit / speed);
}

Particle makeParticle(Vector3 position, Vector3 velocity)
{
    Particle particle;
    particle.setPosition(position);
    particle.setVelocity(velocity);
    particle.setPressure(Vector3(0.0f, 0.0f, 0.0f));
    particle.setMass(Particle::kDefaultMass);
    particle.setDensity(1.0f);
    particle.setTemperature(0.0f);
    return particle;
}
} // namespace blitzar_physics_particle_system_host

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
    if (_particles.empty() || deltaTime <= 0.0f)
        return false;

    if (isComovingCosmology(_cosmology)) {
        return updateComovingCosmology(deltaTime);
    }

    if (_adaptiveTimeStepsEnabled && !_adaptiveTimeStepCostGuard)
        return updateAdaptiveTimeSteps(deltaTime);

    if (_deterministicMode) {
        omp_set_dynamic(0);
    }

    const std::size_t count = _particles.size();
    std::vector<Vector3> accelerations(count, Vector3());
    if (!computeHostAccelerations(accelerations))
        return false;

    for (std::size_t i = 0; i < count; ++i) {
        _particles[i].setPressure(accelerations[i] * 100.0f);
    }

    for (std::size_t i = 0; i < count; ++i) {
        Vector3 velocity = _particles[i].getVelocity() + accelerations[i] * deltaTime;
        velocity = blitzar_physics_particle_system_host::clampedVector(velocity, _sphMaxSpeed);
        _particles[i].setVelocity(velocity);
        _particles[i].setPosition(_particles[i].getPosition() + velocity * deltaTime);
    }

    float scaleRatio = 1.0f;
    float previousHubble = 0.0f;
    float nextHubble = 0.0f;
    if (prepareCosmologyStep(deltaTime, scaleRatio, previousHubble, nextHubble)) {
        applyCosmologyExpansionHost(scaleRatio, previousHubble, nextHubble);
    }

    _cumulativeRadiatedEnergy += applyThermalModel(deltaTime);
    _device._hostStateDirty = false;
    return true;
}

bool ParticleSystem::updateComovingCosmology(float deltaTime)
{
    if (_cosmology.geometry != "cube" || !_treePmEnabled || _treePmModel != "pm_only" ||
        _sphEnabled) {
        fprintf(
            stderr,
            "[cosmology] comoving rejected: require cube, TreePM pm_only, SPH off, adaptive off\n");
        return false;
    }
    const float a0 = std::max(_cosmologyScaleFactor, 1.0e-6f);
    const float a1 = blitzar_physics_particle_system_host::advanceCosmologyScaleFactor(
        _cosmology, a0, deltaTime);
    const float amid = 0.5f * (a0 + a1);
    const float firstKick = 0.5f * deltaTime;
    const float drift =
        blitzar_physics_particle_system_host::comovingDriftFactor(_cosmology, a0, a1);
    const float boxLength = 2.0f * _cosmology.boxHalfExtent;
    if (a1 <= a0 || drift <= 0.0f || boxLength <= 0.0f) {
        return false;
    }

    auto computePmField = [&](float scaleFactor, std::vector<Vector3>& accelerations) {
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
                                      accelerations);
    };

    std::vector<Vector3> firstAcceleration(_particles.size(), Vector3());
    _cpuTreePmWorkspace.reset();
    if (!computePmField(amid, firstAcceleration)) {
        return false;
    }
    for (std::size_t index = 0; index < _particles.size(); ++index) {
        const Vector3 momentum =
            _particles[index].getVelocity() + firstAcceleration[index] * firstKick;
        const Vector3 position = _particles[index].getPosition() + momentum * drift;
        _particles[index].setVelocity(momentum);
        _particles[index].setPosition(Vector3(
            blitzar_physics_particle_system_host::wrapComovingCoordinate(position.x, boxLength),
            blitzar_physics_particle_system_host::wrapComovingCoordinate(position.y, boxLength),
            blitzar_physics_particle_system_host::wrapComovingCoordinate(position.z, boxLength)));
    }

    std::vector<Vector3> secondAcceleration(_particles.size(), Vector3());
    _cpuTreePmWorkspace.reset();
    if (!computePmField(a1, secondAcceleration)) {
        return false;
    }
    for (std::size_t index = 0; index < _particles.size(); ++index) {
        _particles[index].setVelocity(_particles[index].getVelocity() +
                                      secondAcceleration[index] * firstKick);
        _particles[index].setPressure(secondAcceleration[index] * 100.0f);
    }
    _cosmologyScaleFactor = a1;
    _cosmologyTime += deltaTime;
    _device._hostStateDirty = false;
    if (!_cosmologyMarkerPrinted) {
        fprintf(stderr,
                "[cosmology] mode=comoving backend=cpu_pm assignment=tsc box=%.6g a0=%.6g\n",
                boxLength, a0);
        _cosmologyMarkerPrinted = true;
    }
    return true;
}

bool ParticleSystem::computeHostAccelerations(std::vector<Vector3>& accelerations)
{
    const std::size_t count = _particles.size();
    if (accelerations.size() != count)
        accelerations.assign(count, Vector3());
    const std::ptrdiff_t particleTotal = static_cast<std::ptrdiff_t>(count);
    std::vector<ParticleHotData> hotParticles;
    buildParticleHotData(_particles, hotParticles);
    const ForceLawPolicy forceLaw =
        resolveForceLawPolicy(_octreeTheta, _octreeSoftening, _physicsMinSoftening,
                              _physicsMinDistance2, _physicsMinTheta);

    if (_solverMode == SolverMode::FmmCpu) {
        if (!_fmmWorkspace) {
            _fmmWorkspace = std::make_unique<bltzr_fmm::FmmWorkspace>();
        }
        bltzr_fmm::configure(*_fmmWorkspace, _fmmLeafCapacity, _octreeTheta);
        if (!bltzr_fmm::computeForces(_particles, forceLaw, *_fmmWorkspace, accelerations)) {
            return false;
        }
#pragma omp parallel for schedule(static) if (!_deterministicMode)
        for (std::ptrdiff_t index = 0; index < particleTotal; ++index) {
            accelerations[static_cast<std::size_t>(index)] =
                blitzar_physics_particle_system_host::clampedVector(
                    accelerations[static_cast<std::size_t>(index)], _physicsMaxAcceleration);
        }
        return true;
    }

    const bool cpuFp64Reference =
        _treePmEnabled && _treePmModel == "exact_tree" && _treePmPrecision == "fp64";
    const bool cpuTreePm = _treePmEnabled && _treePmModel != "exact_tree";
    if (cpuFp64Reference) {
        if (!computeCpuFp64PairwiseForces(_particles, forceLaw, accelerations))
            return false;
        if (!_device._treePmMarkerPrinted) {
            fprintf(stderr,
                    "[treepm] enabled solver=cpu_fp64_pairwise model=exact_tree precision=fp64\n");
            _device._treePmMarkerPrinted = true;
        }
    }
    else if (cpuTreePm) {
        CpuTreePmParameters parameters;
        parameters.model = _treePmModel;
        parameters.localGrid = _treePmLocalGrid;
        parameters.gridSize = _treePmGridSize;
        parameters.cutoffFactor = _treePmCutoffFactor;
        parameters.maxLocalNeighbors = _treePmMaxLocalNeighbors;
        parameters.particleLimit = _treePmParticleLimit;
        parameters.precision = _treePmPrecision;
        parameters.assignment = _treePmAssignment;
        bool computed = false;
        if (_treePmPrecision == "fp64") {
            if (!_cpuTreePmFp64Workspace)
                _cpuTreePmFp64Workspace = std::make_unique<CpuTreePmFp64Workspace>();
            computed = computeCpuTreePmForcesFp64(_particles, forceLaw, parameters,
                                                  *_cpuTreePmFp64Workspace, _octree,
                                                  _octreeOpeningCriterion, accelerations);
        }
        else {
            if (!_cpuTreePmWorkspace)
                _cpuTreePmWorkspace = std::make_unique<CpuTreePmWorkspace>();
            computed =
                computeCpuTreePmForces(_particles, forceLaw, parameters, *_cpuTreePmWorkspace,
                                       _octree, _octreeOpeningCriterion, accelerations);
        }
        if (!computed)
            return false;
        if (!_device._treePmMarkerPrinted) {
            fprintf(stderr,
                    "[treepm] enabled solver=cpu_fft_%s model=%s assignment=%s grid=%d "
                    "local_grid=%d neighbors=%d\n",
                    _treePmPrecision.c_str(), _treePmModel.c_str(), _treePmAssignment.c_str(),
                    std::clamp(_treePmGridSize, 32, 128), _treePmLocalGrid ? 1 : 0,
                    std::clamp(_treePmMaxLocalNeighbors, 0, 256));
            _device._treePmMarkerPrinted = true;
        }
    }
    else if (_solverMode == SolverMode::PairwiseCuda && count <= 4096u) {
#pragma omp parallel for schedule(static) if (!_deterministicMode)
        for (std::ptrdiff_t i = 0; i < particleTotal; ++i) {
            const std::size_t particleIndex = static_cast<std::size_t>(i);
            const Vector3 pi = hotParticles[particleIndex].getPosition();
            Vector3 acceleration;
            for (std::size_t j = 0; j < count; ++j) {
                if (particleIndex != j)
                    acceleration += blitzar_physics_particle_system_host::accelerationFromSource(
                        pi, hotParticles[j].getPosition(), hotParticles[j].getMass(), forceLaw);
            }
            accelerations[particleIndex] = acceleration;
        }
    }
    else {
        _octree.build(hotParticles);
#pragma omp parallel for schedule(static) if (!_deterministicMode)
        for (std::ptrdiff_t i = 0; i < particleTotal; ++i) {
            const std::size_t particleIndex = static_cast<std::size_t>(i);
            accelerations[particleIndex] = _octree.computeForceOn(
                hotParticles, particleIndex, forceLaw, _octreeOpeningCriterion);
        }
    }

#pragma omp parallel for schedule(static) if (!_deterministicMode)
    for (std::ptrdiff_t i = 0; i < particleTotal; ++i) {
        accelerations[static_cast<std::size_t>(i)] =
            blitzar_physics_particle_system_host::clampedVector(
                accelerations[static_cast<std::size_t>(i)], _physicsMaxAcceleration);
    }
    return true;
}

bool ParticleSystem::computeHostAccelerationsForIndices(const std::vector<int>& activeIndices,
                                                        std::vector<Vector3>& accelerations)
{
    if (activeIndices.empty()) {
        return true;
    }
    const std::size_t count = _particles.size();
    if (accelerations.size() != count) {
        accelerations.assign(count, Vector3());
    }
    const ForceLawPolicy forceLaw =
        resolveForceLawPolicy(_octreeTheta, _octreeSoftening, _physicsMinSoftening,
                              _physicsMinDistance2, _physicsMinTheta);
    if (_solverMode == SolverMode::FmmCpu) {
        if (!_fmmWorkspace) {
            _fmmWorkspace = std::make_unique<bltzr_fmm::FmmWorkspace>();
        }
        bltzr_fmm::configure(*_fmmWorkspace, _fmmLeafCapacity, _octreeTheta);
        std::vector<Vector3> allAccelerations;
        if (!bltzr_fmm::computeForces(_particles, forceLaw, *_fmmWorkspace, allAccelerations)) {
            return false;
        }
        for (const int activeIndex : activeIndices) {
            accelerations[static_cast<std::size_t>(activeIndex)] =
                blitzar_physics_particle_system_host::clampedVector(
                    allAccelerations[static_cast<std::size_t>(activeIndex)],
                    _physicsMaxAcceleration);
        }
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
        bool computed = false;
        if (_treePmPrecision == "fp64") {
            if (!_cpuTreePmFp64Workspace) {
                _cpuTreePmFp64Workspace = std::make_unique<CpuTreePmFp64Workspace>();
            }
            computed = computeCpuTreePmForcesSelectiveFp64(
                _particles, activeIndices, forceLaw, parameters, *_cpuTreePmFp64Workspace, _octree,
                _octreeOpeningCriterion, accelerations);
        }
        else {
            if (!_cpuTreePmWorkspace) {
                _cpuTreePmWorkspace = std::make_unique<CpuTreePmWorkspace>();
            }
            computed = computeCpuTreePmForcesSelective(_particles, activeIndices, forceLaw,
                                                       parameters, *_cpuTreePmWorkspace, _octree,
                                                       _octreeOpeningCriterion, accelerations);
        }
        return computed;
    }

    if (cpuFp64Reference) {
        const std::ptrdiff_t total = static_cast<std::ptrdiff_t>(activeIndices.size());
#pragma omp parallel for schedule(static) if (!_deterministicMode)
        for (std::ptrdiff_t i = 0; i < total; ++i) {
            const std::size_t target =
                static_cast<std::size_t>(activeIndices[static_cast<std::size_t>(i)]);
            Vector3 acceleration;
            const Vector3 position = _particles[target].getPosition();
            for (std::size_t source = 0u; source < count; ++source) {
                if (source != target) {
                    acceleration += blitzar_physics_particle_system_host::accelerationFromSource(
                        position, _particles[source].getPosition(), _particles[source].getMass(),
                        forceLaw);
                }
            }
            accelerations[target] = blitzar_physics_particle_system_host::clampedVector(
                acceleration, _physicsMaxAcceleration);
        }
        return true;
    }

    std::vector<ParticleHotData> hotParticles;
    buildParticleHotData(_particles, hotParticles);
    const std::ptrdiff_t total = static_cast<std::ptrdiff_t>(activeIndices.size());
    if (_solverMode == SolverMode::PairwiseCuda && count <= 4096u) {
#pragma omp parallel for schedule(static) if (!_deterministicMode)
        for (std::ptrdiff_t i = 0; i < total; ++i) {
            const std::size_t target =
                static_cast<std::size_t>(activeIndices[static_cast<std::size_t>(i)]);
            const Vector3 position = hotParticles[target].getPosition();
            Vector3 acceleration;
            for (std::size_t source = 0u; source < count; ++source) {
                if (source != target) {
                    acceleration += blitzar_physics_particle_system_host::accelerationFromSource(
                        position, hotParticles[source].getPosition(),
                        hotParticles[source].getMass(), forceLaw);
                }
            }
            accelerations[target] = blitzar_physics_particle_system_host::clampedVector(
                acceleration, _physicsMaxAcceleration);
        }
        return true;
    }

#pragma omp parallel for schedule(static) if (!_deterministicMode)
    for (std::ptrdiff_t i = 0; i < total; ++i) {
        const std::size_t target =
            static_cast<std::size_t>(activeIndices[static_cast<std::size_t>(i)]);
        accelerations[target] = blitzar_physics_particle_system_host::clampedVector(
            _octree.computeForceOn(hotParticles, target, forceLaw, _octreeOpeningCriterion),
            _physicsMaxAcceleration);
    }
    return true;
}

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
    if (quantum <= 0.0f)
        return false;

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
    // Rebuild the global PM/tree representation once per outer step. The
    // selective path below reuses it only across the nested micro-ticks.
    if (!computeHostAccelerations(refreshedAccelerations))
        return false;
    if (stateChanged || _adaptiveTimeStepTick == 0u) {
        _adaptiveTimeStepLevels.resize(count);
        _adaptiveTimeStepLastForceTicks.assign(count, 0u);
        _adaptiveTimeStepAccelerations = std::move(refreshedAccelerations);
        _adaptiveTimeStepQuantum = quantum;
        for (std::size_t i = 0; i < count; ++i) {
            _adaptiveTimeStepLevels[i] =
                chooseLevel(_adaptiveTimeStepAccelerations[i], _particles[i].getVelocity());
            _particles[i].setPressure(_adaptiveTimeStepAccelerations[i] * 100.0f);
        }
    }
    else {
        _adaptiveTimeStepAccelerations = std::move(refreshedAccelerations);
    }

    std::vector<Vector3> nextAccelerations = _adaptiveTimeStepAccelerations;
    std::vector<int> activeIndices;
    activeIndices.reserve(count);
    for (std::uint32_t slice = 0u; slice < sliceCount; ++slice) {
        for (std::size_t i = 0; i < count; ++i) {
            const Vector3 velocity = _particles[i].getVelocity();
            const Vector3 acceleration = _adaptiveTimeStepAccelerations[i];
            _particles[i].setPosition(_particles[i].getPosition() + velocity * quantum +
                                      acceleration * (0.5f * quantum * quantum));
            _particles[i].setVelocity(blitzar_physics_particle_system_host::clampedVector(
                velocity + acceleration * quantum, _sphMaxSpeed));
        }

        const std::uint64_t targetTick = _adaptiveTimeStepTick + 1u;
        activeIndices.clear();
        for (std::size_t i = 0u; i < count; ++i) {
            const std::uint32_t cadence = 1u << (levelCount - _adaptiveTimeStepLevels[i]);
            if ((targetTick % cadence) == 0u) {
                activeIndices.push_back(static_cast<int>(i));
            }
        }
        if (!computeHostAccelerationsForIndices(activeIndices, nextAccelerations))
            return false;
        for (const int activeIndex : activeIndices) {
            const std::size_t i = static_cast<std::size_t>(activeIndex);
            const float localDt =
                static_cast<float>(targetTick - _adaptiveTimeStepLastForceTicks[i]) * quantum;
            const Vector3 correction = nextAccelerations[i] - _adaptiveTimeStepAccelerations[i];
            _particles[i].setPosition(_particles[i].getPosition() +
                                      correction * (0.5f * localDt * localDt));
            _particles[i].setVelocity(blitzar_physics_particle_system_host::clampedVector(
                _particles[i].getVelocity() + correction * localDt, _sphMaxSpeed));
            _adaptiveTimeStepAccelerations[i] = nextAccelerations[i];
            _adaptiveTimeStepLastForceTicks[i] = targetTick;
            _adaptiveTimeStepLevels[i] =
                chooseLevel(nextAccelerations[i], _particles[i].getVelocity());
        }
        for (std::size_t i = 0u; i < count; ++i) {
            _particles[i].setPressure(_adaptiveTimeStepAccelerations[i] * 100.0f);
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
    _device._hostStateDirty = false;
    return true;
}

void ParticleSystem::setUseOctree(bool enabled)
{
    _solverMode = enabled ? SolverMode::OctreeCpu : SolverMode::PairwiseCuda;
}

bool ParticleSystem::usesOctree() const
{
    return _solverMode != SolverMode::PairwiseCuda;
}

void ParticleSystem::setOctreeTheta(float theta)
{
    _octreeTheta = std::max(theta, _physicsMinTheta);
}

void ParticleSystem::setOctreeOpeningCriterion(OctreeOpeningCriterion criterion)
{
    _octreeOpeningCriterion = criterion;
}

void ParticleSystem::setOctreeSoftening(float softening)
{
    _octreeSoftening = std::max(softening, _physicsMinSoftening);
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
    const bool changed = _adaptiveTimeStepsEnabled != enabled ||
                         _adaptiveTimeStepMaxLevel != std::min<std::uint32_t>(maxLevel, 12u) ||
                         std::abs(_adaptiveTimeStepEta - eta) > 1e-6f;
    _adaptiveTimeStepsEnabled = enabled;
    _adaptiveTimeStepMaxLevel = std::min<std::uint32_t>(maxLevel, 12u);
    _adaptiveTimeStepEta = std::clamp(eta, 0.01f, 1.0f);
    if (changed || !_adaptiveTimeStepsEnabled) {
        _adaptiveTimeStepTick = 0u;
        _adaptiveTimeStepQuantum = 0.0f;
        _adaptiveTimeStepMarkerPrinted = false;
        _adaptiveTimeStepLevels.clear();
        _adaptiveTimeStepLastForceTicks.clear();
        _adaptiveTimeStepAccelerations.clear();
    }
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
}

void ParticleSystem::setCudaCachePreference(const std::string& preference)
{
    if (preference == "default" || preference == "l1" || preference == "shared") {
        _cudaCachePreference = preference;
    }
}

bool ParticleSystem::reconfigureRuntimeBuffers()
{
    return true;
}

void ParticleSystem::setSphEnabled(bool enabled)
{
    _sphEnabled = enabled;
}

bool ParticleSystem::isSphEnabled() const
{
    return _sphEnabled;
}

void ParticleSystem::setSphParameters(float smoothingLength, float restDensity, float gasConstant,
                                      float viscosity)
{
    _sphSmoothingLength = std::max(0.0f, smoothingLength);
    _sphRestDensity = std::max(0.0f, restDensity);
    _sphGasConstant = std::max(0.0f, gasConstant);
    _sphViscosity = std::max(0.0f, viscosity);
}

void ParticleSystem::setPhysicsStabilityConstants(float maxAcceleration, float minSoftening,
                                                  float minDistance2, float minTheta)
{
    _physicsMaxAcceleration = std::max(0.0f, maxAcceleration);
    _physicsMinSoftening = std::max(0.0f, minSoftening);
    _physicsMinDistance2 = std::max(0.0f, minDistance2);
    _physicsMinTheta = std::max(0.0f, minTheta);
    setOctreeSoftening(_octreeSoftening);
    setOctreeTheta(_octreeTheta);
}

void ParticleSystem::setSphCaps(float maxAcceleration, float maxSpeed)
{
    _sphMaxAcceleration = std::max(0.0f, maxAcceleration);
    _sphMaxSpeed = std::max(0.0f, maxSpeed);
}

void ParticleSystem::setThermalParameters(float ambientTemperature, float specificHeat,
                                          float heatingCoeff, float radiationCoeff)
{
    _thermalAmbientTemperature = ambientTemperature;
    _thermalSpecificHeat = std::max(1e-6f, specificHeat);
    _thermalHeatingCoeff = std::max(0.0f, heatingCoeff);
    _thermalRadiationCoeff = std::max(0.0f, radiationCoeff);
}

void ParticleSystem::setCosmologyParameters(const CosmologyConfig& config)
{
    _cosmology = config;
    _cosmology.enabled = config.enabled;
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
    previousHubble =
        blitzar_physics_particle_system_host::cosmologyHubbleRate(_cosmology, previousScale);
    const float midpointScale =
        std::max(previousScale + 0.5f * previousScale * previousHubble * deltaTime, 1.0e-6f);
    const float midpointHubble =
        blitzar_physics_particle_system_host::cosmologyHubbleRate(_cosmology, midpointScale);
    const float nextScale =
        std::max(previousScale + midpointScale * midpointHubble * deltaTime, previousScale);
    _cosmologyScaleFactor = nextScale;
    _cosmologyTime += deltaTime;
    nextHubble = blitzar_physics_particle_system_host::cosmologyHubbleRate(_cosmology, nextScale);
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

float ParticleSystem::getCumulativeRadiatedEnergy() const
{
    return _cumulativeRadiatedEnergy;
}

float ParticleSystem::getThermalSpecificHeat() const
{
    return _thermalSpecificHeat;
}

void ParticleSystem::setSolverMode(SolverMode mode)
{
    _solverMode = mode == SolverMode::OctreeGpu ? SolverMode::OctreeCpu : mode;
}

ParticleSystem::SolverMode ParticleSystem::getSolverMode() const
{
    return _solverMode;
}

void ParticleSystem::setIntegratorMode(IntegratorMode mode)
{
    _integratorMode = mode;
}

ParticleSystem::IntegratorMode ParticleSystem::getIntegratorMode() const
{
    return _integratorMode;
}

void ParticleSystem::syncDeviceState()
{
    _device._hostStateDirty = false;
}

bool ParticleSystem::syncHostState()
{
    return true;
}

bool ParticleSystem::computeEnergyEstimateGpu(std::size_t, float, float, float, float&, float&,
                                              float&, bool&)
{
    return false;
}

const std::vector<Particle>& ParticleSystem::getParticles() const
{
    return _particles;
}

bool ParticleSystem::setParticles(std::vector<Particle> particles)
{
    if (particles.empty())
        return false;
    _particles = std::move(particles);
    _adaptiveTimeStepTick = 0u;
    _adaptiveTimeStepQuantum = 0.0f;
    _adaptiveTimeStepLevels.clear();
    _adaptiveTimeStepLastForceTicks.clear();
    _adaptiveTimeStepAccelerations.clear();
    _device._deviceParticleCapacity = _particles.size();
    _device._hostStateDirty = false;
    return true;
}

ParticleSoAView ParticleSystem::getSoAView(bool) const
{
    return ParticleSoAView{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                           nullptr, nullptr, nullptr, nullptr, nullptr, 0,       {0, 0, 0}};
}

const GpuSystemMetrics* ParticleSystem::getMappedGpuMetrics() const
{
    return nullptr;
}

void ParticleSystem::initializeRuntimeState(std::size_t particleCapacity, bool enableCudaRuntime)
{
    (void)enableCudaRuntime;
    _solverMode = SolverMode::PairwiseCuda;
    _integratorMode = IntegratorMode::Euler;
    _octreeTheta = 0.6f;
    _octreeOpeningCriterion = OctreeOpeningCriterion::CenterOfMass;
    _octreeSoftening = 0.01f;
    _sphEnabled = false;
    _sphSmoothingLength = 1.0f;
    _sphRestDensity = 1.0f;
    _sphGasConstant = 1.0f;
    _sphViscosity = 0.0f;
    _physicsMaxAcceleration = 1000.0f;
    _physicsMinSoftening = 1e-4f;
    _physicsMinDistance2 = 1e-8f;
    _physicsMinTheta = 0.1f;
    _sphMaxAcceleration = 1000.0f;
    _sphMaxSpeed = 1000.0f;
    _thermalAmbientTemperature = 0.0f;
    _thermalSpecificHeat = 1.0f;
    _thermalHeatingCoeff = 0.0f;
    _thermalRadiationCoeff = 0.0f;
    _cumulativeRadiatedEnergy = 0.0f;
    _device = ParticleSystemDeviceState{};
    _device._deviceParticleCapacity = particleCapacity;
}

void ParticleSystem::buildBootstrapState(int particleCount)
{
    const int count = std::max(0, particleCount);
    _particles.clear();
    _particles.reserve(static_cast<std::size_t>(count));
    if (count == 0)
        return;
    for (int i = 0; i < count; ++i) {
        const float fraction = static_cast<float>(i) / static_cast<float>(std::max(1, count));
        const float angle = fraction * 2.0f * kPi;
        const float radius = 0.25f + 0.75f * fraction;
        const Vector3 position(std::cos(angle) * radius, std::sin(angle) * radius, 0.0f);
        const Vector3 velocity(-std::sin(angle) * 0.05f, std::cos(angle) * 0.05f, 0.0f);
        _particles.push_back(
            blitzar_physics_particle_system_host::makeParticle(position, velocity));
    }
}

bool ParticleSystem::allocateParticleBuffers(std::size_t particleCapacity)
{
    _device._deviceParticleCapacity = particleCapacity;
    return true;
}

bool ParticleSystem::seedDeviceState()
{
    return true;
}

void ParticleSystem::releaseParticleBuffers()
{
    releaseMappedMetrics();
}

float ParticleSystem::applyThermalModel(float deltaTime)
{
    if (_thermalRadiationCoeff <= 0.0f || _particles.empty())
        return 0.0f;
    float radiated = 0.0f;
    for (Particle& particle : _particles) {
        const float temperature = particle.getTemperature();
        const float excess = std::max(0.0f, temperature - _thermalAmbientTemperature);
        const float loss = std::min(excess, excess * _thermalRadiationCoeff * deltaTime);
        particle.setTemperature(temperature - loss);
        radiated += loss * particle.getMass() * _thermalSpecificHeat;
    }
    return radiated;
}

bool ParticleSystem::buildSphGrid(int)
{
    return !_sphEnabled || !_particles.empty();
}

void ParticleSystem::releaseRk4Buffers()
{
}

void ParticleSystem::releaseSphBuffers()
{
}

void ParticleSystem::releaseSphGridBuffers()
{
}

bool ParticleSystem::allocateRk4Buffers(int)
{
    return true;
}

bool ParticleSystem::allocateSphBuffers(int)
{
    return true;
}

bool ParticleSystem::allocateSphGridBuffers(int)
{
    return true;
}

bool ParticleSystem::ensureLinearOctreeScratchCapacity(int numParticles)
{
    _device._linearOctreeLeafCapacity = std::max(0, numParticles);
    return true;
}

bool ParticleSystem::ensureEnergyScratchCapacity(int, int)
{
    return true;
}

bool ParticleSystem::buildLinearOctreeGpu(ParticleSoAView, int)
{
    return false;
}

bool ParticleSystem::allocateMappedMetrics()
{
    return false;
}

void ParticleSystem::releaseMappedMetrics()
{
    _device._mappedMetricsHost = nullptr;
    _device._mappedMetricsDevice = 0u;
}

void ParticleSystem::publishMappedMetrics(float deltaTime)
{
    _device._metricsStepId += 1u;
    _device._metricsSimTime += deltaTime;
}

std::size_t
ParticleSystem::estimateMemoryUsage(std::size_t particleCount, bool sphEnabled, SolverMode,
                                    IntegratorMode, std::size_t energySampleLimit,
                                    int octreeLeafCapacity, std::size_t* baseAndIntegratorBytes,
                                    std::size_t* sphBytes, std::size_t* octreeBytes) const
{
    const std::size_t base = particleCount * sizeof(Particle);
    const std::size_t sph =
        sphEnabled ? particleCount * (2u * sizeof(float) + 2u * sizeof(int)) : 0u;
    const std::size_t octree =
        static_cast<std::size_t>(std::max(0, octreeLeafCapacity)) * sizeof(GpuOctreeNode);
    const std::size_t energy = std::max<std::size_t>(1u, energySampleLimit) * sizeof(double);
    if (baseAndIntegratorBytes)
        *baseAndIntegratorBytes = base + energy;
    if (sphBytes)
        *sphBytes = sph;
    if (octreeBytes)
        *octreeBytes = octree;
    return base + sph + octree + energy;
}

std::string ParticleSystem::formatMemoryBreakdown(std::size_t baseAndIntegratorBytes,
                                                  std::size_t sphBytes, std::size_t octreeBytes,
                                                  std::size_t totalBytes, std::size_t budgetBytes)
{
    std::ostringstream stream;
    stream << "base=" << baseAndIntegratorBytes << " sph=" << sphBytes << " octree=" << octreeBytes
           << " total=" << totalBytes << " budget=" << budgetBytes;
    return stream.str();
}
