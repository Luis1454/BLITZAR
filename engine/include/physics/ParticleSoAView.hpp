// File: engine/include/physics/ParticleSoAView.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_PHYSICS_PARTICLE_SOA_VIEW_HPP_
#define GRAVITY_ENGINE_INCLUDE_PHYSICS_PARTICLE_SOA_VIEW_HPP_
#include "physics/Vector.hpp"
/// Description: Defines the ParticleSoAView data or behavior contract.
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
/// Description: Executes the getSoAPosition operation.
GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 getSoAPosition(ParticleSoAView view, int i);
/// Description: Executes the setSoAPosition operation.
GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setSoAPosition(ParticleSoAView view, int i, Vector3 p);
/// Description: Executes the getSoAVelocity operation.
GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 getSoAVelocity(ParticleSoAView view, int i);
/// Description: Executes the setSoAVelocity operation.
GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setSoAVelocity(ParticleSoAView view, int i, Vector3 v);
/// Description: Executes the setSoAPressure operation.
GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setSoAPressure(ParticleSoAView view, int i, Vector3 p);
/// Description: Executes the getSoAPressure operation.
GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 getSoAPressure(ParticleSoAView view, int i);
#include "physics/ParticleSoAView.inl"
#endif // GRAVITY_ENGINE_INCLUDE_PHYSICS_PARTICLE_SOA_VIEW_HPP_
