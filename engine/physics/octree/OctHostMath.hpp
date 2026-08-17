/*
 * @file engine/physics/octree/OctHostMath.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Shared host-side math helpers for ParticleSystem strategies.
 */

#ifndef BLITZAR_ENGINE_SRC_PHYSICS_OCTREE_HOST_MATH_HPP_
#define BLITZAR_ENGINE_SRC_PHYSICS_OCTREE_HOST_MATH_HPP_

#include "PhyParticleSystem.hpp"

namespace blitzar_physics_particle_system_host {
float cosmologyHubbleRate(const CosmologyConfig& config, float scaleFactor);
float advanceCosmologyScaleFactor(const CosmologyConfig& config, float scaleFactor,
                                  float deltaTime);
float comovingDriftFactor(const CosmologyConfig& config, float a0, float a1);
float wrapComovingCoordinate(float value, float boxLength);
Vector3 clampedVector(Vector3 value, float limit);
Particle makeParticle(Vector3 position, Vector3 velocity);
} // namespace blitzar_physics_particle_system_host

#endif // BLITZAR_ENGINE_SRC_PHYSICS_OCTREE_HOST_MATH_HPP_
