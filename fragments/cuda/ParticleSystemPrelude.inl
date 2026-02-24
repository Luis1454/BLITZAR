#include <cuda_runtime.h>
#include "core/Octree.hpp"
#include "core/ParticleSystem.hpp"
#include "sim/EnvUtils.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <numeric>
#include <string_view>
#include <utility>
#include <stdio.h>

#define BLOCK_SIZE 256
#define PI 3.1415926535f

namespace {
constexpr int kOctreeLeafCapacity = 32;
constexpr int kOctreeMaxDepth = 16;
constexpr float kPi = 3.1415926535f;
constexpr float kGravityMaxAcceleration = 64.0f;
constexpr float kGravityMinSoftening = 1e-4f;
constexpr float kGravityMinDistance2 = 1e-12f;
constexpr float kGravityMinTheta = 0.05f;
using ParticleHandle = Particle *;
using ParticleConstHandle = const Particle *;
using Vector3Handle = Vector3 *;
using Vector3ConstHandle = const Vector3 *;
using FloatHandle = float *;
using ConstFloatHandle = const float *;
using OctreeNodeHandle = GpuOctreeNode *;
using OctreeNodeConstHandle = const GpuOctreeNode *;
using IndexHandle = int *;
using IndexConstHandle = const int *;

bool checkCudaStatus(cudaError_t status, std::string_view stage)
{
    if (status != cudaSuccess) {
        fprintf(
            stderr,
            "[cuda] %.*s failed: %s\n",
            static_cast<int>(stage.size()),
            stage.data(),
            cudaGetErrorString(status));
        return false;
    }
    return true;
}

bool parseBoolEnv(std::string_view name, bool fallback)
{
    return sim::env::getBool(name, fallback);
}

float parseFloatEnv(std::string_view name, float fallback)
{
    float parsed = 0.0f;
    if (!sim::env::getNumber(name, parsed)) {
        return fallback;
    }
    return parsed;
}

ParticleSystem::SolverMode solverModeFromEnv()
{
    if (parseBoolEnv("GRAVITY_USE_OCTREE", false)) {
        return ParticleSystem::SolverMode::OctreeGpu;
    }
    const auto solver = sim::env::get("GRAVITY_SOLVER");
    if (!solver.has_value()) {
        return ParticleSystem::SolverMode::PairwiseCuda;
    }
    if (*solver == "octree" || *solver == "octree_cpu") {
        return ParticleSystem::SolverMode::OctreeCpu;
    }
    if (*solver == "octree_gpu") {
        return ParticleSystem::SolverMode::OctreeGpu;
    }
    return ParticleSystem::SolverMode::PairwiseCuda;
}

ParticleSystem::IntegratorMode integratorModeFromEnv()
{
    const auto integrator = sim::env::get("GRAVITY_INTEGRATOR");
    if (!integrator.has_value()) {
        return ParticleSystem::IntegratorMode::Euler;
    }
    if (*integrator == "rk4" || *integrator == "RK4") {
        return ParticleSystem::IntegratorMode::Rk4;
    }
    return ParticleSystem::IntegratorMode::Euler;
}
}

__host__ __device__ Vector3 operator+(Vector3 a, Vector3 b) {
    return Vector3{a.x + b.x, a.y + b.y, a.z + b.z};
}

__host__ __device__ Vector3 operator-(Vector3 a, Vector3 b) {
    return Vector3{a.x - b.x, a.y - b.y, a.z - b.z};
}

__host__ __device__ Vector3 operator*(Vector3 a, float b) {
    return Vector3{a.x * b, a.y * b, a.z * b};
}

__host__ __device__ Vector3 operator/(Vector3 a, float b) {
    return Vector3{a.x / b, a.y / b, a.z / b};
}

__host__ __device__ Vector3 operator*(Vector3 a, Vector3 b) {
    return Vector3{a.x * b.x, a.y * b.y, a.z * b.z};
}

__host__ __device__ Vector3 operator/(Vector3 a, Vector3 b) {
    return Vector3{a.x / b.x, a.y / b.y, a.z / b.z};
}

__host__ __device__ Vector3 &operator+=(Vector3 &a, Vector3 b) {
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    return a;
}

__host__ __device__ Vector3 &operator-=(Vector3 &a, Vector3 b) {
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    return a;
}

__host__ __device__ float dot(Vector3 a, Vector3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Particle::Particle() {
    _position = Vector3{0.0f, 0.0f, 0.0f};
    _velocity = Vector3{0.0f, 0.0f, 0.0f};
    _pressure = Vector3{0.0f, 0.0f, 0.0f};
    _force = Vector3{0.0f, 0.0f, 0.0f};
    _density = 0.0f;
    _mass = MASS;
    _temperature = 0.0f;
}

Particle::~Particle() {
}

__host__ __device__ void Particle::applyForce(Vector3 force) {
    _force += force;
}

__device__ __host__ float Particle::getMass() const {
    return _mass;
}

__device__ __host__ void Particle::setMass(float mass) {
    _mass = mass;
}

__device__ __host__ Vector3 Particle::getPosition() const {
    return _position;
}

__device__ __host__ void Particle::setPosition(Vector3 position) {
    _position = position;
}

__device__ __host__ Vector3 Particle::getVelocity() const {
    return _velocity;
}

__device__ __host__ void Particle::setVelocity(Vector3 velocity) {
    _velocity = velocity;
}

__device__ __host__ Vector3 Particle::getPressure() const {
    return _pressure;
}

__device__ __host__ void Particle::setPressure(Vector3 pressure) {
    _pressure = pressure;
}

__device__ __host__ void Particle::setTemperature(float temperature) {
    _temperature = temperature;
}

__device__ __host__ float Particle::getDensity() const {
    return _density;
}

__device__ __host__ void Particle::setDensity(float density) {
    _density = density;
}

__device__ __host__ float Particle::getTemperature() const {
    return _temperature;
}

__device__ __host__ void Particle::move(Vector3 position) {
    _position += position;
}

__device__ __host__ void Particle::bounce(Vector3 normal, float dt) {
    _position -= _velocity * dt;
    _velocity -= normal * 2.0f * dot(_velocity, normal);
}

__host__ __device__ void Particle::update(float deltaTime) {
    _velocity += _force * deltaTime;
    _position += _velocity * deltaTime;
    _force = Vector3{0.0f, 0.0f, 0.0f};
}

__host__ __device__ float Vector3::norm() const {
    return sqrtf(x * x + y * y + z * z);
}

__host__ __device__ Vector3 normalize(Vector3 v) {
    return v / sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

__host__ __device__ Vector3 clampAcceleration(Vector3 accel, float maxAcceleration)
{
    const float accelNorm = accel.norm();
    if (accelNorm > maxAcceleration && accelNorm > 1e-12f) {
        return accel * (maxAcceleration / accelNorm);
    }
    return accel;
}

GRAVITY_HD float clampSofteningValue(float softening)
{
    return fmaxf(softening, kGravityMinSoftening);
}

GRAVITY_HD float clampThetaValue(float theta)
{
    return fmaxf(theta, kGravityMinTheta);
}

GRAVITY_HD float softenedDistanceSquared(Vector3 delta, float softening)
{
    const float safeSoftening = clampSofteningValue(softening);
    return dot(delta, delta) + safeSoftening * safeSoftening;
}

GRAVITY_HD Vector3 gravityAccelerationFromSource(
    Vector3 selfPosition,
    Vector3 sourcePosition,
    float sourceMass,
    float softening
)
{
    const Vector3 delta = sourcePosition - selfPosition;
    const float dist2 = softenedDistanceSquared(delta, softening);
    if (dist2 <= kGravityMinDistance2) {
        return Vector3(0.0f, 0.0f, 0.0f);
    }
    const float invDist = 1.0f / sqrtf(dist2);
    const float invDist3 = invDist * invDist * invDist;
    return delta * (sourceMass * invDist3);
}

__host__ __device__ Vector3 computePairwiseAcceleration(
    ParticleConstHandle state,
    int numParticles,
    int idx,
    float softening,
    float maxAcceleration
)
{
    Vector3 force = {0.0f, 0.0f, 0.0f};
    const Vector3 selfPos = state[idx].getPosition();

    for (int i = 0; i < numParticles; ++i) {
        if (i == idx) {
            continue;
        }
        const Particle &q = state[i];
        force += gravityAccelerationFromSource(
            selfPos,
            q.getPosition(),
            q.getMass(),
            softening);
    }
    return clampAcceleration(force, maxAcceleration);
}

__device__ float sphPoly6(float r2, float h)
{
    const float h2 = h * h;
    if (r2 >= h2) {
        return 0.0f;
    }
    const float diff = h2 - r2;
    const float coeff = 315.0f / (64.0f * kPi * powf(h, 9.0f));
    return coeff * diff * diff * diff;
}

__device__ float sphSpikyGrad(float r, float h)
{
    if (r <= 1e-6f || r >= h) {
        return 0.0f;
    }
    const float coeff = -45.0f / (kPi * powf(h, 6.0f));
    const float diff = h - r;
    return coeff * diff * diff;
}

__device__ float sphViscosityLaplacian(float r, float h)
{
    if (r >= h) {
        return 0.0f;
    }
    const float coeff = 45.0f / (kPi * powf(h, 6.0f));
    return coeff * (h - r);
}

__global__ void updateParticles(
    ParticleHandle last,
    ParticleHandle particles,
    int numParticles,
    float deltaTime,
    float softening,
    float maxAcceleration
) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < numParticles) {
        const Particle base = last[i];
        const Vector3 force = computePairwiseAcceleration(last, numParticles, i, softening, maxAcceleration);

        Particle updated = base;
        updated.setPressure(force * 100.0f);
        updated.setVelocity(base.getVelocity() + force * deltaTime);
        updated.setPosition(base.getPosition() + updated.getVelocity() * deltaTime);
        particles[i] = updated;
    }
}

__global__ void computeSphDensityPressureKernel(
    ParticleConstHandle particles,
    FloatHandle outDensity,
    FloatHandle outPressure,
    int numParticles,
    float smoothingLength,
    float restDensity,
    float gasConstant
) {
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }

    if (particles[i].getMass() > 0.1f) {
        outDensity[i] = restDensity;
        outPressure[i] = 0.0f;
        return;
    }

    const Vector3 pi = particles[i].getPosition();
    float density = 0.0f;
    for (int j = 0; j < numParticles; ++j) {
        const Particle pj = particles[j];
        if (pj.getMass() > 0.1f) {
            continue;
        }
        const Vector3 d = pi - pj.getPosition();
        const float r2 = dot(d, d);
        density += pj.getMass() * sphPoly6(r2, smoothingLength);
    }
    density = fmaxf(density, restDensity * 0.05f);
    outDensity[i] = density;
    const float pressure = gasConstant * fmaxf(density - restDensity, 0.0f);
    outPressure[i] = fminf(pressure, gasConstant * restDensity * 20.0f);
}

__global__ void integrateSphKernel(
    ParticleHandle particles,
    ConstFloatHandle density,
    ConstFloatHandle pressure,
    int numParticles,
    float smoothingLength,
    float viscosity,
    float deltaTime,
    float correctionScale
) {
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }

    const Particle self = particles[i];
    const Vector3 pi = self.getPosition();
    const Vector3 vi = self.getVelocity();
    const float selfMass = self.getMass();
    if (selfMass > 0.1f) {
        return;
    }

    const float rhoI = fmaxf(density[i], 1e-6f);
    const float pI = pressure[i];

    Vector3 pressureForce(0.0f, 0.0f, 0.0f);
    Vector3 viscosityForce(0.0f, 0.0f, 0.0f);

    for (int j = 0; j < numParticles; ++j) {
        if (j == i) {
            continue;
        }
        const Particle other = particles[j];
        if (other.getMass() > 0.1f) {
            continue;
        }
        const Vector3 rij = pi - other.getPosition();
        const float r = sqrtf(dot(rij, rij));
        if (r >= smoothingLength || r <= 1e-6f) {
            continue;
        }

        const float rhoJ = fmaxf(density[j], 1e-6f);
        const float pJ = pressure[j];
        const Vector3 gradDir = rij / r;
        const float grad = sphSpikyGrad(r, smoothingLength);
        pressureForce += gradDir * (-other.getMass() * (pI + pJ) * 0.5f / rhoJ * grad);

        const float lap = sphViscosityLaplacian(r, smoothingLength);
        viscosityForce += (other.getVelocity() - vi) * (viscosity * other.getMass() / rhoJ * lap);
    }

    const Vector3 totalForce = pressureForce + viscosityForce;
    Vector3 acceleration = totalForce / rhoI;

    const float accelNorm = acceleration.norm();
    const float maxAccel = 40.0f;
    if (accelNorm > maxAccel) {
        acceleration = acceleration * (maxAccel / accelNorm);
    }

    Particle updated = self;
    Vector3 velocity = vi + acceleration * (deltaTime * correctionScale);
    const float speed = velocity.norm();
    const float maxSpeed = 120.0f;
    if (speed > maxSpeed) {
        velocity = velocity * (maxSpeed / speed);
    }
    Vector3 position = pi + velocity * (deltaTime * correctionScale);

    updated.setVelocity(velocity);
    updated.setPosition(position);
    updated.setDensity(rhoI);
    updated.setPressure((pressureForce + viscosityForce) * 2.0f);
    particles[i] = updated;
}

__global__ void copyParticlesKernel(ParticleConstHandle src, ParticleHandle dst, int numParticles)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }
    dst[i] = src[i];
}

__global__ void extractVelocityKernel(ParticleConstHandle particles, Vector3Handle outVelocity, int numParticles)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }
    outVelocity[i] = particles[i].getVelocity();
}

__global__ void computePairwiseAccelerationKernel(
    ParticleConstHandle state,
    Vector3Handle outAcceleration,
    int numParticles,
    float softening,
    float maxAcceleration
)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }
    outAcceleration[i] = computePairwiseAcceleration(state, numParticles, i, softening, maxAcceleration);
}

__global__ void buildRk4StageKernel(
    ParticleConstHandle base,
    Vector3ConstHandle kPos,
    Vector3ConstHandle kVel,
    float dtScale,
    ParticleHandle stage,
    int numParticles
) {
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }

    Particle p = base[i];
    p.setPosition(base[i].getPosition() + kPos[i] * dtScale);
    p.setVelocity(base[i].getVelocity() + kVel[i] * dtScale);
    stage[i] = p;
}

__global__ void finalizeRk4Kernel(
    ParticleConstHandle base,
    Vector3ConstHandle k1x,
    Vector3ConstHandle k2x,
    Vector3ConstHandle k3x,
    Vector3ConstHandle k4x,
    Vector3ConstHandle k1v,
    Vector3ConstHandle k2v,
    Vector3ConstHandle k3v,
    Vector3ConstHandle k4v,
    float deltaTime,
    ParticleHandle out,
    int numParticles
) {
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }

    const Particle baseParticle = base[i];
    const Vector3 weightedVel = (k1v[i] + k2v[i] * 2.0f + k3v[i] * 2.0f + k4v[i]) / 6.0f;
    const Vector3 weightedPos = (k1x[i] + k2x[i] * 2.0f + k3x[i] * 2.0f + k4x[i]) / 6.0f;

    Particle updated = baseParticle;
    updated.setVelocity(baseParticle.getVelocity() + weightedVel * deltaTime);
    updated.setPosition(baseParticle.getPosition() + weightedPos * deltaTime);
    updated.setPressure(weightedVel * 100.0f);
    out[i] = updated;
}

namespace {
ParticleHandle d_particles = nullptr;
ParticleHandle last = nullptr;
ParticleHandle d_stage = nullptr;
Vector3Handle d_k1x = nullptr;
Vector3Handle d_k2x = nullptr;
Vector3Handle d_k3x = nullptr;
Vector3Handle d_k4x = nullptr;
Vector3Handle d_k1v = nullptr;
Vector3Handle d_k2v = nullptr;
Vector3Handle d_k3v = nullptr;
Vector3Handle d_k4v = nullptr;
FloatHandle d_sphDensity = nullptr;
FloatHandle d_sphPressure = nullptr;
OctreeNodeHandle g_dOctreeNodes = nullptr;
IndexHandle g_dOctreeLeafIndices = nullptr;
std::size_t g_dOctreeNodeCapacity = 0;
std::size_t g_dOctreeLeafCapacity = 0;
}

void releaseRk4Buffers()
{
    if (d_stage) {
        cudaFree(d_stage);
        d_stage = nullptr;
    }
    if (d_k1x) {
        cudaFree(d_k1x);
        d_k1x = nullptr;
    }
    if (d_k2x) {
        cudaFree(d_k2x);
        d_k2x = nullptr;
    }
    if (d_k3x) {
        cudaFree(d_k3x);
        d_k3x = nullptr;
    }
    if (d_k4x) {
        cudaFree(d_k4x);
        d_k4x = nullptr;
    }
    if (d_k1v) {
        cudaFree(d_k1v);
        d_k1v = nullptr;
    }
    if (d_k2v) {
        cudaFree(d_k2v);
        d_k2v = nullptr;
    }
    if (d_k3v) {
        cudaFree(d_k3v);
        d_k3v = nullptr;
    }
    if (d_k4v) {
        cudaFree(d_k4v);
        d_k4v = nullptr;
    }
}

void releaseSphBuffers()
{
    if (d_sphDensity) {
        cudaFree(d_sphDensity);
        d_sphDensity = nullptr;
    }
    if (d_sphPressure) {
        cudaFree(d_sphPressure);
        d_sphPressure = nullptr;
    }
}

bool allocateRk4Buffers(int numParticles)
{
    releaseRk4Buffers();
    if (!checkCudaStatus(cudaMalloc(&d_stage, numParticles * sizeof(Particle)), "cudaMalloc(d_stage)")) {
        releaseRk4Buffers();
        return false;
    }
    if (!checkCudaStatus(cudaMalloc(&d_k1x, numParticles * sizeof(Vector3)), "cudaMalloc(d_k1x)")) {
        releaseRk4Buffers();
        return false;
    }
    if (!checkCudaStatus(cudaMalloc(&d_k2x, numParticles * sizeof(Vector3)), "cudaMalloc(d_k2x)")) {
        releaseRk4Buffers();
        return false;
    }
    if (!checkCudaStatus(cudaMalloc(&d_k3x, numParticles * sizeof(Vector3)), "cudaMalloc(d_k3x)")) {
        releaseRk4Buffers();
        return false;
    }
    if (!checkCudaStatus(cudaMalloc(&d_k4x, numParticles * sizeof(Vector3)), "cudaMalloc(d_k4x)")) {
        releaseRk4Buffers();
        return false;
    }
    if (!checkCudaStatus(cudaMalloc(&d_k1v, numParticles * sizeof(Vector3)), "cudaMalloc(d_k1v)")) {
        releaseRk4Buffers();
        return false;
    }
    if (!checkCudaStatus(cudaMalloc(&d_k2v, numParticles * sizeof(Vector3)), "cudaMalloc(d_k2v)")) {
        releaseRk4Buffers();
        return false;
    }
    if (!checkCudaStatus(cudaMalloc(&d_k3v, numParticles * sizeof(Vector3)), "cudaMalloc(d_k3v)")) {
        releaseRk4Buffers();
        return false;
    }
    if (!checkCudaStatus(cudaMalloc(&d_k4v, numParticles * sizeof(Vector3)), "cudaMalloc(d_k4v)")) {
        releaseRk4Buffers();
        return false;
    }
    return true;
}

bool allocateSphBuffers(int numParticles)
{
    releaseSphBuffers();
    if (!checkCudaStatus(cudaMalloc(&d_sphDensity, numParticles * sizeof(float)), "cudaMalloc(d_sphDensity)")) {
        releaseSphBuffers();
        return false;
    }
    if (!checkCudaStatus(cudaMalloc(&d_sphPressure, numParticles * sizeof(float)), "cudaMalloc(d_sphPressure)")) {
        releaseSphBuffers();
        return false;
    }
    return true;
}
