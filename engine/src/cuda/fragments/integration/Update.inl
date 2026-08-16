/*
 * @file engine/src/cuda/fragments/integration/Update.inl
 * @author Luis1454
 * @project BLITZAR
 * @brief Physics and CUDA implementation for the deterministic simulation core.
 */

/*
 * Module: cuda
 * Responsibility: Advance the particle system for one deterministic update step.
 */

#include <algorithm>
#include <chrono>
#include <cstdlib>

struct CudaEndToEndProfiler final {
    cudaEvent_t start = nullptr;
    cudaEvent_t stop = nullptr;
    double* totalMilliseconds = nullptr;
    std::uint64_t* sampleCount = nullptr;
    std::uint32_t interval = 1000u;
    bool active = false;

    CudaEndToEndProfiler(bool enabled, double* total, std::uint64_t* samples)
        : totalMilliseconds(total), sampleCount(samples)
    {
        if (!enabled || totalMilliseconds == nullptr || sampleCount == nullptr ||
            cudaEventCreate(&start) != cudaSuccess || cudaEventCreate(&stop) != cudaSuccess ||
            cudaEventRecord(start) != cudaSuccess) {
            if (start != nullptr) {
                cudaEventDestroy(start);
                start = nullptr;
            }
            if (stop != nullptr) {
                cudaEventDestroy(stop);
                stop = nullptr;
            }
            return;
        }
        if (const char* rawInterval = std::getenv("BLITZAR_CUDA_E2E_PROFILE_INTERVAL");
            rawInterval != nullptr && rawInterval[0] != '\0') {
            const unsigned long parsed = std::strtoul(rawInterval, nullptr, 10);
            interval =
                static_cast<std::uint32_t>(std::clamp<unsigned long>(parsed, 1ul, 1000000ul));
        }
        active = true;
    }

    ~CudaEndToEndProfiler()
    {
        if (!active) {
            return;
        }
        if (cudaEventRecord(stop) == cudaSuccess && cudaEventSynchronize(stop) == cudaSuccess) {
            float elapsedMilliseconds = 0.0f;
            if (cudaEventElapsedTime(&elapsedMilliseconds, start, stop) == cudaSuccess) {
                *totalMilliseconds += static_cast<double>(elapsedMilliseconds);
                *sampleCount += 1u;
                if ((*sampleCount % interval) == 0u) {
                    fprintf(stderr,
                            "[cuda-e2e] samples=%llu last_ms=%.4f avg_ms=%.4f interval=%u\n",
                            static_cast<unsigned long long>(*sampleCount), elapsedMilliseconds,
                            *totalMilliseconds / static_cast<double>(*sampleCount), interval);
                }
            }
        }
        cudaEventDestroy(start);
        cudaEventDestroy(stop);
    }
};

/* Apply the host-side cosmology correction shared by CPU and thermal paths. */
void ParticleSystem::applyHostCosmologyStep(float deltaTime)
{
    float scaleRatio = 1.0f;
    float previousHubble = 0.0f;
    float nextHubble = 0.0f;
    if (prepareCosmologyStep(deltaTime, scaleRatio, previousHubble, nextHubble)) {
        applyCosmologyExpansionHost(scaleRatio, previousHubble, nextHubble);
    }
}

/*
 * @brief Advances one simulation step using the selected backend.
 * @param deltaTime Input value used by this contract.
 * @return bool ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool ParticleSystem::update(float deltaTime)
{
    const ForceLawPolicy forceLaw =
        resolveForceLawPolicy(_octreeTheta, _octreeSoftening, _physicsMinSoftening,
                              _physicsMinDistance2, _physicsMinTheta);
    const bool thermalActive = (_thermalHeatingCoeff > 0.0f || _thermalRadiationCoeff > 0.0f);
    const bool e2eProfileEnabled =
        _device._cudaRuntimeAvailable && _solverMode != SolverMode::OctreeCpu &&
        _solverMode != SolverMode::FmmCpu && parseBoolEnv("BLITZAR_CUDA_E2E_PROFILE", false);
    CudaEndToEndProfiler e2eProfiler(e2eProfileEnabled, &_device._cudaE2eTotalMs,
                                     &_device._cudaE2eSamples);

    if (_adaptiveTimeStepsEnabled && _adaptiveTimeStepCostGuard &&
        !_adaptiveTimeStepMarkerPrinted) {
        fprintf(stderr, "[adaptive] backend=fixed_equivalent scheduler=dyadic reason=cost_guard "
                        "force_with_--adaptive-cost-guard=false\n");
        _adaptiveTimeStepMarkerPrinted = true;
    }

    // The CUDA-native path handles pairwise and Octree GPU modes. Keep the
    // reference implementation only for the explicit CPU/FMM solvers.
    if (_adaptiveTimeStepsEnabled && !_adaptiveTimeStepCostGuard &&
        (_solverMode == SolverMode::OctreeCpu || _solverMode == SolverMode::FmmCpu)) {
        return updateAdaptiveTimeSteps(deltaTime, forceLaw, thermalActive);
    }

    if (_solverMode == SolverMode::OctreeCpu || _solverMode == SolverMode::FmmCpu) {
        return updateCpuSolvers(deltaTime, forceLaw, thermalActive);
    }

    if (_solverMode == SolverMode::OctreeGpu) {
        return updateOctreeGpu(deltaTime, forceLaw, thermalActive);
    }

    return updateCudaSolvers(deltaTime, forceLaw, thermalActive);
}

// Note: destroyParticles logic moved to ParticleSystem destructor and releaseParticleBuffers.
