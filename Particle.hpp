/*
** EPITECH PROJECT, 2024
** rtxcpp
** File description:
** test
*/

#define MASS 0.01f
#define NUM_PARTICLES 30000
#define BLOCK_SIZE 256
#define DELTA_TIME 0.1f

#include <vector>
#include "Vector.hpp"
#ifdef __CUDACC__
    #include <cuda_runtime.h>
#endif

class Particle {
    public:
        Particle();
        ~Particle();

        #ifdef __CUDACC__
            __host__ __device__ void applyForce(Vector3 force);
            __host__ __device__ void setPosition(Vector3 position);
            __host__ __device__ void setVelocity(Vector3 velocity);
            __host__ __device__ Vector3 getPosition();
            __host__ __device__ float getDensity();
            __host__ __device__ Vector3 getVelocity();
            __host__ __device__ void setPressure(Vector3 pressure);
            __host__ __device__ Vector3 getPressure();
            __host__ __device__ void setDensity(float density);
            __host__ __device__ float getMass();
            __host__ __device__ void setMass(float mass);
            __host__ __device__ void move(Vector3 force);
            __host__ __device__ void bounce(Vector3 normal, float dt);
            __host__ __device__ void update(float deltaTime);
        #else
            void update(float deltaTime);
            void setMass(float mass);
            void move(Vector3 force);
            void bounce(Vector3 normal, float dt);
            void applyForce(Vector3 force);
            void setDensity(float density);
            void setPosition(Vector3 position);
            void setVelocity(Vector3 velocity);
            void setPressure(Vector3 pressure);
            Vector3 getPressure();
            Vector3 getPosition();
            Vector3 getVelocity();
            float getDensity();
            float getMass();
        #endif

    private:
        Vector3 _position;
        Vector3 _velocity;
        Vector3 _pressure;
        Vector3 _force;
        float _density;
        float _mass;
};
