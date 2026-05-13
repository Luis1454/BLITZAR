/*
 * @file engine/src/cuda/fragments/system/Buffer.inl
 * @author BLITZAR Contributors
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

static bool treePmFastPathBypassesOctreeScratch(bool eulerIntegrator)
{
    return parseBoolEnv("BLITZAR_TREEPM_ENABLED", false) &&
           eulerIntegrator && parseBoolEnv("BLITZAR_TREEPM_LOCAL_GRID", true);
}

static bool treePmUsesGravityOnlyBuffers(bool eulerIntegrator, bool sphEnabled)
{
    return !sphEnabled && treePmFastPathBypassesOctreeScratch(eulerIntegrator) &&
           parseBoolEnv("BLITZAR_TREEPM_GRAVITY_ONLY_BUFFERS", true);
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
    const bool gravityOnlyBuffers = treePmUsesGravityOnlyBuffers(
        integratorMode == IntegratorMode::Euler, sphEnabled);
    const std::size_t soaFloatCount = gravityOnlyBuffers ? 13u : 18u;
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
            std::max(16, static_cast<int>(parseFloatEnv("BLITZAR_LINEAR_OCTREE_LEAF_CAPACITY",
                                                        static_cast<float>(defaultLeafCapacity))));
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

    if (solverMode == SolverMode::OctreeGpu && parseBoolEnv("BLITZAR_TREEPM_ENABLED", false)) {
        const int defaultGridSize = 64;
        const int gridSize = std::clamp(
            static_cast<int>(parseFloatEnv("BLITZAR_TREEPM_GRID_SIZE",
                                           static_cast<float>(defaultGridSize))),
            32, 128);
        const std::size_t gridCells = static_cast<std::size_t>(gridSize) * gridSize * gridSize;
        const std::size_t maskWords = (gridCells + 31u) / 32u;
        octreeBufferBytes += gridCells * sizeof(float) * 6u;
        octreeBufferBytes += maskWords * sizeof(unsigned int);
        if (treePmFastPath &&
            static_cast<int>(parseFloatEnv("BLITZAR_TREEPM_MAX_LOCAL_NEIGHBORS", 64.0f)) > 0) {
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
void ParticleSystem::initializeRuntimeState(std::size_t particleCapacity)
{
    _device._cudaRuntimeAvailable = cudaRuntimeAvailable();
    if (_device._cudaRuntimeAvailable) {
        const cudaError_t mapHostStatus = cudaSetDeviceFlags(cudaDeviceMapHost);
        if (mapHostStatus != cudaSuccess && mapHostStatus != cudaErrorSetOnActiveProcess) {
            checkCudaStatus(mapHostStatus, "cudaSetDeviceFlags(cudaDeviceMapHost)");
            _device._cudaRuntimeAvailable = false;
        }
    }
    if (_device._cudaRuntimeAvailable) {
        bltzr_x::MemoryPool::initialize();
    }
    _solverMode = solverModeFromEnv();
    _integratorMode = integratorModeFromEnv();
    _octreeTheta = parseFloatEnv("BLITZAR_OCTREE_THETA", 1.2f);
    _octreeOpeningCriterion = OctreeOpeningCriterion::CenterOfMass;
    _octreeSoftening = parseFloatEnv("BLITZAR_OCTREE_SOFTENING", 2.5f);
    _sphEnabled = parseBoolEnv("BLITZAR_SPH_ENABLED", false);
    _sphSmoothingLength = parseFloatEnv("BLITZAR_SPH_H", 1.25f);
    _sphRestDensity = parseFloatEnv("BLITZAR_SPH_REST_DENSITY", 1.0f);
    _sphGasConstant = parseFloatEnv("BLITZAR_SPH_GAS_CONSTANT", 4.0f);
    _sphViscosity = parseFloatEnv("BLITZAR_SPH_VISCOSITY", 0.08f);
    _thermalAmbientTemperature = parseFloatEnv("BLITZAR_THERMAL_AMBIENT", 0.0f);
    _thermalSpecificHeat = parseFloatEnv("BLITZAR_THERMAL_SPECIFIC_HEAT", 1.0f);
    _thermalHeatingCoeff = parseFloatEnv("BLITZAR_THERMAL_HEATING", 0.0002f);
    _thermalRadiationCoeff = parseFloatEnv("BLITZAR_THERMAL_RADIATION", 0.00000001f);
    _physicsMaxAcceleration = kPhysicsMaxAccelerationDefault;
    _physicsMinSoftening = kPhysicsMinSofteningDefault;
    _physicsMinDistance2 = kPhysicsMinDistance2Default;
    _physicsMinTheta = kPhysicsMinTheta;
    _sphMaxAcceleration = kSphMaxAccelerationDefault;
    _sphMaxSpeed = kSphMaxSpeedDefault;
    _cumulativeRadiatedEnergy = 0.0f;
    _device._sphGridSize = 0;
    _device._sphGridTotalCells = 0;
    _device._treePmMarkerPrinted = false;
    _device._treePmGridSize = 0;
    _device._treePmTotalCells = 0;
    _device._deviceParticleCapacity = particleCapacity;
    _device._hostStateDirty = false;
    _device.d_soaPosX = nullptr;
    _device.d_soaPosY = nullptr;
    _device.d_soaPosZ = nullptr;
    _device.d_soaVelX = nullptr;
    _device.d_soaVelY = nullptr;
    _device.d_soaVelZ = nullptr;
    _device.d_soaPressX = nullptr;
    _device.d_soaPressY = nullptr;
    _device.d_soaPressZ = nullptr;
    _device.d_soaMass = nullptr;
    _device.d_soaTemp = nullptr;
    _device.d_soaDens = nullptr;
    _device.d_soaNextPosX = nullptr;
    _device.d_soaNextPosY = nullptr;
    _device.d_soaNextPosZ = nullptr;
    _device.d_soaNextVelX = nullptr;
    _device.d_soaNextVelY = nullptr;
    _device.d_soaNextVelZ = nullptr;
    _device.d_stage = nullptr;
    _device.d_k1x = nullptr;
    _device.d_k2x = nullptr;
    _device.d_k3x = nullptr;
    _device.d_k4x = nullptr;
    _device.d_k1v = nullptr;
    _device.d_k2v = nullptr;
    _device.d_k3v = nullptr;
    _device.d_k4v = nullptr;
    _device.d_vHalf = nullptr;
    _device._leapfrogPrimed = false;
    _device.d_sphDensity = nullptr;
    _device.d_sphPressure = nullptr;
    _device.d_sphCellHash = nullptr;
    _device.d_sphSortedIndex = nullptr;
    _device.d_sphCellStart = nullptr;
    _device.d_sphCellEnd = nullptr;
    _device.d_treePmDensity = nullptr;
    _device.d_treePmPotentialA = nullptr;
    _device.d_treePmPotentialB = nullptr;
    _device.d_treePmAccelX = nullptr;
    _device.d_treePmAccelY = nullptr;
    _device.d_treePmAccelZ = nullptr;
    _device.d_treePmCellMask = nullptr;
    _device.g_dOctreeNodes = nullptr;
    _device.g_dOctreeLeafIndices = nullptr;
    _device.d_octreeMortonKeys = nullptr;
    _device.d_octreePrefixesA = nullptr;
    _device.d_octreePrefixesB = nullptr;
    _device.d_octreeLevelIndicesA = nullptr;
    _device.d_octreeLevelIndicesB = nullptr;
    _device.d_octreeParentCounts = nullptr;
    _device.d_octreeParentOffsets = nullptr;
    _device.d_octreeNodeHot = nullptr;
    _device.d_octreeNodeNav = nullptr;
    _device.d_octreeFirstChild = nullptr;
    _device.d_octreeLeafStarts = nullptr;
    _device.d_octreeLeafCounts = nullptr;
    _device.d_energyKineticBlocks = nullptr;
    _device.d_energyThermalBlocks = nullptr;
    _device.d_energyPotentialPartials = nullptr;
    _device.g_dOctreeNodeCapacity = 0;
    _device.g_dOctreeLeafCapacity = 0;
    _device.d_octreeMortonCapacity = 0;
    _device.d_octreePrefixCapacity = 0;
    _device.d_octreeLevelCapacity = 0;
    _device.d_treePmCapacity = 0;
    _device.d_treePmMaskWordCapacity = 0;
    _device.d_treePmNeighborParticleCapacity = 0;
    _device.d_treePmNeighborCellCapacity = 0;
    _device.d_energyBlockCapacity = 0;
    _device.d_energySampleCapacity = 0;
    _device._gpuOctreeRootIndex = -1;
    _device._gpuOctreeNodeCount = 0;
    _device._gpuOctreeLeafCount = 0;
    _device._mappedMetricsHost = nullptr;
    _device._mappedMetricsDevice = nullptr;
    _device._metricsStepId = 0u;
    _device._metricsSimTime = 0.0f;
    _device._metricsPublishCounter = 0u;
    _device._linearOctreeLeafCapacity = std::max(
        16, static_cast<int>(parseFloatEnv("BLITZAR_LINEAR_OCTREE_LEAF_CAPACITY",
                                           static_cast<float>(kDefaultOctreeLeafCapacity))));

    const bool strictMemoryMode = parseBoolEnv("BLITZAR_STRICT_MEMORY_MODE", false);
    std::size_t selectedEnergySampleLimit = kDefaultEnergySampleLimit;
    int selectedOctreeLeafCapacity = _device._linearOctreeLeafCapacity;

    std::size_t baseAndIntegratorBytes = 0u;
    std::size_t sphBytes = 0u;
    std::size_t octreeBytes = 0u;
    std::size_t totalBytes = estimateMemoryUsage(
        particleCapacity, _sphEnabled, _solverMode, _integratorMode, selectedEnergySampleLimit,
        selectedOctreeLeafCapacity, &baseAndIntegratorBytes, &sphBytes, &octreeBytes);

    std::cout << formatMemoryBreakdown(baseAndIntegratorBytes, sphBytes, octreeBytes, totalBytes,
                                       kVramBudgetBytes)
              << "\n";

    const bool treePmFastPath =
        treePmFastPathBypassesOctreeScratch(_integratorMode == IntegratorMode::Euler);
    if (_solverMode == SolverMode::OctreeGpu && !treePmFastPath &&
        totalBytes > kVramBudgetBytes &&
        selectedOctreeLeafCapacity < kPlanBOctreeLeafCapacity) {
        selectedOctreeLeafCapacity = kPlanBOctreeLeafCapacity;
        _device._linearOctreeLeafCapacity = selectedOctreeLeafCapacity;
        totalBytes = estimateMemoryUsage(
            particleCapacity, _sphEnabled, _solverMode, _integratorMode, selectedEnergySampleLimit,
            selectedOctreeLeafCapacity, &baseAndIntegratorBytes, &sphBytes, &octreeBytes);
        std::cout << "[info] dynamic octree leaf capacity bump to " << selectedOctreeLeafCapacity
                  << " after VRAM estimate exceeded budget\n";
        std::cout << formatMemoryBreakdown(baseAndIntegratorBytes, sphBytes, octreeBytes,
                                           totalBytes, kVramBudgetBytes)
                  << "\n";
    }

    if (strictMemoryMode && totalBytes > kVramBudgetBytes) {
        selectedEnergySampleLimit = kPlanAEnergySampleLimit;
        totalBytes = estimateMemoryUsage(
            particleCapacity, _sphEnabled, _solverMode, _integratorMode, selectedEnergySampleLimit,
            selectedOctreeLeafCapacity, &baseAndIntegratorBytes, &sphBytes, &octreeBytes);
        std::cout << "[info] strict memory plan A (energy sample limit="
                  << selectedEnergySampleLimit << ")\n";
        std::cout << formatMemoryBreakdown(baseAndIntegratorBytes, sphBytes, octreeBytes,
                                           totalBytes, kVramBudgetBytes)
                  << "\n";
    }

    if (strictMemoryMode && totalBytes > kVramBudgetBytes &&
        _solverMode == SolverMode::OctreeGpu && !treePmFastPath) {
        selectedOctreeLeafCapacity = kPlanBOctreeLeafCapacity;
        totalBytes = estimateMemoryUsage(
            particleCapacity, _sphEnabled, _solverMode, _integratorMode, selectedEnergySampleLimit,
            selectedOctreeLeafCapacity, &baseAndIntegratorBytes, &sphBytes, &octreeBytes);
        std::cout << "[info] strict memory plan B (octree leaf capacity="
                  << selectedOctreeLeafCapacity << ")\n";
        std::cout << formatMemoryBreakdown(baseAndIntegratorBytes, sphBytes, octreeBytes,
                                           totalBytes, kVramBudgetBytes)
                  << "\n";
    }

    if (strictMemoryMode && totalBytes > kVramBudgetBytes) {
        throw std::runtime_error(
            std::string("[memory] strict admission-control rejected configuration\n") +
            formatMemoryBreakdown(baseAndIntegratorBytes, sphBytes, octreeBytes, totalBytes,
                                  kVramBudgetBytes) +
            "\n[memory] plan Z: strict mode abort");
    }

    if (_device._cudaRuntimeAvailable) {
        allocateMappedMetrics();
    }
}

/*
 * @brief Documents the allocate particle buffers operation contract.
 * @param particleCapacity Input value used by this contract.
 * @return bool ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool ParticleSystem::allocateParticleBuffers(std::size_t particleCapacity)
{
    if (!_device._cudaRuntimeAvailable) {
        return false;
    }
    const std::size_t bytesTotal = particleCapacity * sizeof(float);
    const bool gravityOnlyBuffers = treePmUsesGravityOnlyBuffers(
        _integratorMode == IntegratorMode::Euler, _sphEnabled);
    bool ok = true;
    ok &= ((_device.d_soaPosX = static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) !=
           nullptr);
    ok &= ((_device.d_soaPosY = static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) !=
           nullptr);
    ok &= ((_device.d_soaPosZ = static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) !=
           nullptr);
    ok &= ((_device.d_soaVelX = static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) !=
           nullptr);
    ok &= ((_device.d_soaVelY = static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) !=
           nullptr);
    ok &= ((_device.d_soaVelZ = static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) !=
           nullptr);
    ok &= ((_device.d_soaMass = static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) !=
           nullptr);
    if (!gravityOnlyBuffers) {
        ok &= ((_device.d_soaPressX =
                    static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) != nullptr);
        ok &= ((_device.d_soaPressY =
                    static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) != nullptr);
        ok &= ((_device.d_soaPressZ =
                    static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) != nullptr);
        ok &= ((_device.d_soaTemp =
                    static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) != nullptr);
        ok &= ((_device.d_soaDens =
                    static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) != nullptr);
    }
    ok &= ((_device.d_soaNextPosX = static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) !=
           nullptr);
    ok &= ((_device.d_soaNextPosY = static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) !=
           nullptr);
    ok &= ((_device.d_soaNextPosZ = static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) !=
           nullptr);
    ok &= ((_device.d_soaNextVelX = static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) !=
           nullptr);
    ok &= ((_device.d_soaNextVelY = static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) !=
           nullptr);
    ok &= ((_device.d_soaNextVelZ = static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) !=
           nullptr);

    if (!ok) {
        releaseParticleBuffers();
        return false;
    }
    return true;
}

/*
 * @brief Documents the release particle buffers operation contract.
 * @param None This contract does not take explicit parameters.
 * @return void ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void ParticleSystem::releaseParticleBuffers()
{
    releaseRk4Buffers();
    releaseSphBuffers();
    releaseSphGridBuffers();

    bltzr_x::MemoryPool::deallocate(_device.d_soaPosX);
    _device.d_soaPosX = nullptr;
    bltzr_x::MemoryPool::deallocate(_device.d_soaPosY);
    _device.d_soaPosY = nullptr;
    bltzr_x::MemoryPool::deallocate(_device.d_soaPosZ);
    _device.d_soaPosZ = nullptr;
    bltzr_x::MemoryPool::deallocate(_device.d_soaVelX);
    _device.d_soaVelX = nullptr;
    bltzr_x::MemoryPool::deallocate(_device.d_soaVelY);
    _device.d_soaVelY = nullptr;
    bltzr_x::MemoryPool::deallocate(_device.d_soaVelZ);

    bltzr_x::MemoryPool::deallocate(_device.d_treePmDensity);
    _device.d_treePmDensity = nullptr;
    bltzr_x::MemoryPool::deallocate(_device.d_treePmPotentialA);
    _device.d_treePmPotentialA = nullptr;
    bltzr_x::MemoryPool::deallocate(_device.d_treePmPotentialB);
    _device.d_treePmPotentialB = nullptr;
    bltzr_x::MemoryPool::deallocate(_device.d_treePmAccelX);
    _device.d_treePmAccelX = nullptr;
    bltzr_x::MemoryPool::deallocate(_device.d_treePmAccelY);
    _device.d_treePmAccelY = nullptr;
    bltzr_x::MemoryPool::deallocate(_device.d_treePmAccelZ);
    _device.d_treePmAccelZ = nullptr;
    bltzr_x::MemoryPool::deallocate(_device.d_treePmCellMask);
    _device.d_treePmCellMask = nullptr;
    _device.d_soaVelZ = nullptr;
    bltzr_x::MemoryPool::deallocate(_device.d_soaPressX);
    _device.d_soaPressX = nullptr;
    bltzr_x::MemoryPool::deallocate(_device.d_soaPressY);
    _device.d_soaPressY = nullptr;
    bltzr_x::MemoryPool::deallocate(_device.d_soaPressZ);
    _device.d_soaPressZ = nullptr;
    bltzr_x::MemoryPool::deallocate(_device.d_soaMass);
    _device.d_soaMass = nullptr;
    bltzr_x::MemoryPool::deallocate(_device.d_soaTemp);
    _device.d_soaTemp = nullptr;
    bltzr_x::MemoryPool::deallocate(_device.d_soaDens);
    _device.d_soaDens = nullptr;
    bltzr_x::MemoryPool::deallocate(_device.d_soaNextPosX);
    _device.d_soaNextPosX = nullptr;
    bltzr_x::MemoryPool::deallocate(_device.d_soaNextPosY);
    _device.d_soaNextPosY = nullptr;
    bltzr_x::MemoryPool::deallocate(_device.d_soaNextPosZ);
    _device.d_soaNextPosZ = nullptr;
    bltzr_x::MemoryPool::deallocate(_device.d_soaNextVelX);
    _device.d_soaNextVelX = nullptr;
    bltzr_x::MemoryPool::deallocate(_device.d_soaNextVelY);
    _device.d_soaNextVelY = nullptr;
    bltzr_x::MemoryPool::deallocate(_device.d_soaNextVelZ);
    _device.d_soaNextVelZ = nullptr;
    bltzr_x::MemoryPool::deallocate(_device.d_vHalf);
    _device.d_vHalf = nullptr;
    _device._leapfrogPrimed = false;

    if (_device.g_dOctreeNodes) {
        bltzr_x::MemoryPool::deallocate(_device.g_dOctreeNodes);
        _device.g_dOctreeNodes = nullptr;
    }
    if (_device.g_dOctreeLeafIndices) {
        bltzr_x::MemoryPool::deallocate(_device.g_dOctreeLeafIndices);
        _device.g_dOctreeLeafIndices = nullptr;
    }
    if (_device.d_octreeMortonKeys) {
        bltzr_x::MemoryPool::deallocate(_device.d_octreeMortonKeys);
        _device.d_octreeMortonKeys = nullptr;
    }
    if (_device.d_octreePrefixesA) {
        bltzr_x::MemoryPool::deallocate(_device.d_octreePrefixesA);
        _device.d_octreePrefixesA = nullptr;
    }
    if (_device.d_octreePrefixesB) {
        bltzr_x::MemoryPool::deallocate(_device.d_octreePrefixesB);
        _device.d_octreePrefixesB = nullptr;
    }
    if (_device.d_octreeLevelIndicesA) {
        bltzr_x::MemoryPool::deallocate(_device.d_octreeLevelIndicesA);
        _device.d_octreeLevelIndicesA = nullptr;
    }
    if (_device.d_octreeLevelIndicesB) {
        bltzr_x::MemoryPool::deallocate(_device.d_octreeLevelIndicesB);
        _device.d_octreeLevelIndicesB = nullptr;
    }
    if (_device.d_octreeParentCounts) {
        bltzr_x::MemoryPool::deallocate(_device.d_octreeParentCounts);
        _device.d_octreeParentCounts = nullptr;
    }
    if (_device.d_octreeParentOffsets) {
        bltzr_x::MemoryPool::deallocate(_device.d_octreeParentOffsets);
        _device.d_octreeParentOffsets = nullptr;
    }
    if (_device.d_octreeNodeHot) {
        bltzr_x::MemoryPool::deallocate(_device.d_octreeNodeHot);
        _device.d_octreeNodeHot = nullptr;
    }
    if (_device.d_octreeNodeNav) {
        bltzr_x::MemoryPool::deallocate(_device.d_octreeNodeNav);
        _device.d_octreeNodeNav = nullptr;
    }
    if (_device.d_octreeFirstChild) {
        bltzr_x::MemoryPool::deallocate(_device.d_octreeFirstChild);
        _device.d_octreeFirstChild = nullptr;
    }
    if (_device.d_octreeLeafStarts) {
        bltzr_x::MemoryPool::deallocate(_device.d_octreeLeafStarts);
        _device.d_octreeLeafStarts = nullptr;
    }
    if (_device.d_octreeLeafCounts) {
        bltzr_x::MemoryPool::deallocate(_device.d_octreeLeafCounts);
        _device.d_octreeLeafCounts = nullptr;
    }
    if (_device.d_energyKineticBlocks) {
        bltzr_x::MemoryPool::deallocate(_device.d_energyKineticBlocks);
        _device.d_energyKineticBlocks = nullptr;
    }
    if (_device.d_energyThermalBlocks) {
        bltzr_x::MemoryPool::deallocate(_device.d_energyThermalBlocks);
        _device.d_energyThermalBlocks = nullptr;
    }
    if (_device.d_energyPotentialPartials) {
        bltzr_x::MemoryPool::deallocate(_device.d_energyPotentialPartials);
        _device.d_energyPotentialPartials = nullptr;
    }
    _device._deviceParticleCapacity = 0;
    _device.g_dOctreeNodeCapacity = 0;
    _device.g_dOctreeLeafCapacity = 0;
    _device.d_octreeMortonCapacity = 0;
    _device.d_octreePrefixCapacity = 0;
    _device.d_octreeLevelCapacity = 0;
    _device.d_treePmCapacity = 0;
    _device.d_treePmMaskWordCapacity = 0;
    _device.d_treePmNeighborParticleCapacity = 0;
    _device.d_treePmNeighborCellCapacity = 0;
    _device.d_energyBlockCapacity = 0;
    _device.d_energySampleCapacity = 0;
    _device._gpuOctreeRootIndex = -1;
    _device._gpuOctreeNodeCount = 0;
    _device._gpuOctreeLeafCount = 0;
    releaseMappedMetrics();
}

/*
 * @brief Documents the allocate mapped metrics operation contract.
 * @param None This contract does not take explicit parameters.
 * @return bool ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool ParticleSystem::allocateMappedMetrics()
{
    if (!_device._cudaRuntimeAvailable) {
        return false;
    }
    releaseMappedMetrics();
    void* hostPtr = nullptr;
    cudaError_t status = cudaHostAlloc(&hostPtr, sizeof(GpuSystemMetrics),
                                       cudaHostAllocMapped | cudaHostAllocPortable);
    if (!checkCudaStatus(status, "cudaHostAlloc(mapped metrics)")) {
        return false;
    }

    std::memset(hostPtr, 0, sizeof(GpuSystemMetrics));
    _device._mappedMetricsHost = static_cast<GpuSystemMetrics*>(hostPtr);

    void* devicePtr = nullptr;
    status = cudaHostGetDevicePointer(&devicePtr, hostPtr, 0);
    if (!checkCudaStatus(status, "cudaHostGetDevicePointer(mapped metrics)")) {
        cudaFreeHost(hostPtr);
        _device._mappedMetricsHost = nullptr;
        return false;
    }
    _device._mappedMetricsDevice = static_cast<GpuSystemMetrics*>(devicePtr);
    _device._metricsStepId = 0u;
    _device._metricsSimTime = 0.0f;
    _device._metricsPublishCounter = 0u;
    return true;
}

/*
 * @brief Documents the release mapped metrics operation contract.
 * @param None This contract does not take explicit parameters.
 * @return void ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void ParticleSystem::releaseMappedMetrics()
{
    if (_device._mappedMetricsHost != nullptr) {
        cudaFreeHost(_device._mappedMetricsHost);
    }
    _device._mappedMetricsHost = nullptr;
    _device._mappedMetricsDevice = nullptr;
    _device._metricsStepId = 0u;
    _device._metricsSimTime = 0.0f;
    _device._metricsPublishCounter = 0u;
}

/*
 * @brief Documents the ensure linear octree scratch capacity operation contract.
 * @param numParticles Input value used by this contract.
 * @return bool ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool ParticleSystem::ensureLinearOctreeScratchCapacity(int numParticles)
{
    if (!_device._cudaRuntimeAvailable) {
        return false;
    }
    if (numParticles <= 0) {
        return false;
    }

    const int leafCapacity = std::max(16, _device._linearOctreeLeafCapacity);

    int leafDepth = 1;
    while (leafDepth < 21) {
        const double avgParticlesPerBucket =
            static_cast<double>(numParticles) / static_cast<double>(1ull << (3 * leafDepth));
        if (avgParticlesPerBucket <= static_cast<double>(leafCapacity)) {
            break;
        }
        ++leafDepth;
    }

    if (_device.g_dOctreeLeafCapacity < static_cast<std::size_t>(numParticles)) {
        if (_device.g_dOctreeLeafIndices) {
            bltzr_x::MemoryPool::deallocate(_device.g_dOctreeLeafIndices);
            _device.g_dOctreeLeafIndices = nullptr;
        }
        _device.g_dOctreeLeafIndices = static_cast<int*>(bltzr_x::MemoryPool::allocate(
            static_cast<std::size_t>(numParticles) * sizeof(int)));
        if (!_device.g_dOctreeLeafIndices) {
            _device.g_dOctreeLeafCapacity = 0;
            return false;
        }
        _device.g_dOctreeLeafCapacity = static_cast<std::size_t>(numParticles);
    }

    if (_device.d_octreeMortonCapacity < static_cast<std::size_t>(numParticles)) {
        if (_device.d_octreeMortonKeys) {
            bltzr_x::MemoryPool::deallocate(_device.d_octreeMortonKeys);
            _device.d_octreeMortonKeys = nullptr;
        }
        _device.d_octreeMortonKeys = static_cast<unsigned long long*>(bltzr_x::MemoryPool::allocate(
            static_cast<std::size_t>(numParticles) * sizeof(unsigned long long)));
        if (!_device.d_octreeMortonKeys) {
            _device.d_octreeMortonCapacity = 0;
            return false;
        }
        _device.d_octreeMortonCapacity = static_cast<std::size_t>(numParticles);
    }

    if (_device.d_octreePrefixCapacity < static_cast<std::size_t>(numParticles)) {
        if (_device.d_octreePrefixesA) {
            bltzr_x::MemoryPool::deallocate(_device.d_octreePrefixesA);
            _device.d_octreePrefixesA = nullptr;
        }
        if (_device.d_octreePrefixesB) {
            bltzr_x::MemoryPool::deallocate(_device.d_octreePrefixesB);
            _device.d_octreePrefixesB = nullptr;
        }
        _device.d_octreePrefixesA = static_cast<unsigned long long*>(bltzr_x::MemoryPool::allocate(
            static_cast<std::size_t>(numParticles) * sizeof(unsigned long long)));
        _device.d_octreePrefixesB = static_cast<unsigned long long*>(bltzr_x::MemoryPool::allocate(
            static_cast<std::size_t>(numParticles) * sizeof(unsigned long long)));
        if (!_device.d_octreePrefixesA || !_device.d_octreePrefixesB) {
            if (_device.d_octreePrefixesA) {
                bltzr_x::MemoryPool::deallocate(_device.d_octreePrefixesA);
                _device.d_octreePrefixesA = nullptr;
            }
            if (_device.d_octreePrefixesB) {
                bltzr_x::MemoryPool::deallocate(_device.d_octreePrefixesB);
                _device.d_octreePrefixesB = nullptr;
            }
            _device.d_octreePrefixCapacity = 0;
            return false;
        }
        _device.d_octreePrefixCapacity = static_cast<std::size_t>(numParticles);
    }

    if (_device.d_octreeLevelCapacity < static_cast<std::size_t>(numParticles)) {
        if (_device.d_octreeLevelIndicesA) {
            bltzr_x::MemoryPool::deallocate(_device.d_octreeLevelIndicesA);
            _device.d_octreeLevelIndicesA = nullptr;
        }
        if (_device.d_octreeLevelIndicesB) {
            bltzr_x::MemoryPool::deallocate(_device.d_octreeLevelIndicesB);
            _device.d_octreeLevelIndicesB = nullptr;
        }
        if (_device.d_octreeParentCounts) {
            bltzr_x::MemoryPool::deallocate(_device.d_octreeParentCounts);
            _device.d_octreeParentCounts = nullptr;
        }
        if (_device.d_octreeParentOffsets) {
            bltzr_x::MemoryPool::deallocate(_device.d_octreeParentOffsets);
            _device.d_octreeParentOffsets = nullptr;
        }

        _device.d_octreeLevelIndicesA = static_cast<int*>(bltzr_x::MemoryPool::allocate(
            static_cast<std::size_t>(numParticles) * sizeof(int)));
        _device.d_octreeLevelIndicesB = static_cast<int*>(bltzr_x::MemoryPool::allocate(
            static_cast<std::size_t>(numParticles) * sizeof(int)));
        _device.d_octreeParentCounts = static_cast<int*>(bltzr_x::MemoryPool::allocate(
            static_cast<std::size_t>(numParticles) * sizeof(int)));
        _device.d_octreeParentOffsets = static_cast<int*>(bltzr_x::MemoryPool::allocate(
            static_cast<std::size_t>(numParticles) * sizeof(int)));
        if (!_device.d_octreeLevelIndicesA || !_device.d_octreeLevelIndicesB || !_device.d_octreeParentCounts ||
            !_device.d_octreeParentOffsets) {
            if (_device.d_octreeLevelIndicesA) {
                bltzr_x::MemoryPool::deallocate(_device.d_octreeLevelIndicesA);
                _device.d_octreeLevelIndicesA = nullptr;
            }
            if (_device.d_octreeLevelIndicesB) {
                bltzr_x::MemoryPool::deallocate(_device.d_octreeLevelIndicesB);
                _device.d_octreeLevelIndicesB = nullptr;
            }
            if (_device.d_octreeParentCounts) {
                bltzr_x::MemoryPool::deallocate(_device.d_octreeParentCounts);
                _device.d_octreeParentCounts = nullptr;
            }
            if (_device.d_octreeParentOffsets) {
                bltzr_x::MemoryPool::deallocate(_device.d_octreeParentOffsets);
                _device.d_octreeParentOffsets = nullptr;
            }
            _device.d_octreeLevelCapacity = 0;
            return false;
        }
        _device.d_octreeLevelCapacity = static_cast<std::size_t>(numParticles);
    }

    const int expectedLeaves = std::max(1, (numParticles + leafCapacity - 1) / leafCapacity);
    const int requiredNodeCapacity = std::max(2, expectedLeaves * (leafDepth + 1) * 4 + 8);
    if (_device.g_dOctreeNodeCapacity < static_cast<std::size_t>(requiredNodeCapacity)) {
        if (_device.g_dOctreeNodes) {
            bltzr_x::MemoryPool::deallocate(_device.g_dOctreeNodes);
            _device.g_dOctreeNodes = nullptr;
        }
        if (_device.d_octreeNodeHot) {
            bltzr_x::MemoryPool::deallocate(_device.d_octreeNodeHot);
            _device.d_octreeNodeHot = nullptr;
        }
        if (_device.d_octreeNodeNav) {
            bltzr_x::MemoryPool::deallocate(_device.d_octreeNodeNav);
            _device.d_octreeNodeNav = nullptr;
        }
        if (_device.d_octreeFirstChild) {
            bltzr_x::MemoryPool::deallocate(_device.d_octreeFirstChild);
            _device.d_octreeFirstChild = nullptr;
        }
        if (_device.d_octreeLeafStarts) {
            bltzr_x::MemoryPool::deallocate(_device.d_octreeLeafStarts);
            _device.d_octreeLeafStarts = nullptr;
        }
        if (_device.d_octreeLeafCounts) {
            bltzr_x::MemoryPool::deallocate(_device.d_octreeLeafCounts);
            _device.d_octreeLeafCounts = nullptr;
        }

        _device.g_dOctreeNodes = static_cast<GpuOctreeNode*>(bltzr_x::MemoryPool::allocate(
            static_cast<std::size_t>(requiredNodeCapacity) * sizeof(GpuOctreeNode)));
        _device.d_octreeNodeHot = static_cast<GpuOctreeNodeHotData*>(bltzr_x::MemoryPool::allocate(
            static_cast<std::size_t>(requiredNodeCapacity) * sizeof(GpuOctreeNodeHotData)));
        _device.d_octreeNodeNav = static_cast<GpuOctreeNodeNavData*>(bltzr_x::MemoryPool::allocate(
            static_cast<std::size_t>(requiredNodeCapacity) * sizeof(GpuOctreeNodeNavData)));
        _device.d_octreeFirstChild = static_cast<int*>(bltzr_x::MemoryPool::allocate(
            static_cast<std::size_t>(requiredNodeCapacity) * sizeof(int)));
        _device.d_octreeLeafStarts = static_cast<int*>(bltzr_x::MemoryPool::allocate(
            static_cast<std::size_t>(requiredNodeCapacity) * sizeof(int)));
        _device.d_octreeLeafCounts = static_cast<int*>(bltzr_x::MemoryPool::allocate(
            static_cast<std::size_t>(requiredNodeCapacity) * sizeof(int)));

        if (!_device.g_dOctreeNodes || !_device.d_octreeNodeHot || !_device.d_octreeNodeNav || !_device.d_octreeFirstChild ||
            !_device.d_octreeLeafStarts || !_device.d_octreeLeafCounts) {
            if (_device.g_dOctreeNodes) {
                bltzr_x::MemoryPool::deallocate(_device.g_dOctreeNodes);
                _device.g_dOctreeNodes = nullptr;
            }
            if (_device.d_octreeNodeHot) {
                bltzr_x::MemoryPool::deallocate(_device.d_octreeNodeHot);
                _device.d_octreeNodeHot = nullptr;
            }
            if (_device.d_octreeNodeNav) {
                bltzr_x::MemoryPool::deallocate(_device.d_octreeNodeNav);
                _device.d_octreeNodeNav = nullptr;
            }
            if (_device.d_octreeFirstChild) {
                bltzr_x::MemoryPool::deallocate(_device.d_octreeFirstChild);
                _device.d_octreeFirstChild = nullptr;
            }
            if (_device.d_octreeLeafStarts) {
                bltzr_x::MemoryPool::deallocate(_device.d_octreeLeafStarts);
                _device.d_octreeLeafStarts = nullptr;
            }
            if (_device.d_octreeLeafCounts) {
                bltzr_x::MemoryPool::deallocate(_device.d_octreeLeafCounts);
                _device.d_octreeLeafCounts = nullptr;
            }
            _device.g_dOctreeNodeCapacity = 0;
            return false;
        }
        _device.g_dOctreeNodeCapacity = static_cast<std::size_t>(requiredNodeCapacity);
    }

    return true;
}

/*
 * @brief Documents the ensure energy scratch capacity operation contract.
 * @param numParticles Input value used by this contract.
 * @param sampleCount Input value used by this contract.
 * @return bool ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool ParticleSystem::ensureEnergyScratchCapacity(int numParticles, int sampleCount)
{
    if (!_device._cudaRuntimeAvailable) {
        return false;
    }
    if (numParticles <= 0 || sampleCount <= 0) {
        return false;
    }
    const int blockCount =
        (numParticles + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    if (_device.d_energyBlockCapacity < static_cast<std::size_t>(blockCount)) {
        if (_device.d_energyKineticBlocks) {
            bltzr_x::MemoryPool::deallocate(_device.d_energyKineticBlocks);
            _device.d_energyKineticBlocks = nullptr;
        }
        if (_device.d_energyThermalBlocks) {
            bltzr_x::MemoryPool::deallocate(_device.d_energyThermalBlocks);
            _device.d_energyThermalBlocks = nullptr;
        }
        _device.d_energyKineticBlocks = static_cast<float*>(bltzr_x::MemoryPool::allocate(
            static_cast<std::size_t>(blockCount) * sizeof(float)));
        _device.d_energyThermalBlocks = static_cast<float*>(bltzr_x::MemoryPool::allocate(
            static_cast<std::size_t>(blockCount) * sizeof(float)));
        if (!_device.d_energyKineticBlocks || !_device.d_energyThermalBlocks) {
            if (_device.d_energyKineticBlocks) {
                bltzr_x::MemoryPool::deallocate(_device.d_energyKineticBlocks);
                _device.d_energyKineticBlocks = nullptr;
            }
            if (_device.d_energyThermalBlocks) {
                bltzr_x::MemoryPool::deallocate(_device.d_energyThermalBlocks);
                _device.d_energyThermalBlocks = nullptr;
            }
            _device.d_energyBlockCapacity = 0;
            return false;
        }
        _device.d_energyBlockCapacity = static_cast<std::size_t>(blockCount);
    }

    if (_device.d_energySampleCapacity < static_cast<std::size_t>(sampleCount)) {
        if (_device.d_energyPotentialPartials) {
            bltzr_x::MemoryPool::deallocate(_device.d_energyPotentialPartials);
            _device.d_energyPotentialPartials = nullptr;
        }
        _device.d_energyPotentialPartials = static_cast<double*>(bltzr_x::MemoryPool::allocate(
            static_cast<std::size_t>(sampleCount) * sizeof(double)));
        if (!_device.d_energyPotentialPartials) {
            _device.d_energySampleCapacity = 0;
            return false;
        }
        _device.d_energySampleCapacity = static_cast<std::size_t>(sampleCount);
    }

    return true;
}

/*
 * @brief Documents the allocate rk4 buffers operation contract.
 * @param numParticles Input value used by this contract.
 * @return bool ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool ParticleSystem::allocateRk4Buffers(int numParticles)
{
    if (!_device._cudaRuntimeAvailable) {
        return false;
    }
    releaseRk4Buffers();
    _device.d_stage =
        static_cast<Particle*>(bltzr_x::MemoryPool::allocate(numParticles * sizeof(Particle)));
    _device.d_k1x =
        static_cast<Vector3*>(bltzr_x::MemoryPool::allocate(numParticles * sizeof(Vector3)));
    _device.d_k2x =
        static_cast<Vector3*>(bltzr_x::MemoryPool::allocate(numParticles * sizeof(Vector3)));
    _device.d_k3x =
        static_cast<Vector3*>(bltzr_x::MemoryPool::allocate(numParticles * sizeof(Vector3)));
    _device.d_k4x =
        static_cast<Vector3*>(bltzr_x::MemoryPool::allocate(numParticles * sizeof(Vector3)));
    _device.d_k1v =
        static_cast<Vector3*>(bltzr_x::MemoryPool::allocate(numParticles * sizeof(Vector3)));
    _device.d_k2v =
        static_cast<Vector3*>(bltzr_x::MemoryPool::allocate(numParticles * sizeof(Vector3)));
    _device.d_k3v =
        static_cast<Vector3*>(bltzr_x::MemoryPool::allocate(numParticles * sizeof(Vector3)));
    _device.d_k4v =
        static_cast<Vector3*>(bltzr_x::MemoryPool::allocate(numParticles * sizeof(Vector3)));
    if (_integratorMode == IntegratorMode::Leapfrog) {
        _device.d_vHalf = static_cast<GpuHalfVelocity*>(
            bltzr_x::MemoryPool::allocate(numParticles * sizeof(GpuHalfVelocity)));
    }

    if (!_device.d_stage || !_device.d_k1x || !_device.d_k2x || !_device.d_k3x || !_device.d_k4x || !_device.d_k1v || !_device.d_k2v || !_device.d_k3v || !_device.d_k4v ||
        (_integratorMode == IntegratorMode::Leapfrog && !_device.d_vHalf)) {
        releaseRk4Buffers();
        return false;
    }
    return true;
}

/*
 * @brief Documents the release rk4 buffers operation contract.
 * @param None This contract does not take explicit parameters.
 * @return void ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void ParticleSystem::releaseRk4Buffers()
{
    if (_device.d_stage) {
        bltzr_x::MemoryPool::deallocate(_device.d_stage);
        _device.d_stage = nullptr;
    }
    if (_device.d_k1x) {
        bltzr_x::MemoryPool::deallocate(_device.d_k1x);
        _device.d_k1x = nullptr;
    }
    if (_device.d_k2x) {
        bltzr_x::MemoryPool::deallocate(_device.d_k2x);
        _device.d_k2x = nullptr;
    }
    if (_device.d_k3x) {
        bltzr_x::MemoryPool::deallocate(_device.d_k3x);
        _device.d_k3x = nullptr;
    }
    if (_device.d_k4x) {
        bltzr_x::MemoryPool::deallocate(_device.d_k4x);
        _device.d_k4x = nullptr;
    }
    if (_device.d_k1v) {
        bltzr_x::MemoryPool::deallocate(_device.d_k1v);
        _device.d_k1v = nullptr;
    }
    if (_device.d_k2v) {
        bltzr_x::MemoryPool::deallocate(_device.d_k2v);
        _device.d_k2v = nullptr;
    }
    if (_device.d_k3v) {
        bltzr_x::MemoryPool::deallocate(_device.d_k3v);
        _device.d_k3v = nullptr;
    }
    if (_device.d_k4v) {
        bltzr_x::MemoryPool::deallocate(_device.d_k4v);
        _device.d_k4v = nullptr;
    }
    if (_device.d_vHalf) {
        bltzr_x::MemoryPool::deallocate(_device.d_vHalf);
        _device.d_vHalf = nullptr;
    }
}

/*
 * @brief Documents the allocate sph buffers operation contract.
 * @param numParticles Input value used by this contract.
 * @return bool ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool ParticleSystem::allocateSphBuffers(int numParticles)
{
    if (!_device._cudaRuntimeAvailable) {
        return false;
    }
    releaseSphBuffers();
    _device.d_sphDensity =
        static_cast<float*>(bltzr_x::MemoryPool::allocate(numParticles * sizeof(float)));
    _device.d_sphPressure =
        static_cast<float*>(bltzr_x::MemoryPool::allocate(numParticles * sizeof(float)));

    if (!_device.d_sphDensity || !_device.d_sphPressure) {
        releaseSphBuffers();
        return false;
    }
    return true;
}

/*
 * @brief Documents the release sph buffers operation contract.
 * @param None This contract does not take explicit parameters.
 * @return void ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void ParticleSystem::releaseSphBuffers()
{
    if (_device.d_sphDensity) {
        bltzr_x::MemoryPool::deallocate(_device.d_sphDensity);
        _device.d_sphDensity = nullptr;
    }
    if (_device.d_sphPressure) {
        bltzr_x::MemoryPool::deallocate(_device.d_sphPressure);
        _device.d_sphPressure = nullptr;
    }
}

/*
 * @brief Documents the allocate sph grid buffers operation contract.
 * @param numParticles Input value used by this contract.
 * @return bool ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool ParticleSystem::allocateSphGridBuffers(int numParticles)
{
    if (!_device._cudaRuntimeAvailable) {
        return false;
    }
    releaseSphGridBuffers();
    const std::size_t particleBytes = static_cast<std::size_t>(numParticles) * sizeof(int);
    _device.d_sphCellHash = static_cast<int*>(bltzr_x::MemoryPool::allocate(particleBytes));
    _device.d_sphSortedIndex = static_cast<int*>(bltzr_x::MemoryPool::allocate(particleBytes));

    if (!_device.d_sphCellHash || !_device.d_sphSortedIndex) {
        releaseSphGridBuffers();
        return false;
    }
    _hostCellHash.resize(numParticles);
    _hostSortedIndex.resize(numParticles);
    return true;
}

/*
 * @brief Documents the release sph grid buffers operation contract.
 * @param None This contract does not take explicit parameters.
 * @return void ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void ParticleSystem::releaseSphGridBuffers()
{
    if (_device.d_sphCellHash) {
        bltzr_x::MemoryPool::deallocate(_device.d_sphCellHash);
        _device.d_sphCellHash = nullptr;
    }
    if (_device.d_sphSortedIndex) {
        bltzr_x::MemoryPool::deallocate(_device.d_sphSortedIndex);
        _device.d_sphSortedIndex = nullptr;
    }
    if (_device.d_sphCellStart) {
        bltzr_x::MemoryPool::deallocate(_device.d_sphCellStart);
        _device.d_sphCellStart = nullptr;
    }
    if (_device.d_sphCellEnd) {
        bltzr_x::MemoryPool::deallocate(_device.d_sphCellEnd);
        _device.d_sphCellEnd = nullptr;
    }
    _hostCellHash.clear();
    _hostSortedIndex.clear();
    _device.d_treePmNeighborParticleCapacity = 0;
    _device.d_treePmNeighborCellCapacity = 0;
}

/*
 * @brief Documents the seed device state operation contract.
 * @param None This contract does not take explicit parameters.
 * @return bool ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool ParticleSystem::seedDeviceState()
{
    if (_particles.empty())
        return true;
    syncDeviceState();
    return true;
}
