#pragma once

#include "physics/ParticleSystem.hpp"
#include <vector>

namespace gpu {

    void initializeParticles(std::vector<Particle> &particles);

    void destroyParticles(std::vector<Particle> &particles);

    void callUpdateParticles(std::vector<Particle> &particles, float deltaTime);
}


