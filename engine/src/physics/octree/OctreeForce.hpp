/*
 * @file engine/src/physics/octree/OctreeForce.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Private shared force helper declarations.
 */

#ifndef BLITZAR_ENGINE_SRC_PHYSICS_OCTREE_OCTREEFORCE_HPP_
#define BLITZAR_ENGINE_SRC_PHYSICS_OCTREE_OCTREEFORCE_HPP_

#include "physics/core/ForceLawPolicy.hpp"
#include "physics/core/Vector.hpp"

namespace blitzar_physics_particle_system_host {
float squaredLength(Vector3 value);
float softenedDistanceSquared(Vector3 delta, const ForceLawPolicy& policy);
Vector3 accelerationFromSource(Vector3 selfPosition, Vector3 sourcePosition, float sourceMass,
                               const ForceLawPolicy& policy);
} // namespace blitzar_physics_particle_system_host

#endif
