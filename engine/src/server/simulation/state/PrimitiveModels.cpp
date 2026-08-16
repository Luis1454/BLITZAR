/*
 * @file engine/src/server/simulation/state/PrimitiveModels.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Primitive and compact initial-state distributions.
 */

#include "GenerationContext.hpp"
#include "Constants.hpp"
#include "engine/src/server/simulation/state/InitializationHelper.hpp"

#include <algorithm>
#include <cmath>
#include <omp.h>
#include <random>

static Particle makeParticle(float mass, const Vector3& position, const Vector3& velocity)
{
    Particle particle;
    particle.setMass(mass);
    particle.setPosition(position);
    particle.setVelocity(velocity);
    return particle;
}

static bool buildTwoBody(GenerationContext& context)
{
    const float separation = std::max(0.2f, context.config.cloudHalfExtent);
    const float mass = std::max(1e-6f, context.config.particleMass);
    const float radius = 0.5f * separation;
    const float orbitalSpeed =
        std::sqrt(mass / std::max(2.0f * separation, 1e-6f)) *
        std::max(0.0f, context.config.velocityScale);

    Particle left = makeParticle(
        mass, context.centralPosition + Vector3(-radius, 0.0f, 0.0f),
        context.centralVelocity + Vector3(0.0f, -orbitalSpeed, 0.0f));
    context.finalizeParticle(left);
    context.particles.push_back(left);

    Particle right = makeParticle(
        mass, context.centralPosition + Vector3(radius, 0.0f, 0.0f),
        context.centralVelocity + Vector3(0.0f, orbitalSpeed, 0.0f));
    context.finalizeParticle(right);
    context.particles.push_back(right);
    return true;
}

static bool buildThreeBody(GenerationContext& context)
{
    const float scale = std::max(0.1f, context.config.cloudHalfExtent);
    const float mass = std::max(1e-6f, context.config.particleMass);
    const float speedScale = std::max(0.0f, context.config.velocityScale) / std::sqrt(scale);
    constexpr float kX = 0.97000436f;
    constexpr float kY = 0.24308753f;
    constexpr float kVx = 0.46620368f;
    constexpr float kVy = 0.43236572f;
    const Vector3 positions[] = {Vector3(-kX * scale, kY * scale, 0.0f),
                                 Vector3(kX * scale, -kY * scale, 0.0f),
                                 Vector3(0.0f, 0.0f, 0.0f)};
    const Vector3 velocities[] = {
        Vector3(kVx * speedScale, kVy * speedScale, 0.0f),
        Vector3(kVx * speedScale, kVy * speedScale, 0.0f),
        Vector3(-2.0f * kVx * speedScale, -2.0f * kVy * speedScale, 0.0f)};

    for (int index = 0; index < 3; ++index) {
        Particle particle = makeParticle(mass, context.centralPosition + positions[index],
                                          context.centralVelocity + velocities[index]);
        context.finalizeParticle(particle);
        context.particles.push_back(particle);
    }
    return true;
}

static bool buildRandomCloud(GenerationContext& context)
{
    const float halfExtent = std::max(0.01f, context.config.cloudHalfExtent);
    const float cloudSpeed = std::max(0.0f, context.config.cloudSpeed);
    const float particleMass = std::max(1e-6f, context.config.particleMass);
    std::uniform_real_distribution<float> positionDistribution(-halfExtent, halfExtent);
    std::uniform_real_distribution<float> velocityDistribution(-cloudSpeed, cloudSpeed);
    context.addCentralBody();

    RandomData values;
    values.reserve(context.count, 6);
    while (values.particleCount() < context.count) {
        values.push(positionDistribution(context.rng));
        values.push(positionDistribution(context.rng));
        values.push(positionDistribution(context.rng));
        values.push(velocityDistribution(context.rng));
        values.push(velocityDistribution(context.rng));
        values.push(velocityDistribution(context.rng));
    }

    context.particles.reserve(values.particleCount());
#pragma omp parallel if (!context.config.deterministicMode && context.velocityTemperature <= 0.0f)
    {
        std::vector<Particle> localParticles;
        const std::ptrdiff_t total = static_cast<std::ptrdiff_t>(values.particleCount());
#pragma omp for nowait
        for (std::ptrdiff_t index = 0; index < total; ++index) {
            const std::size_t particleIndex = static_cast<std::size_t>(index);
            Particle particle = makeParticle(
                particleMass,
                context.centralPosition +
                    Vector3(values.get(particleIndex, 0), values.get(particleIndex, 1),
                            values.get(particleIndex, 2)),
                context.centralVelocity +
                    Vector3(values.get(particleIndex, 3), values.get(particleIndex, 4),
                            values.get(particleIndex, 5)));
            context.finalizeParticle(particle);
            localParticles.push_back(particle);
        }
#pragma omp critical
        context.particles.insert(context.particles.end(), localParticles.begin(), localParticles.end());
    }
    return context.particles.size() >= 2;
}

static bool buildCube(GenerationContext& context)
{
    const float halfExtent = std::max(0.01f, context.config.cubeHalfExtent);
    const float cloudSpeed = std::max(0.0f, context.config.cloudSpeed);
    const float particleMass = std::max(1e-6f, context.config.particleMass);
    std::uniform_real_distribution<float> positionDistribution(-halfExtent, halfExtent);
    std::uniform_real_distribution<float> velocityDistribution(-cloudSpeed, cloudSpeed);
    context.addCentralBody();
    while (context.particles.size() < context.count) {
        Particle particle = makeParticle(
            particleMass,
            context.centralPosition +
                Vector3(positionDistribution(context.rng), positionDistribution(context.rng),
                        positionDistribution(context.rng)),
            context.centralVelocity +
                Vector3(velocityDistribution(context.rng), velocityDistribution(context.rng),
                        velocityDistribution(context.rng)));
        context.finalizeParticle(particle);
        context.particles.push_back(particle);
    }
    return context.particles.size() >= 2;
}

static bool buildSphere(GenerationContext& context)
{
    const float radius = std::max(0.01f, context.config.sphereRadius);
    const float cloudSpeed = std::max(0.0f, context.config.cloudSpeed);
    const float particleMass = std::max(1e-6f, context.config.particleMass);
    const float radiusSquared = radius * radius;
    std::uniform_real_distribution<float> positionDistribution(-radius, radius);
    std::uniform_real_distribution<float> velocityDistribution(-cloudSpeed, cloudSpeed);
    context.addCentralBody();
    while (context.particles.size() < context.count) {
        const float x = positionDistribution(context.rng);
        const float y = positionDistribution(context.rng);
        const float z = positionDistribution(context.rng);
        if (x * x + y * y + z * z > radiusSquared) {
            continue;
        }
        Particle particle = makeParticle(
            particleMass, context.centralPosition + Vector3(x, y, z),
            context.centralVelocity +
                Vector3(velocityDistribution(context.rng), velocityDistribution(context.rng),
                        velocityDistribution(context.rng)));
        context.finalizeParticle(particle);
        context.particles.push_back(particle);
    }
    return context.particles.size() >= 2;
}

static bool buildSolarSystem(GenerationContext& context)
{
    struct Planet {
        float radius;
        float mass;
    };
    const Planet planets[] = {{0.39f, 1.6e-7f}, {0.72f, 2.4e-6f}, {1.00f, 3.0e-6f},
                              {1.52f, 3.2e-7f}, {5.20f, 9.5e-4f}, {9.54f, 2.8e-4f},
                              {19.2f, 4.3e-5f}, {30.1f, 5.1e-5f}};
    std::uniform_real_distribution<float> angleDistribution(0.0f, kTwoPi);
    context.addCentralBody();
    for (const Planet& planet : planets) {
        const float angle = angleDistribution(context.rng);
        const float speed = std::sqrt(context.centralMass / planet.radius) *
                            std::max(0.0f, context.config.velocityScale);
        Particle particle = makeParticle(
            planet.mass,
            context.centralPosition +
                Vector3(planet.radius * std::cos(angle), planet.radius * std::sin(angle), 0.0f),
            context.centralVelocity +
                Vector3(-std::sin(angle) * speed, std::cos(angle) * speed, 0.0f));
        context.finalizeParticle(particle);
        context.particles.push_back(particle);
    }
    return context.particles.size() >= 2;
}

static bool buildSphSphere(GenerationContext& context)
{
    const float radius = std::max(0.1f, context.config.cloudHalfExtent);
    const float mass = std::max(1e-6f, context.config.particleMass);
    const float radiusSquared = radius * radius;
    while (context.particles.size() < context.count) {
        std::uniform_real_distribution<float> distribution(-radius, radius);
        const float x = distribution(context.rng);
        const float y = distribution(context.rng);
        const float z = distribution(context.rng);
        if (x * x + y * y + z * z > radiusSquared) {
            continue;
        }
        Particle particle = makeParticle(mass, context.centralPosition + Vector3(x, y, z),
                                          context.centralVelocity);
        context.finalizeParticle(particle);
        context.particles.push_back(particle);
    }
    return context.particles.size() >= 2;
}

bool buildPrimitiveModel(GenerationContext& context, std::string_view mode)
{
    if (mode == "two_body" || mode == "binary_star") {
        return buildTwoBody(context);
    }
    if (mode == "three_body") {
        return buildThreeBody(context);
    }
    if (mode == "random_cloud") {
        return buildRandomCloud(context);
    }
    if (mode == "cube_random") {
        return buildCube(context);
    }
    if (mode == "sphere_random") {
        return buildSphere(context);
    }
    if (mode == "solar_system") {
        return buildSolarSystem(context);
    }
    if (mode == "sph_collapse" || mode == "sph_sphere") {
        return buildSphSphere(context);
    }
    return false;
}
