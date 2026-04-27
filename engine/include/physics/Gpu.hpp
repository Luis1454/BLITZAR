// File: engine/include/physics/Gpu.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_PHYSICS_GPU_HPP_
#define GRAVITY_ENGINE_INCLUDE_PHYSICS_GPU_HPP_
#include "physics/ParticleSystem.hpp"
#include <vector>
namespace gpu {
/// Description: Executes the initializeParticles operation.
void initializeParticles(std::vector<Particle>& particles);
/// Description: Executes the destroyParticles operation.
void destroyParticles(std::vector<Particle>& particles);
/// Description: Executes the callUpdateParticles operation.
void callUpdateParticles(std::vector<Particle>& particles, float deltaTime);
} // namespace gpu
#endif // GRAVITY_ENGINE_INCLUDE_PHYSICS_GPU_HPP_
