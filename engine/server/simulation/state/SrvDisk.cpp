/*
 * @file engine/server/simulation/state/SrvDisk.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Disk-orbit initial-state generation.
 */

#include "SrvGenerationContext.hpp"
#include "FndConstants.hpp"
#include "simulation/state/SrvInitializationHelper.hpp"

#include <algorithm>
#include <cmath>
#include <omp.h>
#include <random>

bool buildDiskModel(GenerationContext& context)
{
    constexpr float kSolverMaxAcceleration = kPhysicsMaxAccelerationDefault;
    const float radiusMin =
        std::max(0.01f, std::min(context.config.diskRadiusMin, context.config.diskRadiusMax));
    const float radiusMax = std::max(
        radiusMin + 1e-4f, std::max(context.config.diskRadiusMin, context.config.diskRadiusMax));
    const float radiusMinSquared = radiusMin * radiusMin;
    const float radiusMaxSquared = radiusMax * radiusMax;
    const float radiusRangeSquared = std::max(1e-6f, radiusMaxSquared - radiusMinSquared);
    const float diskThickness = std::max(0.0f, context.config.diskThickness);
    const float velocityScale = std::max(0.0f, context.config.velocityScale);
    const float effectiveCentralMass =
        context.config.includeCentralBody ? context.centralMass : 0.0f;
    const float effectiveDiskMass = std::max(0.0f, context.config.diskMass);
    std::uniform_real_distribution<float> radiusDistribution(radiusMin, radiusMax);
    std::uniform_real_distribution<float> angleDistribution(0.0f, kTwoPi);
    std::uniform_real_distribution<float> heightDistribution(-diskThickness, diskThickness);
    context.addCentralBody();
    const std::uint32_t diskCount = std::max<std::uint32_t>(
        1u, context.count - static_cast<std::uint32_t>(context.particles.size()));
    const float particleMass =
        std::max(1e-6f, context.config.diskMass / static_cast<float>(diskCount));

    RandomData data;
    data.reserve(diskCount, 3);
    for (std::uint32_t index = 0; index < diskCount; ++index) {
        data.push(radiusDistribution(context.rng));
        data.push(angleDistribution(context.rng));
        data.push(heightDistribution(context.rng));
    }

    context.particles.reserve(context.particles.size() + diskCount);
#pragma omp parallel if (!context.config.deterministicMode && context.velocityTemperature <= 0.0f)
    {
        std::vector<Particle> localParticles;
        const std::ptrdiff_t total = static_cast<std::ptrdiff_t>(data.particleCount());
#pragma omp for nowait
        for (std::ptrdiff_t index = 0; index < total; ++index) {
            const std::size_t particleIndex = static_cast<std::size_t>(index);
            const float radius = data.get(particleIndex, 0);
            const float angle = data.get(particleIndex, 1);
            const float height = data.get(particleIndex, 2);
            const Vector3 radial(radius * std::cos(angle), radius * std::sin(angle), height);
            const float enclosedFraction = std::clamp(
                (radius * radius - radiusMinSquared) / radiusRangeSquared, 0.0f, 1.0f);
            const float enclosedMass = std::max(
                1e-6f, effectiveCentralMass + effectiveDiskMass * enclosedFraction);
            const float acceleration = enclosedMass / std::max(radius * radius, 1e-6f);
            const float cappedAcceleration = std::min(acceleration, kSolverMaxAcceleration);
            const float orbitalSpeed =
                std::sqrt(cappedAcceleration * std::max(radius, 1e-4f)) * velocityScale;
            Particle particle;
            particle.setMass(particleMass);
            particle.setPosition(context.centralPosition + radial);
            particle.setVelocity(context.centralVelocity +
                                 Vector3(-std::sin(angle) * orbitalSpeed,
                                         std::cos(angle) * orbitalSpeed, 0.0f));
            context.finalizeParticle(particle);
            localParticles.push_back(particle);
        }
#pragma omp critical
        context.particles.insert(context.particles.end(), localParticles.begin(), localParticles.end());
    }
    return context.particles.size() >= 2;
}
