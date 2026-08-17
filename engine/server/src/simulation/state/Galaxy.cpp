/*
 * @file engine/server/src/simulation/state/Galaxy.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Isolated and colliding disk galaxy generation.
 */

#include "GenerationContext.hpp"
#include "Constants.hpp"

#include <algorithm>
#include <cmath>
#include <random>

static void generateDisk(GenerationContext& context, std::uint32_t count, Vector3 center,
                         Vector3 bulkVelocity, float nominalMass, std::uint32_t seedOffset,
                         float radiusMin, float radiusMax, float radiusMinSquared,
                         float radiusRangeSquared, float requestedParticleMass)
{
    if (count == 0u) {
        return;
    }
    std::mt19937 diskRng(context.config.seed + seedOffset);
    const float massPerParticle = std::min(
        requestedParticleMass, std::max(1e-8f, nominalMass / static_cast<float>(count)));
    const float actualDiskMass = massPerParticle * static_cast<float>(count);
    const float softening = std::max(0.05f, radiusMin * 0.5f);
    const std::size_t firstParticle = context.particles.size();
    std::uniform_real_distribution<float> radiusDistribution(radiusMin, radiusMax);
    std::uniform_real_distribution<float> angleDistribution(0.0f, kTwoPi);
    for (std::uint32_t index = 0u; index < count; ++index) {
        const float radius = radiusDistribution(diskRng);
        const float angle = angleDistribution(diskRng);
        const Vector3 position =
            center + Vector3(radius * std::cos(angle), radius * std::sin(angle), 0.0f);
        const float fraction = std::max(
            0.05f, std::clamp((radius * radius - radiusMinSquared) / radiusRangeSquared,
                              0.0f, 1.0f));
        const float enclosedMass = (context.config.includeCentralBody ? context.centralMass : 0.0f) +
                                   actualDiskMass * fraction;
        const float softenedRadiusSquared = radius * radius + softening * softening;
        const float acceleration =
            enclosedMass * radius / std::pow(softenedRadiusSquared, 1.5f);
        const float speed = std::sqrt(std::max(0.0f, acceleration * radius)) *
                            std::max(0.0f, context.config.velocityScale);
        Particle particle;
        particle.setMass(massPerParticle);
        particle.setPosition(position);
        particle.setVelocity(bulkVelocity + Vector3(-std::sin(angle) * speed,
                                                     std::cos(angle) * speed, 0.0f));
        context.finalizeParticle(particle);
        context.particles.push_back(particle);
    }

    Vector3 meanPosition(0.0f, 0.0f, 0.0f);
    Vector3 meanVelocity(0.0f, 0.0f, 0.0f);
    const float inverseCount = 1.0f / static_cast<float>(count);
    for (std::size_t index = firstParticle; index < context.particles.size(); ++index) {
        meanPosition += context.particles[index].getPosition();
        meanVelocity += context.particles[index].getVelocity();
    }
    meanPosition = meanPosition * inverseCount;
    meanVelocity = meanVelocity * inverseCount;
    for (std::size_t index = firstParticle; index < context.particles.size(); ++index) {
        Particle& particle = context.particles[index];
        particle.setPosition(particle.getPosition() - meanPosition + center);
        particle.setVelocity(particle.getVelocity() - meanVelocity + bulkVelocity);
    }
}

bool buildGalaxyModel(GenerationContext& context, std::string_view mode)
{
    const float size = std::max(0.1f, context.config.cloudHalfExtent);
    const float radiusLimit = std::max(0.1f, size);
    const float configuredRadiusMax =
        std::max(0.01f, std::min(context.config.diskRadiusMax, radiusLimit));
    const float radiusMin =
        std::max(0.01f, std::min(context.config.diskRadiusMin, configuredRadiusMax));
    const float radiusMax = std::max(radiusMin + 0.0001f, configuredRadiusMax);
    const float radiusMinSquared = radiusMin * radiusMin;
    const float radiusMaxSquared = radiusMax * radiusMax;
    const float radiusRangeSquared = std::max(1e-6f, radiusMaxSquared - radiusMinSquared);
    const float requestedParticleMass = std::max(1e-8f, context.config.particleMass);

    if (mode == "galaxy") {
        context.addCentralBody();
        generateDisk(context, context.count, context.centralPosition, context.centralVelocity,
                     context.config.diskMass, 0u, radiusMin, radiusMax, radiusMinSquared,
                     radiusRangeSquared, requestedParticleMass);
        return context.particles.size() >= 2;
    }

    const float galaxySeparation = size * 1.5f;
    const std::uint32_t halfCount = context.count / 2u;
    const std::uint32_t remainder = context.count % 2u;
    const float leftMass = requestedParticleMass * static_cast<float>(std::max(1u, halfCount));
    const float rightMass = requestedParticleMass * static_cast<float>(halfCount + remainder);
    const float galaxyMass = std::min(context.config.diskMass * 0.5f,
                                      std::min(leftMass, rightMass));
    const float orbitalSpeed =
        std::sqrt(galaxyMass / std::max(4.0f * galaxySeparation, 0.1f)) * 1.05f;
    generateDisk(context, halfCount,
                 context.centralPosition + Vector3(-galaxySeparation, 0.0f, 0.0f),
                 context.centralVelocity + Vector3(0.0f, orbitalSpeed, 0.0f),
                 context.config.diskMass * 0.5f, 0u, radiusMin, radiusMax, radiusMinSquared,
                 radiusRangeSquared, requestedParticleMass);
    generateDisk(context, halfCount + remainder,
                 context.centralPosition + Vector3(galaxySeparation, 0.0f, 0.0f),
                 context.centralVelocity + Vector3(0.0f, -orbitalSpeed, 0.0f),
                 context.config.diskMass * 0.5f, 1000u, radiusMin, radiusMax, radiusMinSquared,
                 radiusRangeSquared, requestedParticleMass);
    return context.particles.size() >= 2;
}
