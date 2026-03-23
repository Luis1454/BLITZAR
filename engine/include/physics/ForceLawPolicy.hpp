#ifndef GRAVITY_ENGINE_INCLUDE_PHYSICS_FORCELAWPOLICY_HPP_
#define GRAVITY_ENGINE_INCLUDE_PHYSICS_FORCELAWPOLICY_HPP_

/*
 * Module: physics
 * Responsibility: Normalize the canonical force-law clamps shared by all gravity solvers.
 */

struct ForceLawPolicy {
    float theta = 0.05f;
    float softening = 1.0e-4f;
    float minSoftening = 1.0e-4f;
    float minDistance2 = 1.0e-12f;
    float minTheta = 0.05f;
};

ForceLawPolicy resolveForceLawPolicy(
    float theta,
    float softening,
    float minSoftening,
    float minDistance2,
    float minTheta);

#endif // GRAVITY_ENGINE_INCLUDE_PHYSICS_FORCELAWPOLICY_HPP_
