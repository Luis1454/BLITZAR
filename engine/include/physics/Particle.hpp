// File: engine/include/physics/Particle.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_PHYSICS_PARTICLE_HPP_
#define GRAVITY_ENGINE_INCLUDE_PHYSICS_PARTICLE_HPP_
/*
 ** EPITECH PROJECT, 2024
 ** rtxcpp
 ** File description:
 ** test
 */
#include "physics/Vector.hpp"
#include <vector>

/// Description: Defines the Particle data or behavior contract.
class Particle {
public:
    static constexpr float kDefaultMass = 0.01f;
    static constexpr int kDefaultCudaBlockSize = 256;
    /// Description: Describes the particle operation contract.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE Particle();
    /// Description: Releases resources owned by Particle.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE ~Particle();
    /// Description: Describes the update operation contract.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void update(float deltaTime);
    /// Description: Describes the set mass operation contract.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setMass(float mass);
    /// Description: Describes the move operation contract.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void move(Vector3 force);
    /// Description: Describes the bounce operation contract.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void bounce(Vector3 normal, float dt);
    /// Description: Describes the apply force operation contract.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void applyForce(Vector3 force);
    /// Description: Describes the set density operation contract.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setDensity(float density);
    /// Description: Describes the set position operation contract.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setPosition(Vector3 position);
    /// Description: Describes the set velocity operation contract.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setVelocity(Vector3 velocity);
    /// Description: Describes the set pressure operation contract.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setPressure(Vector3 pressure);
    /// Description: Describes the set temperature operation contract.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setTemperature(float temperature);
    /// Description: Describes the get pressure operation contract.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 getPressure() const;
    /// Description: Describes the get position operation contract.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 getPosition() const;
    /// Description: Describes the get velocity operation contract.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 getVelocity() const;
    /// Description: Describes the get density operation contract.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE float getDensity() const;
    /// Description: Describes the get mass operation contract.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE float getMass() const;
    /// Description: Describes the get temperature operation contract.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE float getTemperature() const;

private:
    Vector3 _position;
    Vector3 _velocity;
    Vector3 _pressure;
    Vector3 _force;
    float _density;
    float _mass;
    float _temperature;
};
#endif // GRAVITY_ENGINE_INCLUDE_PHYSICS_PARTICLE_HPP_
