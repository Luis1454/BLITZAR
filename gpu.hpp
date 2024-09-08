/*
** EPITECH PROJECT, 2024
** rtxcpp
** File description:
** gpu
*/

#ifndef GPU_HPP_
#define GPU_HPP_

#include "ParticleSystem.hpp"

namespace gpu {

    void initializeParticles(Particle *particles, int numParticles);

    void destroyParticles(Particle *particles);

    void callUpdateParticles(Particle *particles, int numParticles, float deltaTime);
}

#endif /* !GPU_HPP_ */
