/*
** EPITECH PROJECT, 2024
** rtxcpp
** File description:
** test
*/

#include <vector>
#include "physics/Vector.hpp"

class Particle {
    public:
        static constexpr float kDefaultMass = 0.01f;
        static constexpr int kDefaultCudaBlockSize = 256;

        GRAVITY_HD Particle();
        GRAVITY_HD ~Particle();

        GRAVITY_HD void update(float deltaTime);
        GRAVITY_HD void setMass(float mass);
        GRAVITY_HD void move(Vector3 force);
        GRAVITY_HD void bounce(Vector3 normal, float dt);
        GRAVITY_HD void applyForce(Vector3 force);
        GRAVITY_HD void setDensity(float density);
        GRAVITY_HD void setPosition(Vector3 position);
        GRAVITY_HD void setVelocity(Vector3 velocity);
        GRAVITY_HD void setPressure(Vector3 pressure);
        GRAVITY_HD void setTemperature(float temperature);
        GRAVITY_HD Vector3 getPressure() const;
        GRAVITY_HD Vector3 getPosition() const;
        GRAVITY_HD Vector3 getVelocity() const;
        GRAVITY_HD float getDensity() const;
        GRAVITY_HD float getMass() const;
        GRAVITY_HD float getTemperature() const;

    private:
        Vector3 _position;
        Vector3 _velocity;
        Vector3 _pressure;
        Vector3 _force;
        float _density;
        float _mass;
        float _temperature;
};


