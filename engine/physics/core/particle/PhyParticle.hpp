/*
 * @file engine/physics/core/particle/PhyParticle.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Public physics interfaces and data contracts for deterministic simulation kernels.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_PHYSICS_PARTICLE_HPP_
#define BLITZAR_ENGINE_INCLUDE_PHYSICS_PARTICLE_HPP_
/*
 ** EPITECH PROJECT, 2024
 ** rtxcpp
 ** File description:
 ** test
 */
#include "physics/core/vector/PhyVector.hpp"
#include <vector>

/*
 * @brief Defines the particle type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class Particle {
public:
    static constexpr float kDefaultMass = 0.01f;
    static constexpr int kDefaultCudaBlockSize = 256;  // Tuned for occupancy on octree traversal (Oct 2024)
    /*
     * @brief Documents the particle operation contract.
     * @param None This contract does not take explicit parameters.
     * @return BLITZAR_HD_HOST BLITZAR_HD_DEVICE value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    BLITZAR_HD_HOST BLITZAR_HD_DEVICE Particle();
    /*
     * @brief Documents the ~particle operation contract.
     * @param None This contract does not take explicit parameters.
     * @return BLITZAR_HD_HOST BLITZAR_HD_DEVICE value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    BLITZAR_HD_HOST BLITZAR_HD_DEVICE ~Particle();
    /*
     * @brief Documents the update operation contract.
     * @param deltaTime Input value used by this contract.
     * @return BLITZAR_HD_HOST BLITZAR_HD_DEVICE void value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    BLITZAR_HD_HOST BLITZAR_HD_DEVICE void update(float deltaTime);
    /*
     * @brief Documents the set mass operation contract.
     * @param mass Input value used by this contract.
     * @return BLITZAR_HD_HOST BLITZAR_HD_DEVICE void value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    BLITZAR_HD_HOST BLITZAR_HD_DEVICE void setMass(float mass);
    /*
     * @brief Documents the move operation contract.
     * @param force Input value used by this contract.
     * @return BLITZAR_HD_HOST BLITZAR_HD_DEVICE void value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    BLITZAR_HD_HOST BLITZAR_HD_DEVICE void move(Vector3 force);
    /*
     * @brief Documents the bounce operation contract.
     * @param normal Input value used by this contract.
     * @param dt Input value used by this contract.
     * @return BLITZAR_HD_HOST BLITZAR_HD_DEVICE void value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    BLITZAR_HD_HOST BLITZAR_HD_DEVICE void bounce(Vector3 normal, float dt);
    /*
     * @brief Documents the apply force operation contract.
     * @param force Input value used by this contract.
     * @return BLITZAR_HD_HOST BLITZAR_HD_DEVICE void value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    BLITZAR_HD_HOST BLITZAR_HD_DEVICE void applyForce(Vector3 force);
    /*
     * @brief Documents the set density operation contract.
     * @param density Input value used by this contract.
     * @return BLITZAR_HD_HOST BLITZAR_HD_DEVICE void value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    BLITZAR_HD_HOST BLITZAR_HD_DEVICE void setDensity(float density);
    /*
     * @brief Documents the set position operation contract.
     * @param position Input value used by this contract.
     * @return BLITZAR_HD_HOST BLITZAR_HD_DEVICE void value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    BLITZAR_HD_HOST BLITZAR_HD_DEVICE void setPosition(Vector3 position);
    /*
     * @brief Documents the set velocity operation contract.
     * @param velocity Input value used by this contract.
     * @return BLITZAR_HD_HOST BLITZAR_HD_DEVICE void value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    BLITZAR_HD_HOST BLITZAR_HD_DEVICE void setVelocity(Vector3 velocity);
    /*
     * @brief Documents the set pressure operation contract.
     * @param pressure Input value used by this contract.
     * @return BLITZAR_HD_HOST BLITZAR_HD_DEVICE void value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    BLITZAR_HD_HOST BLITZAR_HD_DEVICE void setPressure(Vector3 pressure);
    /*
     * @brief Documents the set temperature operation contract.
     * @param temperature Input value used by this contract.
     * @return BLITZAR_HD_HOST BLITZAR_HD_DEVICE void value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    BLITZAR_HD_HOST BLITZAR_HD_DEVICE void setTemperature(float temperature);
    /*
     * @brief Documents the get pressure operation contract.
     * @param None This contract does not take explicit parameters.
     * @return BLITZAR_HD_HOST BLITZAR_HD_DEVICE Vector3 value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    BLITZAR_HD_HOST BLITZAR_HD_DEVICE Vector3 getPressure() const;
    /*
     * @brief Documents the get position operation contract.
     * @param None This contract does not take explicit parameters.
     * @return BLITZAR_HD_HOST BLITZAR_HD_DEVICE Vector3 value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    BLITZAR_HD_HOST BLITZAR_HD_DEVICE Vector3 getPosition() const;
    /*
     * @brief Documents the get velocity operation contract.
     * @param None This contract does not take explicit parameters.
     * @return BLITZAR_HD_HOST BLITZAR_HD_DEVICE Vector3 value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    BLITZAR_HD_HOST BLITZAR_HD_DEVICE Vector3 getVelocity() const;
    /*
     * @brief Documents the get density operation contract.
     * @param None This contract does not take explicit parameters.
     * @return BLITZAR_HD_HOST BLITZAR_HD_DEVICE float value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    BLITZAR_HD_HOST BLITZAR_HD_DEVICE float getDensity() const;
    /*
     * @brief Documents the get mass operation contract.
     * @param None This contract does not take explicit parameters.
     * @return BLITZAR_HD_HOST BLITZAR_HD_DEVICE float value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    BLITZAR_HD_HOST BLITZAR_HD_DEVICE float getMass() const;
    /*
     * @brief Documents the get temperature operation contract.
     * @param None This contract does not take explicit parameters.
     * @return BLITZAR_HD_HOST BLITZAR_HD_DEVICE float value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    BLITZAR_HD_HOST BLITZAR_HD_DEVICE float getTemperature() const;

private:
    Vector3 _position;
    Vector3 _velocity;
    Vector3 _pressure;
    Vector3 _force;
    float _density;
    float _mass;
    float _temperature;
};



/*
 * @file engine/physics/core/particle/PhyParticle.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Inline host/device particle operations shared by CPU and CUDA builds.
 */

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline Particle::Particle()
    : _position(Vector3{0.0f, 0.0f, 0.0f}),
      _velocity(Vector3{0.0f, 0.0f, 0.0f}),
      _pressure(Vector3{0.0f, 0.0f, 0.0f}),
      _force(Vector3{0.0f, 0.0f, 0.0f}),
      _density(0.0f),
      _mass(Particle::kDefaultMass),
      _temperature(0.0f)
{
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline Particle::~Particle()
{
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline void Particle::update(float deltaTime)
{
    _velocity += _force * deltaTime;
    _position += _velocity * deltaTime;
    _force = Vector3{0.0f, 0.0f, 0.0f};
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline void Particle::setMass(float mass)
{
    _mass = mass;
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline void Particle::move(Vector3 force)
{
    _position += force;
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline void Particle::bounce(Vector3 normal, float dt)
{
    _position -= _velocity * dt;
    _velocity -= normal * 2.0f * dot(_velocity, normal);
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline void Particle::applyForce(Vector3 force)
{
    _force += force;
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline void Particle::setDensity(float density)
{
    _density = density;
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline void Particle::setPosition(Vector3 position)
{
    _position = position;
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline void Particle::setVelocity(Vector3 velocity)
{
    _velocity = velocity;
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline void Particle::setPressure(Vector3 pressure)
{
    _pressure = pressure;
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline void Particle::setTemperature(float temperature)
{
    _temperature = temperature;
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline Vector3 Particle::getPressure() const
{
    return _pressure;
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline Vector3 Particle::getPosition() const
{
    return _position;
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline Vector3 Particle::getVelocity() const
{
    return _velocity;
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline float Particle::getDensity() const
{
    return _density;
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline float Particle::getMass() const
{
    return _mass;
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline float Particle::getTemperature() const
{
    return _temperature;
}
#endif // BLITZAR_ENGINE_INCLUDE_PHYSICS_PARTICLE_HPP_
