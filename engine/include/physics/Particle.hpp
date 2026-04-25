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
class Particle {
public:
    static constexpr float kDefaultMass = 0.01f;
    static constexpr int kDefaultCudaBlockSize = 256;
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE Particle();
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE ~Particle();
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void update(float deltaTime);
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setMass(float mass);
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void move(Vector3 force);
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void bounce(Vector3 normal, float dt);
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void applyForce(Vector3 force);
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setDensity(float density);
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setPosition(Vector3 position);
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setVelocity(Vector3 velocity);
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setPressure(Vector3 pressure);
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setTemperature(float temperature);
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 getPressure() const;
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 getPosition() const;
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 getVelocity() const;
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE float getDensity() const;
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE float getMass() const;
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
