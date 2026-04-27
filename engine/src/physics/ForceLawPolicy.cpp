/*
 * @file engine/src/physics/ForceLawPolicy.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Physics and CUDA implementation for the deterministic simulation core.
 */

#include "physics/ForceLawPolicy.hpp"
#include <algorithm>

/*
 * @brief Documents the resolve force law policy operation contract.
 * @param theta Input value used by this contract.
 * @param softening Input value used by this contract.
 * @param minSoftening Input value used by this contract.
 * @param minDistance2 Input value used by this contract.
 * @param minTheta Input value used by this contract.
 * @return ForceLawPolicy value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
ForceLawPolicy resolveForceLawPolicy(float theta, float softening, float minSoftening,
                                     float minDistance2, float minTheta)
{
    ForceLawPolicy policy;
    constexpr float kMinThetaFloor = 0.05f;
    constexpr float kMaxThetaCeiling = 4.0f;
    policy.minSoftening = std::max(0.0f, minSoftening);
    policy.minDistance2 = std::max(0.0f, minDistance2);
    policy.minTheta = std::clamp(minTheta, kMinThetaFloor, kMaxThetaCeiling);
    policy.softening = std::max(softening, policy.minSoftening);
    policy.theta = std::clamp(theta, policy.minTheta, kMaxThetaCeiling);
    return policy;
}
