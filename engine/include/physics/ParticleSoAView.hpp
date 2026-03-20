#ifndef GRAVITY_ENGINE_INCLUDE_PHYSICS_PARTICLE_SOA_VIEW_HPP_
#define GRAVITY_ENGINE_INCLUDE_PHYSICS_PARTICLE_SOA_VIEW_HPP_

#include "physics/Vector.hpp"

struct ParticleSoAView {
    float *posX;
    float *posY;
    float *posZ;
    float *velX;
    float *velY;
    float *velZ;
    float *pressX;
    float *pressY;
    float *pressZ;
    float *mass;
    float *temp;
    float *dens;
    int count;
    int _pad[3];
};

GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 getSoAPosition(ParticleSoAView view, int i);
GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setSoAPosition(ParticleSoAView view, int i, Vector3 p);
GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 getSoAVelocity(ParticleSoAView view, int i);
GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setSoAVelocity(ParticleSoAView view, int i, Vector3 v);
GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setSoAPressure(ParticleSoAView view, int i, Vector3 p);
GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 getSoAPressure(ParticleSoAView view, int i);

#include "physics/ParticleSoAView.inl"

#endif // GRAVITY_ENGINE_INCLUDE_PHYSICS_PARTICLE_SOA_VIEW_HPP_
