/*
 * @file engine/src/physics/Particle.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Physics and CUDA implementation for the deterministic simulation core.
 */

/*
** EPITECH PROJECT, 2024
** rtxcpp
** File description:
** Particle
*/
#include "physics/Particle.hpp"

/*
 * @brief Documents the particle operation contract.
 * @param None This contract does not take explicit parameters.
 * @return Particle:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
Particle::Particle()
{
}

/*
 * @brief Documents the ~particle operation contract.
 * @param None This contract does not take explicit parameters.
 * @return Particle:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
Particle::~Particle()
{
}

/*
 * @brief Documents the apply force operation contract.
 * @param force Input value used by this contract.
 * @return void Particle:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void Particle::applyForce(Vector3 force)
{
    _force.x += force.x;
    _force.y += force.y;
    _force.z += force.z;
}
