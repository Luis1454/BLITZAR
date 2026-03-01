/*
** EPITECH PROJECT, 2024
** rtxcpp
** File description:
** Particle
*/

#include "physics/Particle.hpp"

Particle::Particle()
{
}

Particle::~Particle()
{
}

void Particle::applyForce(Vector3 force)
{
    _force.x += force.x;
    _force.y += force.y;
    _force.z += force.z;
}


