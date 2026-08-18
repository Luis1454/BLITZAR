/*
 * @file engine/server/simulation/state/SrvCosmology.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Cosmological initial-state generation.
 */

#include "server/simulation/state/SrvGenerationContext.hpp"
#include "core/constants/FndConstants.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>

static float squaredLength(const Vector3& value)
{
    return value.x * value.x + value.y * value.y + value.z * value.z;
}

static float wrapCoordinate(float value, float boxLength)
{
    const float wrapped = std::fmod(value, boxLength);
    return wrapped < 0.0f ? wrapped + boxLength : wrapped;
}

bool buildCosmologyModel(GenerationContext& context)
{
    const CosmologyConfig& cosmology = context.config.cosmology;
    const bool comoving = isComovingCosmology(cosmology);
    const float extent = std::max(0.01f, cosmology.boxHalfExtent);
    const float radius = std::max(0.01f, cosmology.sphereRadius);
    const float particleMass = resolveCosmologyParticleMass(
        cosmology, context.config.particleMass, context.count);
    const float scaleFactor = std::max(1e-6f, cosmology.initialScaleFactor);
    const float omegaMatter = std::max(0.0f, cosmology.omegaMatter);
    const float omegaLambda = std::max(0.0f, cosmology.omegaLambda);
    const float omegaRadiation = std::max(0.0f, cosmology.omegaRadiation);
    const float hubbleSquared =
        cosmology.hubbleH0 * cosmology.hubbleH0 *
        (omegaRadiation / std::pow(scaleFactor, 4.0f) +
         omegaMatter / std::pow(scaleFactor, 3.0f) + omegaLambda);
    const float hubbleRate = std::sqrt(std::max(0.0f, hubbleSquared));
    const float totalMass = cosmologyReferenceTotalMass(cosmology, particleMass, context.count);
    std::fprintf(stderr, "[cosmology] mass_model=%s total_mass=%.9g particle_mass=%.9g\n",
                 cosmology.massModel.c_str(), totalMass, particleMass);
    const float perturbation = std::clamp(cosmology.perturbationAmplitude, 0.0f, 1.0f);
    const float displacementScale = perturbation * extent * 0.05f;
    const float waveNumber = kTwoPi / std::max(2.0f * extent, 0.01f);
    std::uniform_real_distribution<float> unitDistribution(-1.0f, 1.0f);
    const bool cube = toLower(cosmology.geometry) == "cube";
    const float boxLength = 2.0f * extent;
    context.particles.reserve(context.count);

    while (context.particles.size() < context.count) {
        Vector3 lagrangian;
        if (cube) {
            lagrangian = Vector3(unitDistribution(context.rng) * extent,
                                 unitDistribution(context.rng) * extent,
                                 unitDistribution(context.rng) * extent);
        }
        else {
            lagrangian = Vector3(unitDistribution(context.rng) * radius,
                                 unitDistribution(context.rng) * radius,
                                 unitDistribution(context.rng) * radius);
            if (squaredLength(lagrangian) > radius * radius) {
                continue;
            }
        }

        const Vector3 displacement(
            displacementScale * std::sin(waveNumber * lagrangian.x),
            displacementScale * std::sin(waveNumber * lagrangian.y),
            displacementScale * std::sin(waveNumber * lagrangian.z));
        const Vector3 relativePosition = lagrangian + displacement;
        const Vector3 peculiarVelocity =
            displacement * (hubbleRate * std::max(0.0f, cosmology.peculiarVelocityScale));
        Particle particle;
        particle.setMass(particleMass);
        if (comoving) {
            particle.setPosition(Vector3(
                wrapCoordinate(relativePosition.x + extent, boxLength),
                wrapCoordinate(relativePosition.y + extent, boxLength),
                wrapCoordinate(relativePosition.z + extent, boxLength)));
            particle.setVelocity(peculiarVelocity * scaleFactor);
        }
        else {
            particle.setPosition(context.centralPosition + relativePosition);
            particle.setVelocity(context.centralVelocity + peculiarVelocity);
        }
        context.finalizeParticle(particle);
        context.particles.push_back(particle);
    }
    return context.particles.size() >= 2;
}
