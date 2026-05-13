/*
 * @file engine/include/physics/ForceLawPolicy.hpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Public physics interfaces and data contracts for deterministic simulation kernels.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_PHYSICS_FORCELAWPOLICY_HPP_
#define BLITZAR_ENGINE_INCLUDE_PHYSICS_FORCELAWPOLICY_HPP_

/*
 * Module: physics
 * Responsibility: Normalize the canonical force-law clamps shared by all
 * blitzar solvers.
 */
#include "Constants.hpp"
/*
 * @brief Defines the force law policy type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct ForceLawPolicy {
    float theta = kPhysicsMinTheta;
    float softening = kPhysicsMinSofteningDefault;
    float minSoftening = kPhysicsMinSofteningDefault;
    float minDistance2 = kPhysicsMinDistance2Default;
    float minTheta = kPhysicsMinTheta;
};

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
                                     float minDistance2, float minTheta);
#endif // BLITZAR_ENGINE_INCLUDE_PHYSICS_FORCELAWPOLICY_HPP_
