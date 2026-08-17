/*
 * @file engine/physics/core/cuda/buffer/CudPlanning.inl
 * @project BLITZAR
 * @brief Particle-system buffer lifecycle implementation fragment.
 */

/*
 * @file engine/physics/core/cuda/buffer/CudPlanning.inl
 * @author Luis1454
 * @project BLITZAR
 * @brief Physics and CUDA implementation for the deterministic simulation core.
 */

/*
 * Module: cuda
 * Responsibility: Manage particle-system buffer allocation and release paths.
 */

#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

constexpr std::size_t kVramBudgetBytes = 6656ull * 1024ull * 1024ull;
constexpr std::size_t kDefaultEnergySampleLimit = 65536u;
constexpr std::size_t kPlanAEnergySampleLimit = 4096u;
constexpr int kDefaultOctreeLeafCapacity = 256;
constexpr int kPlanBOctreeLeafCapacity = 4096;

  bool ParticleSystem::treePmFastPathBypassesOctreeScratch(bool eulerIntegrator) const
  {
      const bool legacyLocalGrid = _treePmModel == "auto" || _treePmModel.empty();
      const bool localGridModel = _treePmModel == "local_grid" || _treePmModel == "pm_only";
      return _treePmEnabled && eulerIntegrator && (localGridModel ||
                                                    (legacyLocalGrid && _treePmLocalGrid));
  }

bool ParticleSystem::treePmUsesGravityOnlyBuffers(bool eulerIntegrator, bool sphEnabled) const
{
    return !sphEnabled && treePmFastPathBypassesOctreeScratch(eulerIntegrator) &&
           _treePmGravityOnlyBuffers;
}

/*
 * @brief Documents the bytes to mi b operation contract.
 * @param bytes Input value used by this contract.
 * @return double value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static double bytesToMiB(std::size_t bytes)
{
    return static_cast<double>(bytes) / (1024.0 * 1024.0);
}

/*
 * @brief Documents the cuda runtime available operation contract.
 * @param None This contract does not take explicit parameters.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static bool cudaRuntimeAvailable()
{
    int deviceCount = 0;
    const cudaError_t status = cudaGetDeviceCount(&deviceCount);
    if (status == cudaSuccess && deviceCount > 0) {
        return true;
    }
    if (status != cudaSuccess && status != cudaErrorNoDevice) {
        std::cerr << "[cuda] runtime disabled: " << cudaGetErrorString(status) << "\n";
    }
    cudaGetLastError();
    return false;
}

/*
 * @brief Documents the estimate memory usage operation contract.
 * @param particleCount Input value used by this contract.
 * @param sphEnabled Input value used by this contract.
 * @param solverMode Input value used by this contract.
 * @param integratorMode Input value used by this contract.
 * @param energySampleLimit Input value used by this contract.
 * @param octreeLeafCapacity Input value used by this contract.
 * @param baseAndIntegratorBytes Input value used by this contract.
 * @param sphBytes Input value used by this contract.
 * @param octreeBytes Input value used by this contract.
 * @return std::size_t ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
std::size_t ParticleSystem::estimateMemoryUsage(
    std::size_t particleCount, bool sphEnabled, SolverMode solverMode,
    IntegratorMode integratorMode, std::size_t energySampleLimit, int octreeLeafCapacity,
    std::size_t* baseAndIntegratorBytes, std::size_t* sphBytes, std::size_t* octreeBytes) const
{
    const bool gravityOnlyBuffers =
        treePmUsesGravityOnlyBuffers(integratorMode == IntegratorMode::Euler, sphEnabled);
    // Acceleration is also the exported pressure vector; keep it available in every runtime plan.
    const std::size_t soaFloatCount = gravityOnlyBuffers ? 16u : 18u;
    const std::size_t baseSoABytes = particleCount * sizeof(float) * soaFloatCount;

    std::size_t integratorBytes = 0u;
    if (integratorMode == IntegratorMode::Leapfrog || integratorMode == IntegratorMode::Rk4) {
        integratorBytes += particleCount * sizeof(Particle);
        integratorBytes += particleCount * sizeof(Vector3) * 8u;
        if (integratorMode == IntegratorMode::Leapfrog) {
            integratorBytes += particleCount * sizeof(float3);
        }
    }

    std::size_t sphBufferBytes = 0u;
    if (sphEnabled) {
        sphBufferBytes += particleCount * sizeof(float) * 2u;
        sphBufferBytes += particleCount * sizeof(int) * 2u;
    }

    const bool treePmFastPath =
        solverMode == SolverMode::OctreeGpu &&
        treePmFastPathBypassesOctreeScratch(integratorMode == IntegratorMode::Euler);
    std::size_t octreeBufferBytes = 0u;
    if (solverMode == SolverMode::OctreeGpu && !treePmFastPath) {
        octreeBufferBytes += particleCount * 44u;

        const int defaultLeafCapacity = kDefaultOctreeLeafCapacity;
        const int configuredLeafCapacity =
            std::max(16, _device->_linearOctreeLeafCapacity > 0 ? _device->_linearOctreeLeafCapacity
                                                               : defaultLeafCapacity);
        const int leafCapacity =
            octreeLeafCapacity > 0 ? std::max(16, octreeLeafCapacity) : configuredLeafCapacity;

        int leafDepth = 1;
        while (leafDepth < 21) {
            const double avgParticlesPerBucket =
                static_cast<double>(particleCount) / static_cast<double>(1ull << (3 * leafDepth));
            if (avgParticlesPerBucket <= static_cast<double>(leafCapacity)) {
                break;
            }
            ++leafDepth;
        }

        const int expectedLeaves =
            std::max(1, (static_cast<int>(particleCount) + leafCapacity - 1) / leafCapacity);
        const int requiredNodeCapacity = std::max(2, expectedLeaves * (leafDepth + 1) * 4 + 8);
        octreeBufferBytes += static_cast<std::size_t>(requiredNodeCapacity) * sizeof(GpuOctreeNode);
        octreeBufferBytes +=
            static_cast<std::size_t>(requiredNodeCapacity) * sizeof(GpuOctreeNodeHotData);
        octreeBufferBytes +=
            static_cast<std::size_t>(requiredNodeCapacity) * sizeof(GpuOctreeNodeNavData);
        octreeBufferBytes += static_cast<std::size_t>(requiredNodeCapacity) * sizeof(int) * 3u;
    }

    if (solverMode == SolverMode::OctreeGpu && _treePmEnabled && _treePmModel != "exact_tree") {
        const int defaultGridSize = 64;
        const int requestedGridSize =
            std::clamp(_treePmGridSize > 0 ? _treePmGridSize : defaultGridSize, 32, 128);
        const int gridSize = requestedGridSize * 2;
        const std::size_t gridCells = static_cast<std::size_t>(gridSize) * gridSize * gridSize;
        const std::size_t spectrumCells = static_cast<std::size_t>(gridSize) * gridSize *
                                          static_cast<std::size_t>(gridSize / 2 + 1);
        const std::size_t maskWords = (gridCells + 31u) / 32u;
        octreeBufferBytes += gridCells * sizeof(float) * 6u;
        octreeBufferBytes += spectrumCells * sizeof(float) * 2u * 4u;
        octreeBufferBytes += maskWords * sizeof(unsigned int);
        if (treePmFastPath && _treePmMaxLocalNeighbors > 0) {
            octreeBufferBytes += particleCount * sizeof(int) * 2u;
            octreeBufferBytes += gridCells * sizeof(int) * 2u;
        }
    }

    const std::size_t boundedSample =
        std::max<std::size_t>(64u, std::min(particleCount, energySampleLimit));
    const std::size_t blockCount =
        (particleCount + Particle::kDefaultCudaBlockSize - 1u) / Particle::kDefaultCudaBlockSize;
    const std::size_t energyScratchBytes =
        blockCount * sizeof(float) * 2u + boundedSample * sizeof(double);

    const std::size_t fixedTelemetryBytes = sizeof(GpuSystemMetrics);
    const std::size_t total = baseSoABytes + integratorBytes + sphBufferBytes + octreeBufferBytes +
                              energyScratchBytes + fixedTelemetryBytes;

    if (baseAndIntegratorBytes != nullptr) {
        *baseAndIntegratorBytes = baseSoABytes + integratorBytes + energyScratchBytes;
    }
    if (sphBytes != nullptr) {
        *sphBytes = sphBufferBytes;
    }
    if (octreeBytes != nullptr) {
        *octreeBytes = octreeBufferBytes;
    }
    return total;
}

/*
 * @brief Documents the format memory breakdown operation contract.
 * @param baseAndIntegratorBytes Input value used by this contract.
 * @param sphBytes Input value used by this contract.
 * @param octreeBytes Input value used by this contract.
 * @param totalBytes Input value used by this contract.
 * @param budgetBytes Input value used by this contract.
 * @return std::string ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
std::string ParticleSystem::formatMemoryBreakdown(std::size_t baseAndIntegratorBytes,
                                                  std::size_t sphBytes, std::size_t octreeBytes,
                                                  std::size_t totalBytes, std::size_t budgetBytes)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(2)
        << "[info] [memory] Base/Integrator: " << bytesToMiB(baseAndIntegratorBytes) << " MB\n"
        << "[info] [memory] SPH Buffers: " << bytesToMiB(sphBytes) << " MB\n"
        << "[info] [memory] Octree Scratch/Nodes: " << bytesToMiB(octreeBytes) << " MB\n"
        << "[info] [memory] TOTAL: " << bytesToMiB(totalBytes) << " MB / "
        << bytesToMiB(budgetBytes) << " MB ("
        << (100.0 * static_cast<double>(totalBytes) / static_cast<double>(budgetBytes)) << "%)";
    return out.str();
}

/*
 * @brief Documents the initialize runtime state operation contract.
 * @param particleCapacity Input value used by this contract.
 * @return void ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
