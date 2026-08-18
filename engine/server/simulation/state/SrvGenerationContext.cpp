/*
 * @file engine/server/simulation/state/SrvGenerationContext.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Shared particle initialization behavior.
 */

#include "server/simulation/state/SrvGenerationContext.hpp"

#include <algorithm>
#include <cmath>

GenerationContext::GenerationContext(std::vector<Particle>& output,
                                     std::uint32_t particleCount,
                                     const InitialStateConfig& initialConfig)
    : particles(output),
      count(std::max<std::uint32_t>(2u, particleCount)),
      config(initialConfig),
      rng(initialConfig.seed),
      centralMass(std::max(1e-6f, initialConfig.centralMass)),
      velocityTemperature(std::max(0.0f, initialConfig.velocityTemperature)),
      particleTemperature(std::max(0.0f, initialConfig.particleTemperature)),
      centralPosition(initialConfig.centralX, initialConfig.centralY, initialConfig.centralZ),
      centralVelocity(initialConfig.centralVx, initialConfig.centralVy, initialConfig.centralVz)
{
    particles.clear();
}

void GenerationContext::finalizeParticle(Particle& particle)
{
    if (velocityTemperature > 0.0f) {
        float sigma = std::sqrt(velocityTemperature) * 0.005f;
        sigma = std::min(sigma, 0.1f);
        if (sigma > 0.0f) {
            std::normal_distribution<float> thermalDistribution(0.0f, sigma);
            const Vector3 velocity = particle.getVelocity();
            particle.setVelocity(Vector3(velocity.x + thermalDistribution(rng),
                                         velocity.y + thermalDistribution(rng),
                                         velocity.z + thermalDistribution(rng)));
        }
    }
    particle.setPressure(Vector3(0.0f, 0.0f, 0.0f));
    particle.setDensity(0.0f);
    particle.setTemperature(particleTemperature);
}

void GenerationContext::addCentralBody()
{
    if (!config.includeCentralBody) {
        return;
    }
    Particle central;
    central.setMass(centralMass);
    central.setPosition(centralPosition);
    central.setVelocity(centralVelocity);
    central.setPressure(Vector3(0.0f, 0.0f, 0.0f));
    central.setDensity(0.0f);
    central.setTemperature(particleTemperature);
    particles.push_back(central);
}
