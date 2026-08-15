/*
 * @file engine/src/physics/core/ParticleHotData.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Cache-optimized particle data packing helpers for the physics hotpath.
 */

#include "physics/core/ParticleHotData.hpp"
#include "physics/core/Particle.hpp"

#include <cstddef>

#include <omp.h>

void buildParticleHotData(const std::vector<Particle>& particles,
                          std::vector<ParticleHotData>& hotData)
{
    hotData.resize(particles.size());
    const std::ptrdiff_t particleTotal = static_cast<std::ptrdiff_t>(particles.size());
#pragma omp parallel for schedule(static)
    for (std::ptrdiff_t i = 0; i < particleTotal; ++i) {
        const std::size_t particleIndex = static_cast<std::size_t>(i);
        const Particle& particle = particles[particleIndex];
        const Vector3 position = particle.getPosition();
        hotData[particleIndex] = ParticleHotData(position.x, position.y, position.z, particle.getMass());
    }
}

std::vector<ParticleHotData> buildParticleHotData(const std::vector<Particle>& particles)
{
    std::vector<ParticleHotData> hotData;
    buildParticleHotData(particles, hotData);
    return hotData;
}
