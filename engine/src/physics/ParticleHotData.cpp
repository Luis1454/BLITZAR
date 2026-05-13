/*
 * @file engine/src/physics/ParticleHotData.cpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Cache-optimized particle data packing helpers for the physics hotpath.
 */

#include "physics/ParticleHotData.hpp"
#include "physics/Particle.hpp"

#include <omp.h>

void buildParticleHotData(const std::vector<Particle>& particles,
                          std::vector<ParticleHotData>& hotData)
{
    hotData.resize(particles.size());
#pragma omp parallel for schedule(static)
    for (std::size_t i = 0; i < particles.size(); ++i) {
        const Particle& particle = particles[i];
        const Vector3 position = particle.getPosition();
        hotData[i] = ParticleHotData(position.x, position.y, position.z, particle.getMass());
    }
}

std::vector<ParticleHotData> buildParticleHotData(const std::vector<Particle>& particles)
{
    std::vector<ParticleHotData> hotData;
    buildParticleHotData(particles, hotData);
    return hotData;
}