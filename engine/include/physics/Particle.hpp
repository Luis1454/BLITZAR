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
    /// Description: Executes the Particle operation.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE Particle();
    /// Description: Releases resources owned by Particle.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE ~Particle();
    /// Description: Executes the update operation.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void update(float deltaTime);
    /// Description: Executes the setMass operation.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setMass(float mass);
    /// Description: Executes the move operation.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void move(Vector3 force);
    /// Description: Executes the bounce operation.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void bounce(Vector3 normal, float dt);
    /// Description: Executes the applyForce operation.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void applyForce(Vector3 force);
    /// Description: Executes the setDensity operation.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setDensity(float density);
    /// Description: Executes the setPosition operation.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setPosition(Vector3 position);
    /// Description: Executes the setVelocity operation.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setVelocity(Vector3 velocity);
    /// Description: Executes the setPressure operation.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setPressure(Vector3 pressure);
    /// Description: Executes the setTemperature operation.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setTemperature(float temperature);
    /// Description: Executes the getPressure operation.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 getPressure() const;
    /// Description: Executes the getPosition operation.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 getPosition() const;
    /// Description: Executes the getVelocity operation.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 getVelocity() const;
    /// Description: Executes the getDensity operation.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE float getDensity() const;
    /// Description: Executes the getMass operation.
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE float getMass() const;
    /// Description: Executes the getTemperature operation.
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
