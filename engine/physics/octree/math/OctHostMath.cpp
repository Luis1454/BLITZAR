/*
 * @file engine/physics/octree/math/OctHostMath.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Shared host-side math helpers for ParticleSystem strategies.
 */

#include "physics/octree/math/OctHostMath.hpp"

#include "core/constants/FndConstants.hpp"

#include <algorithm>
#include <cmath>

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
    const auto integrand = [&config](float scaleFactor) {
        const float safeScaleFactor = std::max(scaleFactor, 1.0e-6f);
        return 1.0f / (safeScaleFactor * safeScaleFactor * safeScaleFactor *
                       std::max(cosmologyHubbleRate(config, safeScaleFactor), 1.0e-12f));
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
    if (limit <= 0.0f || speed <= limit || speed <= 1e-6f) {
        return value;
    }
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
