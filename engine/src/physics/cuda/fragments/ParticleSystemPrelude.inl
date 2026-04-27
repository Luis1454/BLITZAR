/*
 * @file engine/src/physics/cuda/fragments/ParticleSystemPrelude.inl
 * @author Luis1454
 * @project BLITZAR
 * @brief Physics and CUDA implementation for the deterministic simulation core.
 */

/*
 * Module: physics/cuda
 * Responsibility: Gather shared CUDA includes and prelude helpers for particle-system fragments.
 */

#include "config/EnvUtils.hpp"
#include "physics/ForceLawPolicy.hpp"
#include "physics/Octree.hpp"
#include "physics/ParticleSystem.hpp"
#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <cuda_runtime.h>
#include <numeric>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <string_view>
#include <utility>

static_assert((sizeof(GpuSystemMetrics) % 64u) == 0u,
              "GpuSystemMetrics must remain cacheline aligned");

/*
 * @brief Defines the sph grid params type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct SphGridParams {
    int gridSize;
    int totalCells;
    float cellSize;
    float originX;
    float originY;
    float originZ;
};

constexpr int kOctreeLeafCapacity = 32;
constexpr int kOctreeMaxDepth = 16;
constexpr float kPi = 3.1415926535f;

#include "physics/ParticleSoAView.hpp"

typedef Particle* ParticleHandle;
typedef const Particle* ParticleConstHandle;
typedef Vector3* Vector3Handle;
typedef const Vector3* Vector3ConstHandle;
typedef float* FloatHandle;
typedef const float* ConstFloatHandle;
typedef GpuOctreeNode* OctreeNodeHandle;
typedef const GpuOctreeNode* OctreeNodeConstHandle;
typedef int* IndexHandle;
typedef const int* IndexConstHandle;

/*
 * @brief Documents the check cuda status operation contract.
 * @param status Input value used by this contract.
 * @param stage Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool checkCudaStatus(cudaError_t status, std::string_view stage)
{
    if (status != cudaSuccess) {
        fprintf(stderr, "[cuda] %.*s failed: %s\n", static_cast<int>(stage.size()), stage.data(),
                cudaGetErrorString(status));
        return false;
    }
    return true;
}

/*
 * @brief Documents the parse bool env operation contract.
 * @param name Input value used by this contract.
 * @param fallback Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool parseBoolEnv(std::string_view name, bool fallback)
{
    constexpr bool kDevProfile = GRAVITY_PROFILE_IS_DEV != 0;
    if (!kDevProfile) {
        (void)name;
        return fallback;
    }
    return grav_env::getBool(name, fallback);
}

/*
 * @brief Documents the parse float env operation contract.
 * @param name Input value used by this contract.
 * @param fallback Input value used by this contract.
 * @return float value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
float parseFloatEnv(std::string_view name, float fallback)
{
    constexpr bool kDevProfile = GRAVITY_PROFILE_IS_DEV != 0;
    if (!kDevProfile) {
        (void)name;
        return fallback;
    }
    float parsed = 0.0f;
    if (!grav_env::getNumber(name, parsed)) {
        return fallback;
    }
    return parsed;
}

/*
 * @brief Documents the solver mode from env operation contract.
 * @param None This contract does not take explicit parameters.
 * @return ParticleSystem::SolverMode value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
ParticleSystem::SolverMode solverModeFromEnv()
{
    constexpr bool kDevProfile = GRAVITY_PROFILE_IS_DEV != 0;
    if (!kDevProfile) {
        return ParticleSystem::SolverMode::PairwiseCuda;
    }
    if (parseBoolEnv("GRAVITY_USE_OCTREE", false)) {
        return ParticleSystem::SolverMode::OctreeGpu;
    }
    const auto solver = grav_env::get("GRAVITY_SOLVER");
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

/*
 * @brief Documents the integrator mode from env operation contract.
 * @param None This contract does not take explicit parameters.
 * @return ParticleSystem::IntegratorMode value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
ParticleSystem::IntegratorMode integratorModeFromEnv()
{
    constexpr bool kDevProfile = GRAVITY_PROFILE_IS_DEV != 0;
    if (!kDevProfile) {
        return ParticleSystem::IntegratorMode::Euler;
    }
    const auto integrator = grav_env::get("GRAVITY_INTEGRATOR");
    if (!integrator.has_value()) {
        return ParticleSystem::IntegratorMode::Euler;
    }
    if (*integrator == "rk4" || *integrator == "RK4") {
        return ParticleSystem::IntegratorMode::Rk4;
    }
    if (*integrator == "leapfrog" || *integrator == "LEAPFROG") {
        return ParticleSystem::IntegratorMode::Leapfrog;
    }
    return ParticleSystem::IntegratorMode::Euler;
}

/*
 * @brief Documents the operator + operation contract.
 * @param a Input value used by this contract.
 * @param b Input value used by this contract.
 * @return Vector3 value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__host__ __device__ Vector3 operator+(Vector3 a, Vector3 b)
{
    return Vector3{a.x + b.x, a.y + b.y, a.z + b.z};
}

/*
 * @brief Documents the operator - operation contract.
 * @param a Input value used by this contract.
 * @param b Input value used by this contract.
 * @return Vector3 value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__host__ __device__ Vector3 operator-(Vector3 a, Vector3 b)
{
    return Vector3{a.x - b.x, a.y - b.y, a.z - b.z};
}

/*
 * @brief Documents the operator * operation contract.
 * @param a Input value used by this contract.
 * @param b Input value used by this contract.
 * @return Vector3 value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__host__ __device__ Vector3 operator*(Vector3 a, float b)
{
    return Vector3{a.x * b, a.y * b, a.z * b};
}

/*
 * @brief Documents the operator / operation contract.
 * @param a Input value used by this contract.
 * @param b Input value used by this contract.
 * @return Vector3 value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__host__ __device__ Vector3 operator/(Vector3 a, float b)
{
    return Vector3{a.x / b, a.y / b, a.z / b};
}

/*
 * @brief Documents the operator * operation contract.
 * @param a Input value used by this contract.
 * @param b Input value used by this contract.
 * @return Vector3 value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__host__ __device__ Vector3 operator*(Vector3 a, Vector3 b)
{
    return Vector3{a.x * b.x, a.y * b.y, a.z * b.z};
}

/*
 * @brief Documents the operator / operation contract.
 * @param a Input value used by this contract.
 * @param b Input value used by this contract.
 * @return Vector3 value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__host__ __device__ Vector3 operator/(Vector3 a, Vector3 b)
{
    return Vector3{a.x / b.x, a.y / b.y, a.z / b.z};
}

__host__ __device__ Vector3& operator+=(Vector3& a, Vector3 b)
{
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    return a;
}

__host__ __device__ Vector3& operator-=(Vector3& a, Vector3 b)
{
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    return a;
}

/*
 * @brief Documents the dot operation contract.
 * @param a Input value used by this contract.
 * @param b Input value used by this contract.
 * @return float value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__host__ __device__ float dot(Vector3 a, Vector3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

/*
 * @brief Documents the particle operation contract.
 * @param None This contract does not take explicit parameters.
 * @return Particle:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__host__ __device__ Particle::Particle()
{
    _position = Vector3{0.0f, 0.0f, 0.0f};
    _velocity = Vector3{0.0f, 0.0f, 0.0f};
    _pressure = Vector3{0.0f, 0.0f, 0.0f};
    _force = Vector3{0.0f, 0.0f, 0.0f};
    _density = 0.0f;
    _mass = Particle::kDefaultMass;
    _temperature = 0.0f;
}

/*
 * @brief Documents the ~particle operation contract.
 * @param None This contract does not take explicit parameters.
 * @return Particle:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__host__ __device__ Particle::~Particle()
{
}

/*
 * @brief Documents the apply force operation contract.
 * @param force Input value used by this contract.
 * @return void Particle:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__host__ __device__ void Particle::applyForce(Vector3 force)
{
    _force += force;
}

/*
 * @brief Documents the get mass operation contract.
 * @param None This contract does not take explicit parameters.
 * @return float Particle:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__device__ __host__ float Particle::getMass() const
{
    return _mass;
}

/*
 * @brief Documents the set mass operation contract.
 * @param mass Input value used by this contract.
 * @return void Particle:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__device__ __host__ void Particle::setMass(float mass)
{
    _mass = mass;
}

/*
 * @brief Documents the get position operation contract.
 * @param None This contract does not take explicit parameters.
 * @return Vector3 Particle:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__device__ __host__ Vector3 Particle::getPosition() const
{
    return _position;
}

/*
 * @brief Documents the set position operation contract.
 * @param position Input value used by this contract.
 * @return void Particle:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__device__ __host__ void Particle::setPosition(Vector3 position)
{
    _position = position;
}

/*
 * @brief Documents the get velocity operation contract.
 * @param None This contract does not take explicit parameters.
 * @return Vector3 Particle:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__device__ __host__ Vector3 Particle::getVelocity() const
{
    return _velocity;
}

/*
 * @brief Documents the set velocity operation contract.
 * @param velocity Input value used by this contract.
 * @return void Particle:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__device__ __host__ void Particle::setVelocity(Vector3 velocity)
{
    _velocity = velocity;
}

/*
 * @brief Documents the get pressure operation contract.
 * @param None This contract does not take explicit parameters.
 * @return Vector3 Particle:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__device__ __host__ Vector3 Particle::getPressure() const
{
    return _pressure;
}

/*
 * @brief Documents the set pressure operation contract.
 * @param pressure Input value used by this contract.
 * @return void Particle:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__device__ __host__ void Particle::setPressure(Vector3 pressure)
{
    _pressure = pressure;
}

/*
 * @brief Documents the set temperature operation contract.
 * @param temperature Input value used by this contract.
 * @return void Particle:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__device__ __host__ void Particle::setTemperature(float temperature)
{
    _temperature = temperature;
}

/*
 * @brief Documents the get density operation contract.
 * @param None This contract does not take explicit parameters.
 * @return float Particle:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__device__ __host__ float Particle::getDensity() const
{
    return _density;
}

/*
 * @brief Documents the set density operation contract.
 * @param density Input value used by this contract.
 * @return void Particle:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__device__ __host__ void Particle::setDensity(float density)
{
    _density = density;
}

/*
 * @brief Documents the get temperature operation contract.
 * @param None This contract does not take explicit parameters.
 * @return float Particle:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__device__ __host__ float Particle::getTemperature() const
{
    return _temperature;
}

/*
 * @brief Documents the move operation contract.
 * @param position Input value used by this contract.
 * @return void Particle:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__device__ __host__ void Particle::move(Vector3 position)
{
    _position += position;
}

/*
 * @brief Documents the bounce operation contract.
 * @param normal Input value used by this contract.
 * @param dt Input value used by this contract.
 * @return void Particle:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__device__ __host__ void Particle::bounce(Vector3 normal, float dt)
{
    _position -= _velocity * dt;
    _velocity -= normal * 2.0f * dot(_velocity, normal);
}

/*
 * @brief Documents the update operation contract.
 * @param deltaTime Input value used by this contract.
 * @return void Particle:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__host__ __device__ void Particle::update(float deltaTime)
{
    _velocity += _force * deltaTime;
    _position += _velocity * deltaTime;
    _force = Vector3{0.0f, 0.0f, 0.0f};
}

/*
 * @brief Documents the norm operation contract.
 * @param None This contract does not take explicit parameters.
 * @return float Vector3:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__host__ __device__ float Vector3::norm() const
{
    return sqrtf(x * x + y * y + z * z);
}

/*
 * @brief Documents the normalize operation contract.
 * @param v Input value used by this contract.
 * @return Vector3 value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__host__ __device__ Vector3 normalize(Vector3 v)
{
    return v / sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

/*
 * @brief Documents the clamp acceleration operation contract.
 * @param accel Input value used by this contract.
 * @param maxAcceleration Input value used by this contract.
 * @return Vector3 value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__host__ __device__ Vector3 clampAcceleration(Vector3 accel, float maxAcceleration)
{
    const float accelNorm = accel.norm();
    if (accelNorm > maxAcceleration && accelNorm > 1e-12f) {
        return accel * (maxAcceleration / accelNorm);
    }
    return accel;
}

/*
 * @brief Documents the clamp softening value operation contract.
 * @param softening Input value used by this contract.
 * @param minSoftening Input value used by this contract.
 * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE float value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
GRAVITY_HD_HOST GRAVITY_HD_DEVICE float clampSofteningValue(float softening, float minSoftening)
{
    return fmaxf(softening, minSoftening);
}

/*
 * @brief Documents the clamp theta value operation contract.
 * @param theta Input value used by this contract.
 * @param minTheta Input value used by this contract.
 * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE float value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
GRAVITY_HD_HOST GRAVITY_HD_DEVICE float clampThetaValue(float theta, float minTheta)
{
    return fmaxf(theta, minTheta);
}

/*
 * @brief Documents the softened distance squared operation contract.
 * @param delta Input value used by this contract.
 * @param policy Input value used by this contract.
 * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE float value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
GRAVITY_HD_HOST GRAVITY_HD_DEVICE float softenedDistanceSquared(Vector3 delta,
                                                                ForceLawPolicy policy)
{
    return dot(delta, delta) + policy.softening * policy.softening;
}

/*
 * @brief Documents the gravity acceleration from source operation contract.
 * @param selfPosition Input value used by this contract.
 * @param sourcePosition Input value used by this contract.
 * @param sourceMass Input value used by this contract.
 * @param policy Input value used by this contract.
 * @return GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
GRAVITY_HD_HOST GRAVITY_HD_DEVICE Vector3 gravityAccelerationFromSource(Vector3 selfPosition,
                                                                        Vector3 sourcePosition,
                                                                        float sourceMass,
                                                                        ForceLawPolicy policy)
{
    const Vector3 delta = sourcePosition - selfPosition;
    const float dist2 = softenedDistanceSquared(delta, policy);
    if (dist2 <= policy.minDistance2) {
        return Vector3(0.0f, 0.0f, 0.0f);
    }
    const float invDist = 1.0f / sqrtf(dist2);
    const float invDist3 = invDist * invDist * invDist;
    return delta * (sourceMass * invDist3);
}

/*
 * @brief Documents the compute pairwise acceleration operation contract.
 * @param state Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @param idx Input value used by this contract.
 * @param policy Input value used by this contract.
 * @param maxAcceleration Input value used by this contract.
 * @return Vector3 value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__host__ __device__ Vector3 computePairwiseAcceleration(ParticleSoAView state, int numParticles,
                                                        int idx, ForceLawPolicy policy,
                                                        float maxAcceleration)
{
    Vector3 force = {0.0f, 0.0f, 0.0f};
    const Vector3 selfPos = getSoAPosition(state, idx);

    for (int i = 0; i < numParticles; ++i) {
        if (i == idx) {
            continue;
        }
        const Vector3 otherPos = getSoAPosition(state, i);
        const float otherMass = state.mass[i];
        force += gravityAccelerationFromSource(selfPos, otherPos, otherMass, policy);
    }
    return clampAcceleration(force, maxAcceleration);
}

/*
 * @brief Documents the sph poly6 operation contract.
 * @param r2 Input value used by this contract.
 * @param h Input value used by this contract.
 * @return float value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
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

/*
 * @brief Documents the sph spiky grad operation contract.
 * @param r Input value used by this contract.
 * @param h Input value used by this contract.
 * @return float value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__device__ float sphSpikyGrad(float r, float h)
{
    if (r <= 1e-6f || r >= h) {
        return 0.0f;
    }
    const float coeff = -45.0f / (kPi * powf(h, 6.0f));
    const float diff = h - r;
    return coeff * diff * diff;
}

/*
 * @brief Documents the sph viscosity laplacian operation contract.
 * @param r Input value used by this contract.
 * @param h Input value used by this contract.
 * @return float value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__device__ float sphViscosityLaplacian(float r, float h)
{
    if (r >= h) {
        return 0.0f;
    }
    const float coeff = 45.0f / (kPi * powf(h, 6.0f));
    return coeff * (h - r);
}

/*
 * @brief Documents the publish metrics kernel operation contract.
 * @param mappedMetrics Input value used by this contract.
 * @param state Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @param stepId Input value used by this contract.
 * @param simTime Input value used by this contract.
 * @param dt Input value used by this contract.
 * @param vramUsedBytes Input value used by this contract.
 * @param vramPeakBytes Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void publishMetricsKernel(GpuSystemMetrics* mappedMetrics, ParticleSoAView state,
                                     int numParticles, std::uint64_t stepId, float simTime,
                                     float dt, std::uint64_t vramUsedBytes,
                                     std::uint64_t vramPeakBytes)
{
    if (blockIdx.x != 0 || threadIdx.x != 0 || mappedMetrics == nullptr) {
        return;
    }

    GpuMetricsPayload payload{};
    payload.flags = static_cast<std::uint32_t>(kGpuMetricsValid | kGpuMetricsEstimated);
    payload.stepId = stepId;
    payload.simTime = simTime;
    payload.dt = dt;
    payload.particleCount = static_cast<std::uint32_t>(numParticles > 0 ? numParticles : 0);
    payload.nanCount = 0u;
    payload.infCount = 0u;
    payload.minSpeed = 0.0f;
    payload.maxSpeed = 0.0f;
    payload.kineticEnergy = 0.0f;
    payload.potentialEnergy = 0.0f;
    payload.totalEnergy = 0.0f;
    payload.vramUsedBytes = vramUsedBytes;
    payload.vramPeakBytes = vramPeakBytes;

    if (numParticles > 0) {
        constexpr int kMaxSamples = 4096;
        const int sampleCount = numParticles < kMaxSamples ? numParticles : kMaxSamples;
        const int stride = sampleCount > 0 ? max(1, numParticles / sampleCount) : 1;
        float minSpeed = FLT_MAX;
        float maxSpeed = 0.0f;
        float kinetic = 0.0f;
        int counted = 0;

        for (int i = 0; i < numParticles && counted < sampleCount; i += stride, ++counted) {
            const float vx = state.velX[i];
            const float vy = state.velY[i];
            const float vz = state.velZ[i];
            const float mass = state.mass[i];
            const float speed2 = vx * vx + vy * vy + vz * vz;
            const float speed = sqrtf(speed2);

            if (!isfinite(speed) || !isfinite(mass)) {
                if (!isfinite(speed)) {
                    payload.infCount += 1u;
                }
                if (isnan(speed) || isnan(mass)) {
                    payload.nanCount += 1u;
                }
                continue;
            }

            minSpeed = fminf(minSpeed, speed);
            maxSpeed = fmaxf(maxSpeed, speed);
            kinetic += 0.5f * mass * speed2;
        }

        if (counted > 0 && sampleCount > 0) {
            const float scale = static_cast<float>(numParticles) / static_cast<float>(sampleCount);
            payload.minSpeed = minSpeed == FLT_MAX ? 0.0f : minSpeed;
            payload.maxSpeed = maxSpeed;
            payload.kineticEnergy = kinetic * scale;
            payload.totalEnergy = payload.kineticEnergy;
        }
    }

    volatile std::uint32_t* sequence = &mappedMetrics->sequence;
    const std::uint32_t observed = *sequence;
    const std::uint32_t evenBase = (observed & 1u) == 0u ? observed : (observed + 1u);
    const std::uint32_t odd = evenBase + 1u;
    const std::uint32_t even = evenBase + 2u;

    *sequence = odd;
    __threadfence_system();

    mappedMetrics->flags = payload.flags;
    mappedMetrics->stepId = payload.stepId;
    mappedMetrics->simTime = payload.simTime;
    mappedMetrics->dt = payload.dt;
    mappedMetrics->particleCount = payload.particleCount;
    mappedMetrics->nanCount = payload.nanCount;
    mappedMetrics->infCount = payload.infCount;
    mappedMetrics->minSpeed = payload.minSpeed;
    mappedMetrics->maxSpeed = payload.maxSpeed;
    mappedMetrics->kineticEnergy = payload.kineticEnergy;
    mappedMetrics->potentialEnergy = payload.potentialEnergy;
    mappedMetrics->totalEnergy = payload.totalEnergy;
    mappedMetrics->vramUsedBytes = payload.vramUsedBytes;
    mappedMetrics->vramPeakBytes = payload.vramPeakBytes;
    mappedMetrics->reserved0 = 0u;
    mappedMetrics->reservedAlignment = 0u;
    mappedMetrics->reserved1 = 0u;
    mappedMetrics->reserved2 = 0u;
    mappedMetrics->reserved3 = 0u;
    mappedMetrics->reserved4 = 0u;
    mappedMetrics->reserved5 = 0u;
    mappedMetrics->reserved6 = 0u;

    __threadfence_system();
    *sequence = even;
}

/*
 * @brief Documents the update particles operation contract.
 * @param last Input value used by this contract.
 * @param current Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @param deltaTime Input value used by this contract.
 * @param policy Input value used by this contract.
 * @param maxAcceleration Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void updateParticles(ParticleSoAView last, ParticleSoAView current, int numParticles,
                                float deltaTime, ForceLawPolicy policy, float maxAcceleration)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < numParticles) {
        const Vector3 pos = getSoAPosition(last, i);
        const Vector3 vel = getSoAVelocity(last, i);
        const Vector3 force =
            computePairwiseAcceleration(last, numParticles, i, policy, maxAcceleration);

        const Vector3 nextVel = vel + force * deltaTime;
        const Vector3 nextPos = pos + nextVel * deltaTime;

        setSoAPressure(current, i, force * 100.0f);
        setSoAVelocity(current, i, nextVel);
        setSoAPosition(current, i, nextPos);
        current.mass[i] = last.mass[i];
        current.temp[i] = last.temp[i];
        current.dens[i] = last.dens[i];
    }
}

/*
 * @brief Documents the compute sph density pressure kernel operation contract.
 * @param particles Input value used by this contract.
 * @param outDensity Input value used by this contract.
 * @param outPressure Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @param smoothingLength Input value used by this contract.
 * @param restDensity Input value used by this contract.
 * @param gasConstant Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void computeSphDensityPressureKernel(ParticleSoAView particles, FloatHandle outDensity,
                                                FloatHandle outPressure, int numParticles,
                                                float smoothingLength, float restDensity,
                                                float gasConstant)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }

    if (particles.mass[i] > 0.1f) {
        outDensity[i] = restDensity;
        outPressure[i] = 0.0f;
        return;
    }

    const Vector3 pi = getSoAPosition(particles, i);
    float density = 0.0f;
    for (int j = 0; j < numParticles; ++j) {
        if (particles.mass[j] > 0.1f) {
            continue;
        }
        const Vector3 pj = getSoAPosition(particles, j);
        const Vector3 d = pi - pj;
        const float r2 = dot(d, d);
        density += particles.mass[j] * sphPoly6(r2, smoothingLength);
    }
    density = fmaxf(density, restDensity * 0.05f);
    outDensity[i] = density;
    const float pressure = gasConstant * fmaxf(density - restDensity, 0.0f);
    outPressure[i] = fminf(pressure, gasConstant * restDensity * 20.0f);
}

__global__ void integrateSphKernel(ParticleSoAView inParticles, ParticleSoAView outParticles,
                                   ConstFloatHandle density, ConstFloatHandle pressure,
                                   int numParticles, float smoothingLength, float viscosity,
                                   float deltaTime, float correctionScale,
                                   IndexConstHandle cellHash, IndexConstHandle sortedIndex,
                                   IndexConstHandle cellStart, IndexConstHandle cellEnd,
                                   SphGridParams grid, float maxAcceleration, float maxSpeed)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }

    const float selfMass = inParticles.mass[i];
    if (selfMass > 0.1f) {
        outParticles.posX[i] = inParticles.posX[i];
        outParticles.posY[i] = inParticles.posY[i];
        outParticles.posZ[i] = inParticles.posZ[i];
        outParticles.velX[i] = inParticles.velX[i];
        outParticles.velY[i] = inParticles.velY[i];
        outParticles.velZ[i] = inParticles.velZ[i];
        outParticles.dens[i] = inParticles.dens[i];
        setSoAPressure(outParticles, i, getSoAPressure(inParticles, i));
        outParticles.mass[i] = inParticles.mass[i];
        return;
    }

    const Vector3 pi = getSoAPosition(inParticles, i);
    const Vector3 vi = getSoAVelocity(inParticles, i);
    const float rhoI = fmaxf(density[i], 1e-6f);
    const float pI = pressure[i];

    Vector3 pressureForce(0.0f, 0.0f, 0.0f);
    Vector3 viscosityForce(0.0f, 0.0f, 0.0f);

    for (int j = 0; j < numParticles; ++j) {
        if (j == i) {
            continue;
        }
        if (inParticles.mass[j] > 0.1f) {
            continue;
        }
        const Vector3 pj = getSoAPosition(inParticles, j);
        const Vector3 rij = pi - pj;
        const float r = sqrtf(dot(rij, rij));
        if (r >= smoothingLength || r <= 1e-6f) {
            continue;
        }

        const float rhoJ = fmaxf(density[j], 1e-6f);
        const float pJ = pressure[j];
        const Vector3 gradDir = rij / r;
        const float grad = sphSpikyGrad(r, smoothingLength);
        pressureForce += gradDir * (-inParticles.mass[j] * (pI + pJ) * 0.5f / rhoJ * grad);

        const float lap = sphViscosityLaplacian(r, smoothingLength);
        viscosityForce +=
            (getSoAVelocity(inParticles, j) - vi) * (viscosity * inParticles.mass[j] / rhoJ * lap);
    }

    const Vector3 totalForce = pressureForce + viscosityForce;
    Vector3 acceleration = totalForce / rhoI;

    const float accelNorm = acceleration.norm();
    if (accelNorm > maxAcceleration) {
        acceleration = acceleration * (maxAcceleration / accelNorm);
    }

    Vector3 velocity = vi + acceleration * (deltaTime * correctionScale);
    const float speed = velocity.norm();
    if (speed > maxSpeed) {
        velocity = velocity * (maxSpeed / speed);
    }
    Vector3 position = pi + velocity * (deltaTime * correctionScale);

    outParticles.velX[i] = velocity.x;
    outParticles.velY[i] = velocity.y;
    outParticles.velZ[i] = velocity.z;
    outParticles.posX[i] = position.x;
    outParticles.posY[i] = position.y;
    outParticles.posZ[i] = position.z;
    outParticles.dens[i] = rhoI;
    setSoAPressure(outParticles, i, (pressureForce + viscosityForce) * 2.0f);
    outParticles.mass[i] = selfMass;
}

/*
 * @brief Documents the copy particles kernel operation contract.
 * @param src Input value used by this contract.
 * @param dst Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void copyParticlesKernel(ParticleConstHandle src, ParticleHandle dst, int numParticles)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }
    dst[i] = src[i];
}

/*
 * @brief Documents the extract velocity kernel operation contract.
 * @param particles Input value used by this contract.
 * @param outVelocity Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void extractVelocityKernel(ParticleSoAView particles, Vector3Handle outVelocity,
                                      int numParticles)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }
    outVelocity[i] = getSoAVelocity(particles, i);
}

/*
 * @brief Documents the compute pairwise acceleration kernel operation contract.
 * @param state Input value used by this contract.
 * @param outAcceleration Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @param policy Input value used by this contract.
 * @param maxAcceleration Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void computePairwiseAccelerationKernel(ParticleSoAView state,
                                                  Vector3Handle outAcceleration, int numParticles,
                                                  ForceLawPolicy policy, float maxAcceleration)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }
    outAcceleration[i] =
        computePairwiseAcceleration(state, numParticles, i, policy, maxAcceleration);
}

/*
 * @brief Documents the build rk4 stage kernel operation contract.
 * @param base Input value used by this contract.
 * @param kPos Input value used by this contract.
 * @param kVel Input value used by this contract.
 * @param dtScale Input value used by this contract.
 * @param stage Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void buildRk4StageKernel(ParticleSoAView base, Vector3ConstHandle kPos,
                                    Vector3ConstHandle kVel, float dtScale, ParticleSoAView stage,
                                    int numParticles)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }

    const Vector3 nextPos = getSoAPosition(base, i) + kPos[i] * dtScale;
    const Vector3 nextVel = getSoAVelocity(base, i) + kVel[i] * dtScale;

    setSoAPosition(stage, i, nextPos);
    setSoAVelocity(stage, i, nextVel);
    stage.mass[i] = base.mass[i];
    stage.temp[i] = base.temp[i];
    stage.dens[i] = base.dens[i];
}

/*
 * @brief Documents the prime half velocity kernel operation contract.
 * @param state Input value used by this contract.
 * @param vHalf Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void primeHalfVelocityKernel(ParticleSoAView state, float3* vHalf, int numParticles)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }

    __shared__ float3 sVel[Particle::kDefaultCudaBlockSize];
    sVel[threadIdx.x] = make_float3(state.velX[i], state.velY[i], state.velZ[i]);
    __syncthreads();

    vHalf[i] = sVel[threadIdx.x];
}

/*
 * @brief Documents the apply kick half step kernel operation contract.
 * @param state Input value used by this contract.
 * @param acceleration Input value used by this contract.
 * @param deltaTime Input value used by this contract.
 * @param vHalf Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void applyKickHalfStepKernel(ParticleSoAView state, Vector3ConstHandle acceleration,
                                        float deltaTime, float3* vHalf, int numParticles)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }

    __shared__ float3 sVel[Particle::kDefaultCudaBlockSize];
    __shared__ float3 sAcc[Particle::kDefaultCudaBlockSize];

    sVel[threadIdx.x] = make_float3(state.velX[i], state.velY[i], state.velZ[i]);
    sAcc[threadIdx.x] = make_float3(acceleration[i].x, acceleration[i].y, acceleration[i].z);
    __syncthreads();

    const float halfDt = 0.5f * deltaTime;
    const float3 vel = sVel[threadIdx.x];
    const float3 acc = sAcc[threadIdx.x];
    vHalf[i] = make_float3(vel.x + acc.x * halfDt, vel.y + acc.y * halfDt, vel.z + acc.z * halfDt);
}

/*
 * @brief Documents the drift with half velocity kernel operation contract.
 * @param state Input value used by this contract.
 * @param vHalf Input value used by this contract.
 * @param deltaTime Input value used by this contract.
 * @param out Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void driftWithHalfVelocityKernel(ParticleSoAView state, const float3* vHalf,
                                            float deltaTime, ParticleSoAView out, int numParticles)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }

    __shared__ float3 sPos[Particle::kDefaultCudaBlockSize];
    __shared__ float3 sHalf[Particle::kDefaultCudaBlockSize];

    sPos[threadIdx.x] = make_float3(state.posX[i], state.posY[i], state.posZ[i]);
    sHalf[threadIdx.x] = vHalf[i];
    __syncthreads();

    const float3 pos = sPos[threadIdx.x];
    const float3 halfVel = sHalf[threadIdx.x];
    const Vector3 nextPos(pos.x + halfVel.x * deltaTime, pos.y + halfVel.y * deltaTime,
                          pos.z + halfVel.z * deltaTime);

    setSoAPosition(out, i, nextPos);
    setSoAVelocity(out, i, getSoAVelocity(state, i));
    out.mass[i] = state.mass[i];
    out.temp[i] = state.temp[i];
    out.dens[i] = state.dens[i];
}

/*
 * @brief Documents the finalize leapfrog kick kernel operation contract.
 * @param driftedState Input value used by this contract.
 * @param vHalf Input value used by this contract.
 * @param acceleration Input value used by this contract.
 * @param deltaTime Input value used by this contract.
 * @param out Input value used by this contract.
 * @param vHalfOut Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void finalizeLeapfrogKickKernel(ParticleSoAView driftedState, const float3* vHalf,
                                           Vector3ConstHandle acceleration, float deltaTime,
                                           ParticleSoAView out, float3* vHalfOut, int numParticles)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }

    __shared__ float3 sHalf[Particle::kDefaultCudaBlockSize];
    __shared__ float3 sAcc[Particle::kDefaultCudaBlockSize];

    sHalf[threadIdx.x] = vHalf[i];
    sAcc[threadIdx.x] = make_float3(acceleration[i].x, acceleration[i].y, acceleration[i].z);
    __syncthreads();

    const float3 halfVel = sHalf[threadIdx.x];
    const float3 acc = sAcc[threadIdx.x];
    const float halfDt = 0.5f * deltaTime;

    const Vector3 nextVel(halfVel.x + acc.x * halfDt, halfVel.y + acc.y * halfDt,
                          halfVel.z + acc.z * halfDt);

    setSoAPosition(out, i, getSoAPosition(driftedState, i));
    setSoAVelocity(out, i, nextVel);
    setSoAPressure(out, i, Vector3(acc.x, acc.y, acc.z) * 100.0f);
    out.mass[i] = driftedState.mass[i];
    out.temp[i] = driftedState.temp[i];
    out.dens[i] = driftedState.dens[i];

    vHalfOut[i] = make_float3(halfVel.x + acc.x * deltaTime, halfVel.y + acc.y * deltaTime,
                              halfVel.z + acc.z * deltaTime);
}

/*
 * @brief Documents the pack so akernel operation contract.
 * @param src Input value used by this contract.
 * @param dst Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void packSoAKernel(ParticleConstHandle src, ParticleSoAView dst, int numParticles)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles)
        return;

    const Particle p = src[i];
    const Vector3 pos = p.getPosition();
    const Vector3 vel = p.getVelocity();
    const Vector3 press = p.getPressure();

    dst.posX[i] = pos.x;
    dst.posY[i] = pos.y;
    dst.posZ[i] = pos.z;
    dst.velX[i] = vel.x;
    dst.velY[i] = vel.y;
    dst.velZ[i] = vel.z;
    dst.pressX[i] = press.x;
    dst.pressY[i] = press.y;
    dst.pressZ[i] = press.z;
    dst.mass[i] = p.getMass();
    dst.temp[i] = p.getTemperature();
    dst.dens[i] = p.getDensity();
}

/*
 * @brief Documents the unpack so akernel operation contract.
 * @param src Input value used by this contract.
 * @param dst Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void unpackSoAKernel(ParticleSoAView src, ParticleHandle dst, int numParticles)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles)
        return;

    Particle p;
    p.setPosition(getSoAPosition(src, i));
    p.setVelocity(getSoAVelocity(src, i));
    p.setPressure(getSoAPressure(src, i));
    p.setMass(src.mass[i]);
    p.setTemperature(src.temp[i]);
    p.setDensity(src.dens[i]);
    dst[i] = p;
}

__global__ void finalizeRk4Kernel(ParticleSoAView base, Vector3ConstHandle k1x,
                                  Vector3ConstHandle k2x, Vector3ConstHandle k3x,
                                  Vector3ConstHandle k4x, Vector3ConstHandle k1v,
                                  Vector3ConstHandle k2v, Vector3ConstHandle k3v,
                                  Vector3ConstHandle k4v, float deltaTime, ParticleSoAView out,
                                  int numParticles)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }

    const Vector3 weightedVel = (k1v[i] + k2v[i] * 2.0f + k3v[i] * 2.0f + k4v[i]) / 6.0f;
    const Vector3 weightedPos = (k1x[i] + k2x[i] * 2.0f + k3x[i] * 2.0f + k4x[i]) / 6.0f;

    const Vector3 nextVel = getSoAVelocity(base, i) + weightedVel * deltaTime;
    const Vector3 nextPos = getSoAPosition(base, i) + weightedPos * deltaTime;

    setSoAVelocity(out, i, nextVel);
    setSoAPosition(out, i, nextPos);
    setSoAPressure(out, i, weightedVel * 100.0f);
    out.mass[i] = base.mass[i];
    out.temp[i] = base.temp[i];
    out.dens[i] = base.dens[i];
}

// Note: Device buffers and management functions moved to ParticleSystem class members/methods.
