/*
 * @file engine/physics/octree/OctConfiguration.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime configuration and host-state accessors for ParticleSystem.
 */

#include "OctHostMath.hpp"
#include "OctParticleSystemDeviceState.hpp"
#include "PhyParticleSystem.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

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
        particle.setPosition(particle.getPosition() * scaleRatio);
        particle.setVelocity(particle.getVelocity() * inverseScaleRatio);
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
    _device->_hostStateDirty = false;
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
    if (particles.empty()) {
        return false;
    }
    _particles = std::move(particles);
    _adaptiveTimeStepTick = 0u;
    _adaptiveTimeStepQuantum = 0.0f;
    _adaptiveTimeStepLevels.clear();
    _adaptiveTimeStepLastForceTicks.clear();
    _adaptiveTimeStepAccelerations.clear();
    _device->_deviceParticleCapacity = _particles.size();
    _device->_hostStateDirty = false;
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
