/*
** EPITECH PROJECT, 2024
** rtxcpp
** File description:
** ParticleSystem
*/

#ifndef PARTICLESYSTEM_HPP_
#define PARTICLESYSTEM_HPP_

#include "Octree.hpp"

class ParticleSystem {
    public:
        ParticleSystem(int numParticles);
        ~ParticleSystem();

        void update(float deltaTime);

        static Octree buildOctree(Particle *particles, int numParticles);

        static Vector3 getForce(Particle *last, Particle *particle, int numParticles, int idx, float dt);
        std::vector<Particle> &getParticles();

    private:
        std::vector<Particle> _particles;
};

#endif /* !PARTICLESYSTEM_HPP_ */
