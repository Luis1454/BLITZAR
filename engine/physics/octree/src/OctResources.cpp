/*
 * @file engine/physics/octree/src/OctResources.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Host-side resource and memory lifecycle for ParticleSystem.
 */

#include "OctHostMath.hpp"
#include "ParticleSystemDeviceState.hpp"
#include "ParticleSystem.hpp"

#include "Constants.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

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
    _device = std::make_unique<ParticleSystemDeviceState>();
    _device->_deviceParticleCapacity = particleCapacity;
}

void ParticleSystem::buildBootstrapState(int particleCount)
{
    const int count = std::max(0, particleCount);
    _particles.clear();
    _particles.reserve(static_cast<std::size_t>(count));
    if (count == 0) {
        return;
    }
    for (int index = 0; index < count; ++index) {
        const float fraction = static_cast<float>(index) / static_cast<float>(std::max(1, count));
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
    _device->_deviceParticleCapacity = particleCapacity;
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
    if (_thermalRadiationCoeff <= 0.0f || _particles.empty()) {
        return 0.0f;
    }
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
    _device->_linearOctreeLeafCapacity = std::max(0, numParticles);
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
    _device->_mappedMetricsHost = nullptr;
    _device->_mappedMetricsDevice = 0u;
}

void ParticleSystem::publishMappedMetrics(float deltaTime)
{
    _device->_metricsStepId += 1u;
    _device->_metricsSimTime += deltaTime;
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
    if (baseAndIntegratorBytes) {
        *baseAndIntegratorBytes = base + energy;
    }
    if (sphBytes) {
        *sphBytes = sph;
    }
    if (octreeBytes) {
        *octreeBytes = octree;
    }
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
