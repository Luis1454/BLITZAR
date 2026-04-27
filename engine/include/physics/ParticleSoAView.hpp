/*
 * @file engine/include/physics/ParticleSoAView.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Public physics interfaces and data contracts for deterministic simulation kernels.
 */

#ifndef GRAVITY_ENGINE_INCLUDE_PHYSICS_PARTICLE_SOA_VIEW_HPP_
#define GRAVITY_ENGINE_INCLUDE_PHYSICS_PARTICLE_SOA_VIEW_HPP_
#include "physics/Vector.hpp"

/*
 * @brief Defines the particle so aview type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct ParticleSoAView {
    float* posX;
    float* posY;
    float* posZ;
    float* velX;
    float* velY;
    float* velZ;
    float* pressX;
    float* pressY;
    float* pressZ;
    float* mass;
    float* temp;
    float* dens;
    int count;
    int _pad[3];
};

/*
 * @brief Documents the get so aposition operation contract.
 * @param view Input value used by this contract.
 * @param i Input value used by this contract.
 * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 getSoAPosition(ParticleSoAView view, int i);
/*
 * @brief Documents the set so aposition operation contract.
 * @param view Input value used by this contract.
 * @param i Input value used by this contract.
 * @param p Input value used by this contract.
 * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE void value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setSoAPosition(ParticleSoAView view, int i, Vector3 p);
/*
 * @brief Documents the get so avelocity operation contract.
 * @param view Input value used by this contract.
 * @param i Input value used by this contract.
 * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 getSoAVelocity(ParticleSoAView view, int i);
/*
 * @brief Documents the set so avelocity operation contract.
 * @param view Input value used by this contract.
 * @param i Input value used by this contract.
 * @param v Input value used by this contract.
 * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE void value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setSoAVelocity(ParticleSoAView view, int i, Vector3 v);
/*
 * @brief Documents the set so apressure operation contract.
 * @param view Input value used by this contract.
 * @param i Input value used by this contract.
 * @param p Input value used by this contract.
 * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE void value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setSoAPressure(ParticleSoAView view, int i, Vector3 p);
/*
 * @brief Documents the get so apressure operation contract.
 * @param view Input value used by this contract.
 * @param i Input value used by this contract.
 * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 getSoAPressure(ParticleSoAView view, int i);
#include "physics/ParticleSoAView.inl"
#endif // GRAVITY_ENGINE_INCLUDE_PHYSICS_PARTICLE_SOA_VIEW_HPP_
