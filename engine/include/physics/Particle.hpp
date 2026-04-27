/*
 * @file engine/include/physics/Particle.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Public physics interfaces and data contracts for deterministic simulation kernels.
 */

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

/*
 * @brief Defines the particle type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class Particle {
public:
    static constexpr float kDefaultMass = 0.01f;
    static constexpr int kDefaultCudaBlockSize = 256;
    /*
     * @brief Documents the particle operation contract.
     * @param None This contract does not take explicit parameters.
     * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE Particle();
    /*
     * @brief Documents the ~particle operation contract.
     * @param None This contract does not take explicit parameters.
     * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE ~Particle();
    /*
     * @brief Documents the update operation contract.
     * @param deltaTime Input value used by this contract.
     * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE void value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void update(float deltaTime);
    /*
     * @brief Documents the set mass operation contract.
     * @param mass Input value used by this contract.
     * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE void value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setMass(float mass);
    /*
     * @brief Documents the move operation contract.
     * @param force Input value used by this contract.
     * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE void value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void move(Vector3 force);
    /*
     * @brief Documents the bounce operation contract.
     * @param normal Input value used by this contract.
     * @param dt Input value used by this contract.
     * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE void value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void bounce(Vector3 normal, float dt);
    /*
     * @brief Documents the apply force operation contract.
     * @param force Input value used by this contract.
     * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE void value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void applyForce(Vector3 force);
    /*
     * @brief Documents the set density operation contract.
     * @param density Input value used by this contract.
     * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE void value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setDensity(float density);
    /*
     * @brief Documents the set position operation contract.
     * @param position Input value used by this contract.
     * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE void value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setPosition(Vector3 position);
    /*
     * @brief Documents the set velocity operation contract.
     * @param velocity Input value used by this contract.
     * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE void value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setVelocity(Vector3 velocity);
    /*
     * @brief Documents the set pressure operation contract.
     * @param pressure Input value used by this contract.
     * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE void value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setPressure(Vector3 pressure);
    /*
     * @brief Documents the set temperature operation contract.
     * @param temperature Input value used by this contract.
     * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE void value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE void setTemperature(float temperature);
    /*
     * @brief Documents the get pressure operation contract.
     * @param None This contract does not take explicit parameters.
     * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 getPressure() const;
    /*
     * @brief Documents the get position operation contract.
     * @param None This contract does not take explicit parameters.
     * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 getPosition() const;
    /*
     * @brief Documents the get velocity operation contract.
     * @param None This contract does not take explicit parameters.
     * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 getVelocity() const;
    /*
     * @brief Documents the get density operation contract.
     * @param None This contract does not take explicit parameters.
     * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE float value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE float getDensity() const;
    /*
     * @brief Documents the get mass operation contract.
     * @param None This contract does not take explicit parameters.
     * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE float value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    GRAVITY_HD_HOST GRAVITY_HD_DEVICE float getMass() const;
    /*
     * @brief Documents the get temperature operation contract.
     * @param None This contract does not take explicit parameters.
     * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE float value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
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
