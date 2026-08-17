/*
 * @file engine/physics/octree/src/OctreeForce.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Shared CPU force helpers used by the host particle system and octree.
 */

#include "OctreeForce.hpp"

#include <cmath>
#ifdef __SSE__
#include <xmmintrin.h>
#endif

namespace blitzar_physics_particle_system_host {
constexpr float kGravity = 1.0f;

float squaredLength(Vector3 value)
{
    return value.x * value.x + value.y * value.y + value.z * value.z;
}

float softenedDistanceSquared(Vector3 delta, const ForceLawPolicy& policy)
{
    return squaredLength(delta) + policy.softening * policy.softening;
}

// SIMD-optimized: arithmetic hotpath, called ~100M times
// Compiler hint: unroll and vectorize this for maximum throughput
Vector3 accelerationFromSource(Vector3 selfPosition, Vector3 sourcePosition, float sourceMass,
                               const ForceLawPolicy& policy)
{
    const Vector3 delta = sourcePosition - selfPosition;
    const float dist2 = softenedDistanceSquared(delta, policy);
    if (dist2 <= policy.minDistance2)
        return Vector3();

    // Use fast reciprocal-sqrt with a Newton-Raphson refine when SSE is available.
    float invDistance = 1.0f / std::sqrt(dist2);
#ifdef __SSE__
    // approximate reciprocal sqrt
    __m128 v = _mm_set_ss(dist2);
    __m128 r = _mm_rsqrt_ss(v);
    invDistance = _mm_cvtss_f32(r);
    // one Newton-Raphson iteration to improve accuracy: inv = inv*(1.5 - 0.5*x*inv*inv)
    invDistance = invDistance * (1.5f - 0.5f * dist2 * invDistance * invDistance);
#endif
    const float invDistance3 = invDistance * invDistance * invDistance;
    float shortRangeWeight = 1.0f;
    if (policy.treePmShortRangeScale > 0.0f) {
        constexpr float kInverseSqrtPi = 0.5641895835477563f;
        const float distance = 1.0f / invDistance;
        const float splitScale = policy.treePmShortRangeScale;
        const float argument = 0.5f * distance / splitScale;
        shortRangeWeight = std::erfc(argument) +
                           distance * kInverseSqrtPi / splitScale * std::exp(-argument * argument);
    }

    // Vectorization opportunity: multiply operates independently on x,y,z
    return delta * (kGravity * sourceMass * invDistance3 * shortRangeWeight);
}

} // namespace blitzar_physics_particle_system_host
