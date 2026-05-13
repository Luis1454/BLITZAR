/*
 * @file engine/include/physics/ParticleHotData.hpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Cache-optimized particle data for force computation hotpath.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_PHYSICS_PARTICLE_HOT_DATA_HPP_
#define BLITZAR_ENGINE_INCLUDE_PHYSICS_PARTICLE_HOT_DATA_HPP_

#include "physics/Vector.hpp"
#include <vector>

/*
 * @brief Cache-line-aligned particle data for force computation.
 * Only position + mass (16 bytes per particle vs 60 bytes full Particle struct).
 * Reduces L1 cache pressure during octree traversal and force accumulation.
 *
 * Expected layout (32-byte aligned):
 *   x, y, z (12 bytes) + mass (4 bytes) + 16 bytes padding = 32 bytes cache line
 *
 * This enables:
 *   - Prefetch-friendly iteration (one particle per cache line)
 *   - SIMD vectorization without memory stalls
 *   - 3-4x fewer cache misses vs AoS Particle layout
 */
struct alignas(32) ParticleHotData {
    float x;
    float y;
    float z;
    float mass;

    ParticleHotData() : x(0.0f), y(0.0f), z(0.0f), mass(0.0f) {}

    ParticleHotData(float px, float py, float pz, float m) : x(px), y(py), z(pz), mass(m) {}

    Vector3 getPosition() const { return Vector3(x, y, z); }
    float getMass() const { return mass; }
};

/*
 * @brief Build cache-optimized hot data from full particle list.
 * Call once per simulation step to populate cache-friendly data structure.
 */
void buildParticleHotData(const std::vector<class Particle>& particles,
                          std::vector<ParticleHotData>& hotData);

std::vector<ParticleHotData> buildParticleHotData(const std::vector<class Particle>& particles);

#endif // BLITZAR_ENGINE_INCLUDE_PHYSICS_PARTICLE_HOT_DATA_HPP_
