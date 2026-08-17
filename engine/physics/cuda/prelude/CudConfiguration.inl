/*
 * @file engine/physics/cuda/prelude/CudConfiguration.inl
 * @project BLITZAR
 * @brief Shared CUDA system helper or kernel implementation fragment.
 */

/*
 * @file engine/physics/cuda/prelude/CudConfiguration.inl
 * @author Luis1454
 * @project BLITZAR
 * @brief Physics and CUDA implementation for the deterministic simulation core.
 */

/*
 * Module: cuda
 * Responsibility: Gather shared CUDA includes and prelude helpers for particle-system fragments.
 */

#include "core/constants/FndConstants.hpp"
#include "config/env/platform/CfgBase.hpp"
#include "physics/core/force/PhyForceLawPolicy.hpp"
#include "physics/core/particle/PhyParticleSystem.hpp"
#include "physics/octree/model/Octree.hpp"
#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <cuda_runtime.h>
#include <cufft.h>
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

struct TreePmGridParams {
    int gridSize;
    int totalCells;
    int assignment;
    int periodic;
    float cellSize;
    float invCellSize;
    float originX;
    float originY;
    float originZ;
    float boundaryMass;
    float boundaryCenterX;
    float boundaryCenterY;
    float boundaryCenterZ;
    float boundarySoftening;
    float shortRangeScale;
    float densityScale;
    float poissonCoefficient;
};

constexpr int kOctreeLeafCapacity = 32;
constexpr int kOctreeMaxDepth = 16;

#include "physics/core/particle/PhyParticleSoAView.hpp"

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
    constexpr bool kDevProfile = BLITZAR_PROFILE_IS_DEV != 0;
    if (!kDevProfile) {
        (void)name;
        return fallback;
    }
    return bltzr_env::getBool(name, fallback);
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
    constexpr bool kDevProfile = BLITZAR_PROFILE_IS_DEV != 0;
    if (!kDevProfile) {
        (void)name;
        return fallback;
    }
    float parsed = 0.0f;
    if (!bltzr_env::getNumber(name, parsed)) {
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
    constexpr bool kDevProfile = BLITZAR_PROFILE_IS_DEV != 0;
    if (!kDevProfile) {
        return ParticleSystem::SolverMode::PairwiseCuda;
    }
    if (parseBoolEnv("BLITZAR_USE_OCTREE", false)) {
        return ParticleSystem::SolverMode::OctreeGpu;
    }
    const auto solver = bltzr_env::get("BLITZAR_SOLVER");
    if (!solver.has_value()) {
        return ParticleSystem::SolverMode::PairwiseCuda;
    }
    if (*solver == "octree" || *solver == "octree_cpu") {
        return ParticleSystem::SolverMode::OctreeCpu;
    }
    if (*solver == "fmm" || *solver == "fmm_cpu") {
        return ParticleSystem::SolverMode::FmmCpu;
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
    constexpr bool kDevProfile = BLITZAR_PROFILE_IS_DEV != 0;
    if (!kDevProfile) {
        return ParticleSystem::IntegratorMode::Euler;
    }
    const auto integrator = bltzr_env::get("BLITZAR_INTEGRATOR");
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
 * @brief Documents the clamp acceleration operation contract.
 * @param accel Input value used by this contract.
 * @param maxAcceleration Input value used by this contract.
 * @return Vector3 value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
