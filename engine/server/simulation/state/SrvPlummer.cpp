/*
 * @file engine/server/simulation/state/SrvPlummer.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Plummer sphere initial-state generation.
 */

#include "SrvGenerationContext.hpp"
#include "FndConstants.hpp"
#include "simulation/state/SrvInitializationHelper.hpp"

#include <algorithm>
#include <cmath>
#include <omp.h>
#include <random>

bool buildPlummerModel(GenerationContext& context)
{
    const float scale = std::max(0.1f, context.config.cloudHalfExtent);
    const float totalMass = std::max(
        1e-6f, context.config.particleMass * static_cast<float>(context.count));
    const float mass = std::max(1e-6f, totalMass / static_cast<float>(context.count));
    const float sigma = std::sqrt(totalMass / std::max(6.0f * scale, 1e-6f)) *
                        std::max(0.0f, context.config.velocityScale);
    std::uniform_real_distribution<float> unitDistribution(1e-4f, 0.9999f);
    std::uniform_real_distribution<float> azimuthDistribution(0.0f, kTwoPi);
    std::uniform_real_distribution<float> cosThetaDistribution(-1.0f, 1.0f);
    std::normal_distribution<float> velocityDistribution(0.0f, sigma);

    RandomData data;
    constexpr float kAcceptanceFactor = 1.2f;
    const std::size_t targetGenerated =
        static_cast<std::size_t>(static_cast<float>(context.count) * kAcceptanceFactor);
    data.reserve(targetGenerated, 9);
    while (data.particleCount() < targetGenerated) {
        const float unit = unitDistribution(context.rng);
        const float azimuth = azimuthDistribution(context.rng);
        const float cosTheta = cosThetaDistribution(context.rng);
        data.push(unit);
        data.push(azimuth);
        data.push(cosTheta);
        data.push(velocityDistribution(context.rng));
        data.push(velocityDistribution(context.rng));
        data.push(velocityDistribution(context.rng));
        const float sinTheta = std::sqrt(std::max(0.0f, 1.0f - cosTheta * cosTheta));
        const float radius = scale / std::sqrt(std::pow(unit, -2.0f / 3.0f) - 1.0f);
        data.push(radius * sinTheta * std::cos(azimuth));
        data.push(radius * sinTheta * std::sin(azimuth));
        data.push(radius * cosTheta);
    }

    std::vector<Particle> generated;
    generated.reserve(std::min(data.particleCount(), static_cast<std::size_t>(context.count) * 2u));
    Vector3 meanPosition(0.0f, 0.0f, 0.0f);
    Vector3 meanVelocity(0.0f, 0.0f, 0.0f);
#pragma omp parallel if (!context.config.deterministicMode && context.velocityTemperature <= 0.0f)
    {
        std::vector<Particle> localParticles;
        Vector3 localPosition(0.0f, 0.0f, 0.0f);
        Vector3 localVelocity(0.0f, 0.0f, 0.0f);
        const std::ptrdiff_t total = static_cast<std::ptrdiff_t>(data.particleCount());
#pragma omp for nowait
        for (std::ptrdiff_t index = 0; index < total; ++index) {
            const std::size_t particleIndex = static_cast<std::size_t>(index);
            Particle particle;
            particle.setMass(mass);
            particle.setPosition(
                context.centralPosition +
                Vector3(data.get(particleIndex, 6), data.get(particleIndex, 7),
                        data.get(particleIndex, 8)));
            particle.setVelocity(
                context.centralVelocity +
                Vector3(data.get(particleIndex, 3), data.get(particleIndex, 4),
                        data.get(particleIndex, 5)));
            context.finalizeParticle(particle);
            localParticles.push_back(particle);
            localPosition += particle.getPosition();
            localVelocity += particle.getVelocity();
        }
#pragma omp critical
        {
            generated.insert(generated.end(), localParticles.begin(), localParticles.end());
            meanPosition += localPosition;
            meanVelocity += localVelocity;
        }
    }

    if (generated.size() > context.count) {
        generated.resize(context.count);
    }
    meanPosition = Vector3(0.0f, 0.0f, 0.0f);
    meanVelocity = Vector3(0.0f, 0.0f, 0.0f);
    for (const Particle& particle : generated) {
        meanPosition += particle.getPosition();
        meanVelocity += particle.getVelocity();
    }
    const float inverseCount = 1.0f / static_cast<float>(generated.size());
    meanPosition = meanPosition * inverseCount;
    meanVelocity = meanVelocity * inverseCount;
    for (Particle& particle : generated) {
        particle.setPosition(particle.getPosition() - meanPosition + context.centralPosition);
        particle.setVelocity(particle.getVelocity() - meanVelocity + context.centralVelocity);
    }
    context.particles.insert(context.particles.end(), generated.begin(), generated.end());
    return context.particles.size() >= 2;
}
