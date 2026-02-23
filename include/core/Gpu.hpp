/*
** EPITECH PROJECT, 2024
** rtxcpp
** File description:
** gpu
*/

#ifndef GPU_HPP_
#define GPU_HPP_

#include "core/ParticleSystem.hpp"
#include <vector>

namespace gpu {

    void initializeParticles(std::vector<Particle> &particles);

    void destroyParticles(std::vector<Particle> &particles);

    void callUpdateParticles(std::vector<Particle> &particles, float deltaTime);
}

#endif /* !GPU_HPP_ */

