#ifndef GRAVITY_ENGINE_INCLUDE_PHYSICS_GPU_HPP_
#define GRAVITY_ENGINE_INCLUDE_PHYSICS_GPU_HPP_
#include "physics/ParticleSystem.hpp"
#include <vector>
namespace gpu {
void initializeParticles(std::vector<Particle>& particles);
void destroyParticles(std::vector<Particle>& particles);
void callUpdateParticles(std::vector<Particle>& particles, float deltaTime);
} // namespace gpu
#endif // GRAVITY_ENGINE_INCLUDE_PHYSICS_GPU_HPP_
