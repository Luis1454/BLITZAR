#include "physics/ForceLawPolicy.hpp"

#include <algorithm>

ForceLawPolicy resolveForceLawPolicy(
    float theta,
    float softening,
    float minSoftening,
    float minDistance2,
    float minTheta)
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
