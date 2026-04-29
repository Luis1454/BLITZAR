/*
 * @file engine/include/physics/Gpu.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Public physics interfaces and data contracts for deterministic simulation kernels.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_PHYSICS_GPU_HPP_
#define BLITZAR_ENGINE_INCLUDE_PHYSICS_GPU_HPP_
#include "physics/ParticleSystem.hpp"
#include <vector>

namespace gpu {
void initializeParticles(std::vector<Particle>& particles);
void destroyParticles(std::vector<Particle>& particles);
void callUpdateParticles(std::vector<Particle>& particles, float deltaTime);
} // namespace gpu
#endif // BLITZAR_ENGINE_INCLUDE_PHYSICS_GPU_HPP_
