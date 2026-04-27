// File: engine/src/physics/Particle.cpp
// Purpose: Engine implementation for the BLITZAR simulation core.

/*
** EPITECH PROJECT, 2024
** rtxcpp
** File description:
** Particle
*/
#include "physics/Particle.hpp"
/// Description: Executes the Particle operation.
Particle::Particle()
{
}
/// Description: Releases resources owned by Particle.
Particle::~Particle()
{
}
/// Description: Executes the applyForce operation.
void Particle::applyForce(Vector3 force)
{
    _force.x += force.x;
    _force.y += force.y;
    _force.z += force.z;
}
