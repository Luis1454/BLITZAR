#include <cuda_runtime.h>
#include "Octree.hpp"
#include "ParticleSystem.hpp"
#include <stdio.h>

#define BLOCK_SIZE 256
#define PI 3.1415926535f

__host__ __device__ Vector3 operator+(Vector3 a, Vector3 b) {
    return (Vector3){a.x + b.x, a.y + b.y, a.z + b.z};
}

__host__ __device__ Vector3 operator-(Vector3 a, Vector3 b) {
    return (Vector3){a.x - b.x, a.y - b.y, a.z - b.z};
}

__host__ __device__ Vector3 operator*(Vector3 a, float b) {
    return (Vector3){a.x * b, a.y * b, a.z * b};
}

__host__ __device__ Vector3 operator/(Vector3 a, float b) {
    return (Vector3){a.x / b, a.y / b, a.z / b};
}

__host__ __device__ Vector3 operator*(Vector3 a, Vector3 b) {
    return (Vector3){a.x * b.x, a.y * b.y, a.z * b.z};
}

__host__ __device__ Vector3 operator/(Vector3 a, Vector3 b) {
    return (Vector3){a.x / b.x, a.y / b.y, a.z / b.z};
}

__host__ __device__ Vector3 operator+=(Vector3 &a, Vector3 b) {
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    return a;
}

__host__ __device__ Vector3 operator-=(Vector3 &a, Vector3 b) {
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    return a;
}

__host__ __device__ float dot(Vector3 a, Vector3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

__device__ __host__ float Particle::getMass() {
    return _mass;
}

__device__ __host__ void Particle::setMass(float mass) {
    _mass = mass;
}

__device__ __host__ Vector3 Particle::getPosition() {
    return _position;
}

__device__ __host__ void Particle::setPosition(Vector3 position) {
    _position = position;
}

__device__ __host__ Vector3 Particle::getVelocity() {
    return _velocity;
}

__device__ __host__ void Particle::setVelocity(Vector3 velocity) {
    _velocity = velocity;
}

__device__ __host__ Vector3 Particle::getPressure() {
    return _pressure;
}

__device__ __host__ void Particle::setPressure(Vector3 pressure) {
    _pressure = pressure;
}

__device__ __host__ float Particle::getDensity() {
    return _density;
}

__device__ __host__ void Particle::setDensity(float density) {
    _density = density;
}

__device__ __host__ void Particle::move(Vector3 position) {
    _position += position;
}

__device__ __host__ void Particle::bounce(Vector3 normal, float dt) {
    _position -= _velocity * dt;
    _velocity -= normal * 2.0f * dot(_velocity, normal);
}

__host__ __device__ float Vector3::norm() {
    return sqrtf(x * x + y * y + z * z);
}

__host__ __device__ Vector3 normalize(Vector3 v) {
    return v / sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

__host__ __device__ Vector3 ParticleSystem::getForce(Particle *last, Particle *particles, int numParticles, int idx, float dt) {
    Vector3 force = {0.0f, 0.0f, 0.0f};
    Particle& p = particles[idx];

    for (int i = 0; i < numParticles; ++i) {
        if (i == idx) continue;
        Particle &q = particles[i];
        Vector3 r = p.getPosition() - q.getPosition();
        float dist = r.norm();
        if (dist < 1.0f) continue;

        Vector3 f = normalize(r) * q.getMass() / (dist * dist);

        if (dist <= 1.05f)
            // 
        else        
            force -= f;
        // if (dist <= 1.05f) {
        //     // calculate the force of the collision
        //     Vector3 normal = normalize(r);
        //     Vector3 relativeVelocity = p.getVelocity() - q.getVelocity();
        //     float relativeSpeed = dot(relativeVelocity, normal);
        //     if (relativeSpeed < 0) {
        //         // printf("Collision\n");
        //         float e = 0.5f;
        //         float j = -(1 + e) * relativeSpeed / (1 / p.getMass() + 1 / q.getMass());
        //         Vector3 impulse = normal * j;
        //         p.setVelocity(p.getVelocity() + impulse / p.getMass());
        //         q.setVelocity(q.getVelocity() - impulse / q.getMass());
        //         force += impulse;
        //     }
        // }
    }
    p.setPressure(force * 100.0f);
    return force;
}

__global__ void updateParticles(Particle *last, Particle *particles, int numParticles, float deltaTime) {
    // test with a simple gravity
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < numParticles) {
        Vector3 force = ParticleSystem::getForce(last, particles, numParticles, i, deltaTime);
        particles[i].setVelocity((particles[i].getVelocity() + force * deltaTime));
        particles[i].setPosition(particles[i].getPosition() + particles[i].getVelocity() * deltaTime);
        // printf("speed: %f\n", particles[i].getVelocity().norm());

        // print particle pos
        // boundary check (100x100 domain between -50 and 50)
        // if (particles[i].getPosition().x < -50.0f) {
        //     particles[i].bounce((Vector3){1.0f, 0.0f, 0.0f}, deltaTime);
        // }
        // if (particles[i].getPosition().x > 50.0f) {
        //     particles[i].bounce((Vector3){-1.0f, 0.0f, 0.0f}, deltaTime);
        // }
        // if (particles[i].getPosition().y < -50.0f) {
        //     particles[i].bounce((Vector3){0.0f, 0.8f, 0.0f}, deltaTime);
        // }
        // if (particles[i].getPosition().y > 50.0f) {
        //     particles[i].bounce((Vector3){0.0f, -1.0f, 0.0f}, deltaTime);
        // }
        // if (particles[i].getPosition().z < -50.0f) {
        //     particles[i].bounce((Vector3){0.0f, 0.0f, 1.0f}, deltaTime);
        // }
        // if (particles[i].getPosition().z > 50.0f) {
        //     particles[i].bounce((Vector3){0.0f, 0.0f, -1.0f}, deltaTime);
        // }
    }
}

Particle *d_particles = nullptr;
Particle *last = nullptr;

ParticleSystem::ParticleSystem(int numParticles) {
    Particle p;
    float d = 100;
    p.setVelocity((Vector3){0, 0, 0});
    float massTerre = 1.0f;
    float diskMass = 0.75f * massTerre;
    p.setMass(massTerre);
    p.setPosition((Vector3){0, 0, 0});
    _particles.push_back(p);
    // p.setPosition((Vector3){d, d, 0});
    // _particles.push_back(p);
    // p.setPosition((Vector3){-d, -d, 0});
    // _particles.push_back(p);
    for (int i = 1; i < numParticles; ++i) {
        p.setPosition((Vector3){
            random() / (float)RAND_MAX * 10.0f + 1.5f,
            random() / (float)RAND_MAX * 10.0f + 1.5f,
            0.0f
        });
        // rotate the position to have a circular orbit
        double angle = random() / (float)RAND_MAX * 2.0f * PI;
        p.setPosition((Vector3){
            p.getPosition().x * cos(angle) - p.getPosition().y * sin(angle),
            p.getPosition().x * sin(angle) + p.getPosition().y * cos(angle),
            0.0f
        });
        p.setMass(diskMass / numParticles);
        float orbitalSpeed = sqrtf((massTerre + p.getMass() * numParticles / 2.0f) / p.getPosition().norm());
        printf("mass: %f | dist : %f | speed: %f\n", massTerre, p.getPosition().norm(), orbitalSpeed);
        printf("Orbital speed: %f\n", orbitalSpeed);
        // tacke account of the mass of the particle in the orbital speed
        // if (random() % 2 == 0) { 
        //     p.setPosition(p.getPosition() * -10.0f);
        // }
        p.setVelocity(Vector3{
            p.getPosition().y * orbitalSpeed / p.getPosition().norm(),
            -p.getPosition().x * orbitalSpeed / p.getPosition().norm(),
            0.0f
        });
        // if (random() % 2)
        //     p.setPosition(p.getPosition() + (Vector3){-d, -d, 0});
        // else
        //     p.setPosition(p.getPosition() + (Vector3){d, d, 0});
        _particles.push_back(p);
    }
    // for (int i = -sqrt(numParticles) / 2; i < sqrt(numParticles) / 2; i++)
    //     for (int j = -sqrt(numParticles) / 2; j < sqrt(numParticles) / 2; j++) {
    //         p.setPosition((Vector3){
    //             i * 2.0f,
    //             j * 2.0f,
    //             0
    //         });
    //         p.setVelocity((Vector3){0, 0, 0});
    //         p.setMass(MASS);
    //         _particles.push_back(p);
    //     }
    cudaMalloc(&d_particles, numParticles * sizeof(Particle));
    cudaMalloc(&last, numParticles * sizeof(Particle));
    cudaMemcpy(d_particles, _particles.data(), numParticles * sizeof(Particle), cudaMemcpyHostToDevice);
}

ParticleSystem::~ParticleSystem() {
    cudaFree(d_particles);
}

std::vector<Particle> &ParticleSystem::getParticles() {
    return _particles;
}

void Octree::setSize(float size) {
    _size = size;
}

float Octree::getSize() {
    return _size;
}

Octree::Octree() {
    _size = 0.0f;
}

Octree::~Octree() {
}

void Octree::insert(Particle particle) {
    _particles.push_back(particle);
}

Octree ParticleSystem::buildOctree(Particle *particles, int numParticles) {
    Octree octree = Octree();

    octree.setSize(100.0f);
    for (int i = 0; i < numParticles; ++i)
        octree.insert(particles[i]);
    return octree;
}

void ParticleSystem::update(float deltaTime) {
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    cudaEventRecord(start);

    cudaMemcpy(last, _particles.data(), _particles.size() * sizeof(Particle), cudaMemcpyHostToDevice);
    cudaDeviceSynchronize();

    // // Mise à jour des densités
    // computeDensity<<<(_particles.size() + BLOCK_SIZE - 1) / BLOCK_SIZE, BLOCK_SIZE>>>(d_particles, _particles.size());
    // cudaDeviceSynchronize();

    // // Mise à jour des pressions
    // computePressure<<<(_particles.size() + BLOCK_SIZE - 1) / BLOCK_SIZE, BLOCK_SIZE>>>(d_particles, _particles.size(), 1.0f, 100000.0f);
    // cudaDeviceSynchronize();

    // Mise à jour des positions des particules
    updateParticles<<<(_particles.size() + BLOCK_SIZE - 1) / BLOCK_SIZE, BLOCK_SIZE>>>(last, d_particles, _particles.size(), deltaTime);
    cudaDeviceSynchronize();

    cudaMemcpy(_particles.data(), d_particles, _particles.size() * sizeof(Particle), cudaMemcpyDeviceToHost);

    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    float milliseconds = 0;
    cudaEventElapsedTime(&milliseconds, start, stop);
    printf("Time elapsed: %f ms (%f fps) for computing %i particles\n", milliseconds, 1000.0f / milliseconds, _particles.size());
}


void destroyParticles(Particle *particles) {
    cudaFree(d_particles);
    cudaFree(last);

    free(particles);
}
