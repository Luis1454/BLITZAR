// File: engine/src/physics/cuda/fragments/ParticleSystemBuffer.inl
// Purpose: Engine implementation for the BLITZAR simulation core.

/*
 * Module: physics/cuda
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

/// Description: Executes the bytesToMiB operation.
static double bytesToMiB(std::size_t bytes)
{
    return static_cast<double>(bytes) / (1024.0 * 1024.0);
}

/// Description: Executes the cudaRuntimeAvailable operation.
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
    /// Description: Executes the cudaGetLastError operation.
    cudaGetLastError();
    return false;
}

std::size_t ParticleSystem::estimateMemoryUsage(
    std::size_t particleCount,
    bool sphEnabled,
    SolverMode solverMode,
    IntegratorMode integratorMode,
    std::size_t energySampleLimit,
    int octreeLeafCapacity,
    std::size_t *baseAndIntegratorBytes,
    std::size_t *sphBytes,
    std::size_t *octreeBytes) const
{
    const std::size_t baseSoABytes = particleCount * sizeof(float) * 18u;

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

    std::size_t octreeBufferBytes = 0u;
    if (solverMode == SolverMode::OctreeGpu) {
        octreeBufferBytes += particleCount * 44u;

        const int defaultLeafCapacity = kDefaultOctreeLeafCapacity;
        const int configuredLeafCapacity = std::max(
            16,
            static_cast<int>(parseFloatEnv(
                "GRAVITY_LINEAR_OCTREE_LEAF_CAPACITY",
                static_cast<float>(defaultLeafCapacity))));
        const int leafCapacity = octreeLeafCapacity > 0
            ? std::max(16, octreeLeafCapacity)
            : configuredLeafCapacity;

        int leafDepth = 1;
        while (leafDepth < 21) {
            const double avgParticlesPerBucket =
                static_cast<double>(particleCount)
                / static_cast<double>(1ull << (3 * leafDepth));
            if (avgParticlesPerBucket <= static_cast<double>(leafCapacity)) {
                break;
            }
            ++leafDepth;
        }

        const int expectedLeaves = std::max(
            1,
            (static_cast<int>(particleCount) + leafCapacity - 1) / leafCapacity);
        const int requiredNodeCapacity =
            /// Description: Executes the max operation.
            std::max(2, expectedLeaves * (leafDepth + 1) * 4 + 8);
        octreeBufferBytes += static_cast<std::size_t>(requiredNodeCapacity) * sizeof(GpuOctreeNode);
        octreeBufferBytes += static_cast<std::size_t>(requiredNodeCapacity) * sizeof(GpuOctreeNodeHotData);
        octreeBufferBytes += static_cast<std::size_t>(requiredNodeCapacity) * sizeof(GpuOctreeNodeNavData);
        octreeBufferBytes += static_cast<std::size_t>(requiredNodeCapacity) * sizeof(int) * 3u;
    }

    const std::size_t boundedSample = std::max<std::size_t>(64u, std::min(particleCount, energySampleLimit));
    const std::size_t blockCount = (particleCount + Particle::kDefaultCudaBlockSize - 1u)
        / Particle::kDefaultCudaBlockSize;
    const std::size_t energyScratchBytes = blockCount * sizeof(float) * 2u
        + boundedSample * sizeof(double);

    const std::size_t fixedTelemetryBytes = sizeof(GpuSystemMetrics);
    const std::size_t total =
        baseSoABytes + integratorBytes + sphBufferBytes + octreeBufferBytes + energyScratchBytes + fixedTelemetryBytes;

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

std::string ParticleSystem::formatMemoryBreakdown(
    std::size_t baseAndIntegratorBytes,
    std::size_t sphBytes,
    std::size_t octreeBytes,
    std::size_t totalBytes,
    std::size_t budgetBytes)
{
    std::ostringstream out;
    /// Description: Executes the setprecision operation.
    out << std::fixed << std::setprecision(2)
        << "[info] [memory] Base/Integrator: " << bytesToMiB(baseAndIntegratorBytes) << " MB\n"
        << "[info] [memory] SPH Buffers: " << bytesToMiB(sphBytes) << " MB\n"
        << "[info] [memory] Octree Scratch/Nodes: " << bytesToMiB(octreeBytes) << " MB\n"
        << "[info] [memory] TOTAL: " << bytesToMiB(totalBytes) << " MB / " << bytesToMiB(budgetBytes)
        << " MB (" << (100.0 * static_cast<double>(totalBytes) / static_cast<double>(budgetBytes)) << "%)";
    return out.str();
}

/// Description: Executes the initializeRuntimeState operation.
void ParticleSystem::initializeRuntimeState(std::size_t particleCapacity)
{
    _cudaRuntimeAvailable = cudaRuntimeAvailable();
    if (_cudaRuntimeAvailable) {
        const cudaError_t mapHostStatus = cudaSetDeviceFlags(cudaDeviceMapHost);
        if (mapHostStatus != cudaSuccess && mapHostStatus != cudaErrorSetOnActiveProcess) {
            /// Description: Executes the checkCudaStatus operation.
            checkCudaStatus(mapHostStatus, "cudaSetDeviceFlags(cudaDeviceMapHost)");
            _cudaRuntimeAvailable = false;
        }
    }
    if (_cudaRuntimeAvailable) {
        /// Description: Executes the initialize operation.
        grav_x::CudaMemoryPool::initialize();
    }
    _solverMode = solverModeFromEnv();
    _integratorMode = integratorModeFromEnv();
    _octreeTheta = parseFloatEnv("GRAVITY_OCTREE_THETA", 1.2f);
    _octreeOpeningCriterion = OctreeOpeningCriterion::CenterOfMass;
    _octreeSoftening = parseFloatEnv("GRAVITY_OCTREE_SOFTENING", 2.5f);
    _sphEnabled = parseBoolEnv("GRAVITY_SPH_ENABLED", false);
    _sphSmoothingLength = parseFloatEnv("GRAVITY_SPH_H", 1.25f);
    _sphRestDensity = parseFloatEnv("GRAVITY_SPH_REST_DENSITY", 1.0f);
    _sphGasConstant = parseFloatEnv("GRAVITY_SPH_GAS_CONSTANT", 4.0f);
    _sphViscosity = parseFloatEnv("GRAVITY_SPH_VISCOSITY", 0.08f);
    _thermalAmbientTemperature = parseFloatEnv("GRAVITY_THERMAL_AMBIENT", 0.0f);
    _thermalSpecificHeat = parseFloatEnv("GRAVITY_THERMAL_SPECIFIC_HEAT", 1.0f);
    _thermalHeatingCoeff = parseFloatEnv("GRAVITY_THERMAL_HEATING", 0.0002f);
    _thermalRadiationCoeff = parseFloatEnv("GRAVITY_THERMAL_RADIATION", 0.00000001f);
    _physicsMaxAcceleration = 64.0f;
    _physicsMinSoftening = 1e-4f;
    _physicsMinDistance2 = 1e-12f;
    _physicsMinTheta = 0.05f;
    _sphMaxAcceleration = 40.0f;
    _sphMaxSpeed = 120.0f;
    _cumulativeRadiatedEnergy = 0.0f;
    _sphGridSize = 0;
    _sphGridTotalCells = 0;
    _deviceParticleCapacity = particleCapacity;
    _hostStateDirty = false;
    d_soaPosX = nullptr;
    d_soaPosY = nullptr;
    d_soaPosZ = nullptr;
    d_soaVelX = nullptr;
    d_soaVelY = nullptr;
    d_soaVelZ = nullptr;
    d_soaPressX = nullptr;
    d_soaPressY = nullptr;
    d_soaPressZ = nullptr;
    d_soaMass = nullptr;
    d_soaTemp = nullptr;
    d_soaDens = nullptr;
    d_soaNextPosX = nullptr;
    d_soaNextPosY = nullptr;
    d_soaNextPosZ = nullptr;
    d_soaNextVelX = nullptr;
    d_soaNextVelY = nullptr;
    d_soaNextVelZ = nullptr;
    d_stage = nullptr;
    d_k1x = nullptr; d_k2x = nullptr; d_k3x = nullptr; d_k4x = nullptr;
    d_k1v = nullptr; d_k2v = nullptr; d_k3v = nullptr; d_k4v = nullptr;
    d_vHalf = nullptr;
    _leapfrogPrimed = false;
    d_sphDensity = nullptr;
    d_sphPressure = nullptr;
    d_sphCellHash = nullptr;
    d_sphSortedIndex = nullptr;
    d_sphCellStart = nullptr;
    d_sphCellEnd = nullptr;
    g_dOctreeNodes = nullptr;
    g_dOctreeLeafIndices = nullptr;
    d_octreeMortonKeys = nullptr;
    d_octreePrefixesA = nullptr;
    d_octreePrefixesB = nullptr;
    d_octreeLevelIndicesA = nullptr;
    d_octreeLevelIndicesB = nullptr;
    d_octreeParentCounts = nullptr;
    d_octreeParentOffsets = nullptr;
    d_octreeNodeHot = nullptr;
    d_octreeNodeNav = nullptr;
    d_octreeFirstChild = nullptr;
    d_octreeLeafStarts = nullptr;
    d_octreeLeafCounts = nullptr;
    d_energyKineticBlocks = nullptr;
    d_energyThermalBlocks = nullptr;
    d_energyPotentialPartials = nullptr;
    g_dOctreeNodeCapacity = 0;
    g_dOctreeLeafCapacity = 0;
    d_octreeMortonCapacity = 0;
    d_octreePrefixCapacity = 0;
    d_octreeLevelCapacity = 0;
    d_energyBlockCapacity = 0;
    d_energySampleCapacity = 0;
    _gpuOctreeRootIndex = -1;
    _gpuOctreeNodeCount = 0;
    _gpuOctreeLeafCount = 0;
    _mappedMetricsHost = nullptr;
    _mappedMetricsDevice = nullptr;
    _metricsStepId = 0u;
    _metricsSimTime = 0.0f;
    _linearOctreeLeafCapacity = std::max(
        16,
        static_cast<int>(parseFloatEnv(
            "GRAVITY_LINEAR_OCTREE_LEAF_CAPACITY",
            static_cast<float>(kDefaultOctreeLeafCapacity))));

    const bool strictMemoryMode = parseBoolEnv("GRAVITY_STRICT_MEMORY_MODE", false);
    std::size_t selectedEnergySampleLimit = kDefaultEnergySampleLimit;
    int selectedOctreeLeafCapacity = _linearOctreeLeafCapacity;

    std::size_t baseAndIntegratorBytes = 0u;
    std::size_t sphBytes = 0u;
    std::size_t octreeBytes = 0u;
    std::size_t totalBytes = estimateMemoryUsage(
        particleCapacity,
        _sphEnabled,
        _solverMode,
        _integratorMode,
        selectedEnergySampleLimit,
        selectedOctreeLeafCapacity,
        &baseAndIntegratorBytes,
        &sphBytes,
        &octreeBytes);

    std::cout << formatMemoryBreakdown(
        baseAndIntegratorBytes,
        sphBytes,
        octreeBytes,
        totalBytes,
        kVramBudgetBytes)
              << "\n";

    if (_solverMode == SolverMode::OctreeGpu
        && totalBytes > kVramBudgetBytes
        && selectedOctreeLeafCapacity < kPlanBOctreeLeafCapacity) {
        selectedOctreeLeafCapacity = kPlanBOctreeLeafCapacity;
        _linearOctreeLeafCapacity = selectedOctreeLeafCapacity;
        totalBytes = estimateMemoryUsage(
            particleCapacity,
            _sphEnabled,
            _solverMode,
            _integratorMode,
            selectedEnergySampleLimit,
            selectedOctreeLeafCapacity,
            &baseAndIntegratorBytes,
            &sphBytes,
            &octreeBytes);
        std::cout << "[info] dynamic octree leaf capacity bump to "
                  << selectedOctreeLeafCapacity
                  << " after VRAM estimate exceeded budget\n";
        std::cout << formatMemoryBreakdown(
                      baseAndIntegratorBytes,
                      sphBytes,
                      octreeBytes,
                      totalBytes,
                      kVramBudgetBytes)
                  << "\n";
    }

    if (strictMemoryMode && totalBytes > kVramBudgetBytes) {
        selectedEnergySampleLimit = kPlanAEnergySampleLimit;
        totalBytes = estimateMemoryUsage(
            particleCapacity,
            _sphEnabled,
            _solverMode,
            _integratorMode,
            selectedEnergySampleLimit,
            selectedOctreeLeafCapacity,
            &baseAndIntegratorBytes,
            &sphBytes,
            &octreeBytes);
        std::cout << "[info] strict memory plan A (energy sample limit="
                  << selectedEnergySampleLimit << ")\n";
        std::cout << formatMemoryBreakdown(
                      baseAndIntegratorBytes,
                      sphBytes,
                      octreeBytes,
                      totalBytes,
                      kVramBudgetBytes)
                  << "\n";
    }

    if (strictMemoryMode && totalBytes > kVramBudgetBytes && _solverMode == SolverMode::OctreeGpu) {
        selectedOctreeLeafCapacity = kPlanBOctreeLeafCapacity;
        totalBytes = estimateMemoryUsage(
            particleCapacity,
            _sphEnabled,
            _solverMode,
            _integratorMode,
            selectedEnergySampleLimit,
            selectedOctreeLeafCapacity,
            &baseAndIntegratorBytes,
            &sphBytes,
            &octreeBytes);
        std::cout << "[info] strict memory plan B (octree leaf capacity="
                  << selectedOctreeLeafCapacity << ")\n";
        std::cout << formatMemoryBreakdown(
                      baseAndIntegratorBytes,
                      sphBytes,
                      octreeBytes,
                      totalBytes,
                      kVramBudgetBytes)
                  << "\n";
    }

    if (strictMemoryMode && totalBytes > kVramBudgetBytes) {
        throw std::runtime_error(
            /// Description: Executes the string operation.
            std::string("[memory] strict admission-control rejected configuration\n")
            + formatMemoryBreakdown(
                baseAndIntegratorBytes,
                sphBytes,
                octreeBytes,
                totalBytes,
                kVramBudgetBytes)
            + "\n[memory] plan Z: strict mode abort");
    }

    if (_cudaRuntimeAvailable) {
        /// Description: Executes the allocateMappedMetrics operation.
        allocateMappedMetrics();
    }
}

/// Description: Executes the allocateParticleBuffers operation.
bool ParticleSystem::allocateParticleBuffers(std::size_t particleCapacity)
{
    if (!_cudaRuntimeAvailable) {
        return false;
    }
    const std::size_t bytesTotal = particleCapacity * sizeof(float);
    bool ok = true;
    ok &= ((d_soaPosX = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaPosY = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaPosZ = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaVelX = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaVelY = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaVelZ = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaPressX = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaPressY = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaPressZ = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaMass = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaTemp = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaDens = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaNextPosX = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaNextPosY = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaNextPosZ = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaNextVelX = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaNextVelY = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaNextVelZ = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    
    if (!ok) {
        /// Description: Executes the releaseParticleBuffers operation.
        releaseParticleBuffers();
        return false;
    }
    return true;
}

/// Description: Executes the releaseParticleBuffers operation.
void ParticleSystem::releaseParticleBuffers()
{
    /// Description: Executes the releaseRk4Buffers operation.
    releaseRk4Buffers();
    /// Description: Executes the releaseSphBuffers operation.
    releaseSphBuffers();
    /// Description: Executes the releaseSphGridBuffers operation.
    releaseSphGridBuffers();
    
    /// Description: Executes the deallocate operation.
    grav_x::CudaMemoryPool::deallocate(d_soaPosX); d_soaPosX = nullptr;
    /// Description: Executes the deallocate operation.
    grav_x::CudaMemoryPool::deallocate(d_soaPosY); d_soaPosY = nullptr;
    /// Description: Executes the deallocate operation.
    grav_x::CudaMemoryPool::deallocate(d_soaPosZ); d_soaPosZ = nullptr;
    /// Description: Executes the deallocate operation.
    grav_x::CudaMemoryPool::deallocate(d_soaVelX); d_soaVelX = nullptr;
    /// Description: Executes the deallocate operation.
    grav_x::CudaMemoryPool::deallocate(d_soaVelY); d_soaVelY = nullptr;
    /// Description: Executes the deallocate operation.
    grav_x::CudaMemoryPool::deallocate(d_soaVelZ); d_soaVelZ = nullptr;
    /// Description: Executes the deallocate operation.
    grav_x::CudaMemoryPool::deallocate(d_soaPressX); d_soaPressX = nullptr;
    /// Description: Executes the deallocate operation.
    grav_x::CudaMemoryPool::deallocate(d_soaPressY); d_soaPressY = nullptr;
    /// Description: Executes the deallocate operation.
    grav_x::CudaMemoryPool::deallocate(d_soaPressZ); d_soaPressZ = nullptr;
    /// Description: Executes the deallocate operation.
    grav_x::CudaMemoryPool::deallocate(d_soaMass); d_soaMass = nullptr;
    /// Description: Executes the deallocate operation.
    grav_x::CudaMemoryPool::deallocate(d_soaTemp); d_soaTemp = nullptr;
    /// Description: Executes the deallocate operation.
    grav_x::CudaMemoryPool::deallocate(d_soaDens); d_soaDens = nullptr;
    /// Description: Executes the deallocate operation.
    grav_x::CudaMemoryPool::deallocate(d_soaNextPosX); d_soaNextPosX = nullptr;
    /// Description: Executes the deallocate operation.
    grav_x::CudaMemoryPool::deallocate(d_soaNextPosY); d_soaNextPosY = nullptr;
    /// Description: Executes the deallocate operation.
    grav_x::CudaMemoryPool::deallocate(d_soaNextPosZ); d_soaNextPosZ = nullptr;
    /// Description: Executes the deallocate operation.
    grav_x::CudaMemoryPool::deallocate(d_soaNextVelX); d_soaNextVelX = nullptr;
    /// Description: Executes the deallocate operation.
    grav_x::CudaMemoryPool::deallocate(d_soaNextVelY); d_soaNextVelY = nullptr;
    /// Description: Executes the deallocate operation.
    grav_x::CudaMemoryPool::deallocate(d_soaNextVelZ); d_soaNextVelZ = nullptr;
    /// Description: Executes the deallocate operation.
    grav_x::CudaMemoryPool::deallocate(d_vHalf); d_vHalf = nullptr;
    _leapfrogPrimed = false;

    if (g_dOctreeNodes) {
        /// Description: Executes the deallocate operation.
        grav_x::CudaMemoryPool::deallocate(g_dOctreeNodes);
        g_dOctreeNodes = nullptr;
    }
    if (g_dOctreeLeafIndices) {
        /// Description: Executes the deallocate operation.
        grav_x::CudaMemoryPool::deallocate(g_dOctreeLeafIndices);
        g_dOctreeLeafIndices = nullptr;
    }
    if (d_octreeMortonKeys) {
        /// Description: Executes the deallocate operation.
        grav_x::CudaMemoryPool::deallocate(d_octreeMortonKeys);
        d_octreeMortonKeys = nullptr;
    }
    if (d_octreePrefixesA) {
        /// Description: Executes the deallocate operation.
        grav_x::CudaMemoryPool::deallocate(d_octreePrefixesA);
        d_octreePrefixesA = nullptr;
    }
    if (d_octreePrefixesB) {
        /// Description: Executes the deallocate operation.
        grav_x::CudaMemoryPool::deallocate(d_octreePrefixesB);
        d_octreePrefixesB = nullptr;
    }
    if (d_octreeLevelIndicesA) {
        /// Description: Executes the deallocate operation.
        grav_x::CudaMemoryPool::deallocate(d_octreeLevelIndicesA);
        d_octreeLevelIndicesA = nullptr;
    }
    if (d_octreeLevelIndicesB) {
        /// Description: Executes the deallocate operation.
        grav_x::CudaMemoryPool::deallocate(d_octreeLevelIndicesB);
        d_octreeLevelIndicesB = nullptr;
    }
    if (d_octreeParentCounts) {
        /// Description: Executes the deallocate operation.
        grav_x::CudaMemoryPool::deallocate(d_octreeParentCounts);
        d_octreeParentCounts = nullptr;
    }
    if (d_octreeParentOffsets) {
        /// Description: Executes the deallocate operation.
        grav_x::CudaMemoryPool::deallocate(d_octreeParentOffsets);
        d_octreeParentOffsets = nullptr;
    }
    if (d_octreeNodeHot) {
        /// Description: Executes the deallocate operation.
        grav_x::CudaMemoryPool::deallocate(d_octreeNodeHot);
        d_octreeNodeHot = nullptr;
    }
    if (d_octreeNodeNav) {
        /// Description: Executes the deallocate operation.
        grav_x::CudaMemoryPool::deallocate(d_octreeNodeNav);
        d_octreeNodeNav = nullptr;
    }
    if (d_octreeFirstChild) {
        /// Description: Executes the deallocate operation.
        grav_x::CudaMemoryPool::deallocate(d_octreeFirstChild);
        d_octreeFirstChild = nullptr;
    }
    if (d_octreeLeafStarts) {
        /// Description: Executes the deallocate operation.
        grav_x::CudaMemoryPool::deallocate(d_octreeLeafStarts);
        d_octreeLeafStarts = nullptr;
    }
    if (d_octreeLeafCounts) {
        /// Description: Executes the deallocate operation.
        grav_x::CudaMemoryPool::deallocate(d_octreeLeafCounts);
        d_octreeLeafCounts = nullptr;
    }
    if (d_energyKineticBlocks) {
        /// Description: Executes the deallocate operation.
        grav_x::CudaMemoryPool::deallocate(d_energyKineticBlocks);
        d_energyKineticBlocks = nullptr;
    }
    if (d_energyThermalBlocks) {
        /// Description: Executes the deallocate operation.
        grav_x::CudaMemoryPool::deallocate(d_energyThermalBlocks);
        d_energyThermalBlocks = nullptr;
    }
    if (d_energyPotentialPartials) {
        /// Description: Executes the deallocate operation.
        grav_x::CudaMemoryPool::deallocate(d_energyPotentialPartials);
        d_energyPotentialPartials = nullptr;
    }
    _deviceParticleCapacity = 0;
    g_dOctreeNodeCapacity = 0;
    g_dOctreeLeafCapacity = 0;
    d_octreeMortonCapacity = 0;
    d_octreePrefixCapacity = 0;
    d_octreeLevelCapacity = 0;
    d_energyBlockCapacity = 0;
    d_energySampleCapacity = 0;
    _gpuOctreeRootIndex = -1;
    _gpuOctreeNodeCount = 0;
    _gpuOctreeLeafCount = 0;
    /// Description: Executes the releaseMappedMetrics operation.
    releaseMappedMetrics();
}

/// Description: Executes the allocateMappedMetrics operation.
bool ParticleSystem::allocateMappedMetrics()
{
    if (!_cudaRuntimeAvailable) {
        return false;
    }
    /// Description: Executes the releaseMappedMetrics operation.
    releaseMappedMetrics();
    void *hostPtr = nullptr;
    cudaError_t status = cudaHostAlloc(
        &hostPtr,
        sizeof(GpuSystemMetrics),
        cudaHostAllocMapped | cudaHostAllocPortable);
    if (!checkCudaStatus(status, "cudaHostAlloc(mapped metrics)")) {
        return false;
    }

    /// Description: Executes the memset operation.
    std::memset(hostPtr, 0, sizeof(GpuSystemMetrics));
    _mappedMetricsHost = static_cast<GpuSystemMetrics *>(hostPtr);

    void *devicePtr = nullptr;
    status = cudaHostGetDevicePointer(&devicePtr, hostPtr, 0);
    if (!checkCudaStatus(status, "cudaHostGetDevicePointer(mapped metrics)")) {
        /// Description: Executes the cudaFreeHost operation.
        cudaFreeHost(hostPtr);
        _mappedMetricsHost = nullptr;
        return false;
    }
    _mappedMetricsDevice = static_cast<GpuSystemMetrics *>(devicePtr);
    _metricsStepId = 0u;
    _metricsSimTime = 0.0f;
    return true;
}

/// Description: Executes the releaseMappedMetrics operation.
void ParticleSystem::releaseMappedMetrics()
{
    if (_mappedMetricsHost != nullptr) {
        /// Description: Executes the cudaFreeHost operation.
        cudaFreeHost(_mappedMetricsHost);
    }
    _mappedMetricsHost = nullptr;
    _mappedMetricsDevice = nullptr;
    _metricsStepId = 0u;
    _metricsSimTime = 0.0f;
}

/// Description: Executes the ensureLinearOctreeScratchCapacity operation.
bool ParticleSystem::ensureLinearOctreeScratchCapacity(int numParticles)
{
    if (!_cudaRuntimeAvailable) {
        return false;
    }
    if (numParticles <= 0) {
        return false;
    }

    const int leafCapacity = std::max(16, _linearOctreeLeafCapacity);

    int leafDepth = 1;
    while (leafDepth < 21) {
        const double avgParticlesPerBucket =
            static_cast<double>(numParticles) / static_cast<double>(1ull << (3 * leafDepth));
        if (avgParticlesPerBucket <= static_cast<double>(leafCapacity)) {
            break;
        }
        ++leafDepth;
    }

    if (g_dOctreeLeafCapacity < static_cast<std::size_t>(numParticles)) {
        if (g_dOctreeLeafIndices) {
            /// Description: Executes the deallocate operation.
            grav_x::CudaMemoryPool::deallocate(g_dOctreeLeafIndices);
            g_dOctreeLeafIndices = nullptr;
        }
        g_dOctreeLeafIndices = static_cast<int*>(
            /// Description: Executes the allocate operation.
            grav_x::CudaMemoryPool::allocate(static_cast<std::size_t>(numParticles) * sizeof(int)));
        if (!g_dOctreeLeafIndices) {
            g_dOctreeLeafCapacity = 0;
            return false;
        }
        g_dOctreeLeafCapacity = static_cast<std::size_t>(numParticles);
    }

    if (d_octreeMortonCapacity < static_cast<std::size_t>(numParticles)) {
        if (d_octreeMortonKeys) {
            /// Description: Executes the deallocate operation.
            grav_x::CudaMemoryPool::deallocate(d_octreeMortonKeys);
            d_octreeMortonKeys = nullptr;
        }
        d_octreeMortonKeys = static_cast<unsigned long long*>(grav_x::CudaMemoryPool::allocate(
            static_cast<std::size_t>(numParticles) * sizeof(unsigned long long)));
        if (!d_octreeMortonKeys) {
            d_octreeMortonCapacity = 0;
            return false;
        }
        d_octreeMortonCapacity = static_cast<std::size_t>(numParticles);
    }

    if (d_octreePrefixCapacity < static_cast<std::size_t>(numParticles)) {
        if (d_octreePrefixesA) {
            /// Description: Executes the deallocate operation.
            grav_x::CudaMemoryPool::deallocate(d_octreePrefixesA);
            d_octreePrefixesA = nullptr;
        }
        if (d_octreePrefixesB) {
            /// Description: Executes the deallocate operation.
            grav_x::CudaMemoryPool::deallocate(d_octreePrefixesB);
            d_octreePrefixesB = nullptr;
        }
        d_octreePrefixesA = static_cast<unsigned long long*>(grav_x::CudaMemoryPool::allocate(
            static_cast<std::size_t>(numParticles) * sizeof(unsigned long long)));
        d_octreePrefixesB = static_cast<unsigned long long*>(grav_x::CudaMemoryPool::allocate(
            static_cast<std::size_t>(numParticles) * sizeof(unsigned long long)));
        if (!d_octreePrefixesA || !d_octreePrefixesB) {
            if (d_octreePrefixesA) {
                /// Description: Executes the deallocate operation.
                grav_x::CudaMemoryPool::deallocate(d_octreePrefixesA);
                d_octreePrefixesA = nullptr;
            }
            if (d_octreePrefixesB) {
                /// Description: Executes the deallocate operation.
                grav_x::CudaMemoryPool::deallocate(d_octreePrefixesB);
                d_octreePrefixesB = nullptr;
            }
            d_octreePrefixCapacity = 0;
            return false;
        }
        d_octreePrefixCapacity = static_cast<std::size_t>(numParticles);
    }

    if (d_octreeLevelCapacity < static_cast<std::size_t>(numParticles)) {
        if (d_octreeLevelIndicesA) {
            /// Description: Executes the deallocate operation.
            grav_x::CudaMemoryPool::deallocate(d_octreeLevelIndicesA);
            d_octreeLevelIndicesA = nullptr;
        }
        if (d_octreeLevelIndicesB) {
            /// Description: Executes the deallocate operation.
            grav_x::CudaMemoryPool::deallocate(d_octreeLevelIndicesB);
            d_octreeLevelIndicesB = nullptr;
        }
        if (d_octreeParentCounts) {
            /// Description: Executes the deallocate operation.
            grav_x::CudaMemoryPool::deallocate(d_octreeParentCounts);
            d_octreeParentCounts = nullptr;
        }
        if (d_octreeParentOffsets) {
            /// Description: Executes the deallocate operation.
            grav_x::CudaMemoryPool::deallocate(d_octreeParentOffsets);
            d_octreeParentOffsets = nullptr;
        }

        d_octreeLevelIndicesA = static_cast<int*>(
            /// Description: Executes the allocate operation.
            grav_x::CudaMemoryPool::allocate(static_cast<std::size_t>(numParticles) * sizeof(int)));
        d_octreeLevelIndicesB = static_cast<int*>(
            /// Description: Executes the allocate operation.
            grav_x::CudaMemoryPool::allocate(static_cast<std::size_t>(numParticles) * sizeof(int)));
        d_octreeParentCounts = static_cast<int*>(
            /// Description: Executes the allocate operation.
            grav_x::CudaMemoryPool::allocate(static_cast<std::size_t>(numParticles) * sizeof(int)));
        d_octreeParentOffsets = static_cast<int*>(
            /// Description: Executes the allocate operation.
            grav_x::CudaMemoryPool::allocate(static_cast<std::size_t>(numParticles) * sizeof(int)));
        if (!d_octreeLevelIndicesA || !d_octreeLevelIndicesB || !d_octreeParentCounts ||
            !d_octreeParentOffsets) {
            if (d_octreeLevelIndicesA) {
                /// Description: Executes the deallocate operation.
                grav_x::CudaMemoryPool::deallocate(d_octreeLevelIndicesA);
                d_octreeLevelIndicesA = nullptr;
            }
            if (d_octreeLevelIndicesB) {
                /// Description: Executes the deallocate operation.
                grav_x::CudaMemoryPool::deallocate(d_octreeLevelIndicesB);
                d_octreeLevelIndicesB = nullptr;
            }
            if (d_octreeParentCounts) {
                /// Description: Executes the deallocate operation.
                grav_x::CudaMemoryPool::deallocate(d_octreeParentCounts);
                d_octreeParentCounts = nullptr;
            }
            if (d_octreeParentOffsets) {
                /// Description: Executes the deallocate operation.
                grav_x::CudaMemoryPool::deallocate(d_octreeParentOffsets);
                d_octreeParentOffsets = nullptr;
            }
            d_octreeLevelCapacity = 0;
            return false;
        }
        d_octreeLevelCapacity = static_cast<std::size_t>(numParticles);
    }

    const int expectedLeaves = std::max(1, (numParticles + leafCapacity - 1) / leafCapacity);
    const int requiredNodeCapacity = std::max(2, expectedLeaves * (leafDepth + 1) * 4 + 8);
    if (g_dOctreeNodeCapacity < static_cast<std::size_t>(requiredNodeCapacity)) {
        if (g_dOctreeNodes) {
            /// Description: Executes the deallocate operation.
            grav_x::CudaMemoryPool::deallocate(g_dOctreeNodes);
            g_dOctreeNodes = nullptr;
        }
        if (d_octreeNodeHot) {
            /// Description: Executes the deallocate operation.
            grav_x::CudaMemoryPool::deallocate(d_octreeNodeHot);
            d_octreeNodeHot = nullptr;
        }
        if (d_octreeNodeNav) {
            /// Description: Executes the deallocate operation.
            grav_x::CudaMemoryPool::deallocate(d_octreeNodeNav);
            d_octreeNodeNav = nullptr;
        }
        if (d_octreeFirstChild) {
            /// Description: Executes the deallocate operation.
            grav_x::CudaMemoryPool::deallocate(d_octreeFirstChild);
            d_octreeFirstChild = nullptr;
        }
        if (d_octreeLeafStarts) {
            /// Description: Executes the deallocate operation.
            grav_x::CudaMemoryPool::deallocate(d_octreeLeafStarts);
            d_octreeLeafStarts = nullptr;
        }
        if (d_octreeLeafCounts) {
            /// Description: Executes the deallocate operation.
            grav_x::CudaMemoryPool::deallocate(d_octreeLeafCounts);
            d_octreeLeafCounts = nullptr;
        }

        g_dOctreeNodes = static_cast<GpuOctreeNode*>(grav_x::CudaMemoryPool::allocate(
            static_cast<std::size_t>(requiredNodeCapacity) * sizeof(GpuOctreeNode)));
        d_octreeNodeHot = static_cast<GpuOctreeNodeHotData*>(grav_x::CudaMemoryPool::allocate(
            static_cast<std::size_t>(requiredNodeCapacity) * sizeof(GpuOctreeNodeHotData)));
        d_octreeNodeNav = static_cast<GpuOctreeNodeNavData*>(grav_x::CudaMemoryPool::allocate(
            static_cast<std::size_t>(requiredNodeCapacity) * sizeof(GpuOctreeNodeNavData)));
        d_octreeFirstChild = static_cast<int*>(grav_x::CudaMemoryPool::allocate(
            static_cast<std::size_t>(requiredNodeCapacity) * sizeof(int)));
        d_octreeLeafStarts = static_cast<int*>(grav_x::CudaMemoryPool::allocate(
            static_cast<std::size_t>(requiredNodeCapacity) * sizeof(int)));
        d_octreeLeafCounts = static_cast<int*>(grav_x::CudaMemoryPool::allocate(
            static_cast<std::size_t>(requiredNodeCapacity) * sizeof(int)));

        if (!g_dOctreeNodes || !d_octreeNodeHot || !d_octreeNodeNav || !d_octreeFirstChild ||
            !d_octreeLeafStarts || !d_octreeLeafCounts) {
            if (g_dOctreeNodes) {
                /// Description: Executes the deallocate operation.
                grav_x::CudaMemoryPool::deallocate(g_dOctreeNodes);
                g_dOctreeNodes = nullptr;
            }
            if (d_octreeNodeHot) {
                /// Description: Executes the deallocate operation.
                grav_x::CudaMemoryPool::deallocate(d_octreeNodeHot);
                d_octreeNodeHot = nullptr;
            }
            if (d_octreeNodeNav) {
                /// Description: Executes the deallocate operation.
                grav_x::CudaMemoryPool::deallocate(d_octreeNodeNav);
                d_octreeNodeNav = nullptr;
            }
            if (d_octreeFirstChild) {
                /// Description: Executes the deallocate operation.
                grav_x::CudaMemoryPool::deallocate(d_octreeFirstChild);
                d_octreeFirstChild = nullptr;
            }
            if (d_octreeLeafStarts) {
                /// Description: Executes the deallocate operation.
                grav_x::CudaMemoryPool::deallocate(d_octreeLeafStarts);
                d_octreeLeafStarts = nullptr;
            }
            if (d_octreeLeafCounts) {
                /// Description: Executes the deallocate operation.
                grav_x::CudaMemoryPool::deallocate(d_octreeLeafCounts);
                d_octreeLeafCounts = nullptr;
            }
            g_dOctreeNodeCapacity = 0;
            return false;
        }
        g_dOctreeNodeCapacity = static_cast<std::size_t>(requiredNodeCapacity);
    }

    return true;
}

/// Description: Executes the ensureEnergyScratchCapacity operation.
bool ParticleSystem::ensureEnergyScratchCapacity(int numParticles, int sampleCount)
{
    if (!_cudaRuntimeAvailable) {
        return false;
    }
    if (numParticles <= 0 || sampleCount <= 0) {
        return false;
    }
    const int blockCount =
        (numParticles + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    if (d_energyBlockCapacity < static_cast<std::size_t>(blockCount)) {
        if (d_energyKineticBlocks) {
            /// Description: Executes the deallocate operation.
            grav_x::CudaMemoryPool::deallocate(d_energyKineticBlocks);
            d_energyKineticBlocks = nullptr;
        }
        if (d_energyThermalBlocks) {
            /// Description: Executes the deallocate operation.
            grav_x::CudaMemoryPool::deallocate(d_energyThermalBlocks);
            d_energyThermalBlocks = nullptr;
        }
        d_energyKineticBlocks = static_cast<float*>(grav_x::CudaMemoryPool::allocate(
            static_cast<std::size_t>(blockCount) * sizeof(float)));
        d_energyThermalBlocks = static_cast<float*>(grav_x::CudaMemoryPool::allocate(
            static_cast<std::size_t>(blockCount) * sizeof(float)));
        if (!d_energyKineticBlocks || !d_energyThermalBlocks) {
            if (d_energyKineticBlocks) {
                /// Description: Executes the deallocate operation.
                grav_x::CudaMemoryPool::deallocate(d_energyKineticBlocks);
                d_energyKineticBlocks = nullptr;
            }
            if (d_energyThermalBlocks) {
                /// Description: Executes the deallocate operation.
                grav_x::CudaMemoryPool::deallocate(d_energyThermalBlocks);
                d_energyThermalBlocks = nullptr;
            }
            d_energyBlockCapacity = 0;
            return false;
        }
        d_energyBlockCapacity = static_cast<std::size_t>(blockCount);
    }

    if (d_energySampleCapacity < static_cast<std::size_t>(sampleCount)) {
        if (d_energyPotentialPartials) {
            /// Description: Executes the deallocate operation.
            grav_x::CudaMemoryPool::deallocate(d_energyPotentialPartials);
            d_energyPotentialPartials = nullptr;
        }
        d_energyPotentialPartials = static_cast<double*>(grav_x::CudaMemoryPool::allocate(
            static_cast<std::size_t>(sampleCount) * sizeof(double)));
        if (!d_energyPotentialPartials) {
            d_energySampleCapacity = 0;
            return false;
        }
        d_energySampleCapacity = static_cast<std::size_t>(sampleCount);
    }

    return true;
}

/// Description: Executes the allocateRk4Buffers operation.
bool ParticleSystem::allocateRk4Buffers(int numParticles)
{
    if (!_cudaRuntimeAvailable) {
        return false;
    }
    /// Description: Executes the releaseRk4Buffers operation.
    releaseRk4Buffers();
    d_stage = static_cast<Particle*>(grav_x::CudaMemoryPool::allocate(numParticles * sizeof(Particle)));
    d_k1x = static_cast<Vector3*>(grav_x::CudaMemoryPool::allocate(numParticles * sizeof(Vector3)));
    d_k2x = static_cast<Vector3*>(grav_x::CudaMemoryPool::allocate(numParticles * sizeof(Vector3)));
    d_k3x = static_cast<Vector3*>(grav_x::CudaMemoryPool::allocate(numParticles * sizeof(Vector3)));
    d_k4x = static_cast<Vector3*>(grav_x::CudaMemoryPool::allocate(numParticles * sizeof(Vector3)));
    d_k1v = static_cast<Vector3*>(grav_x::CudaMemoryPool::allocate(numParticles * sizeof(Vector3)));
    d_k2v = static_cast<Vector3*>(grav_x::CudaMemoryPool::allocate(numParticles * sizeof(Vector3)));
    d_k3v = static_cast<Vector3*>(grav_x::CudaMemoryPool::allocate(numParticles * sizeof(Vector3)));
    d_k4v = static_cast<Vector3*>(grav_x::CudaMemoryPool::allocate(numParticles * sizeof(Vector3)));
    if (_integratorMode == IntegratorMode::Leapfrog) {
        d_vHalf = static_cast<float3*>(grav_x::CudaMemoryPool::allocate(numParticles * sizeof(float3)));
    }

    if (!d_stage || !d_k1x || !d_k2x || !d_k3x || !d_k4x || !d_k1v || !d_k2v || !d_k3v || !d_k4v
        || (_integratorMode == IntegratorMode::Leapfrog && !d_vHalf)) {
        /// Description: Executes the releaseRk4Buffers operation.
        releaseRk4Buffers();
        return false;
    }
    return true;
}

/// Description: Executes the releaseRk4Buffers operation.
void ParticleSystem::releaseRk4Buffers()
{
    if (d_stage) { grav_x::CudaMemoryPool::deallocate(d_stage); d_stage = nullptr; }
    if (d_k1x) { grav_x::CudaMemoryPool::deallocate(d_k1x); d_k1x = nullptr; }
    if (d_k2x) { grav_x::CudaMemoryPool::deallocate(d_k2x); d_k2x = nullptr; }
    if (d_k3x) { grav_x::CudaMemoryPool::deallocate(d_k3x); d_k3x = nullptr; }
    if (d_k4x) { grav_x::CudaMemoryPool::deallocate(d_k4x); d_k4x = nullptr; }
    if (d_k1v) { grav_x::CudaMemoryPool::deallocate(d_k1v); d_k1v = nullptr; }
    if (d_k2v) { grav_x::CudaMemoryPool::deallocate(d_k2v); d_k2v = nullptr; }
    if (d_k3v) { grav_x::CudaMemoryPool::deallocate(d_k3v); d_k3v = nullptr; }
    if (d_k4v) { grav_x::CudaMemoryPool::deallocate(d_k4v); d_k4v = nullptr; }
    if (d_vHalf) { grav_x::CudaMemoryPool::deallocate(d_vHalf); d_vHalf = nullptr; }
}

/// Description: Executes the allocateSphBuffers operation.
bool ParticleSystem::allocateSphBuffers(int numParticles)
{
    if (!_cudaRuntimeAvailable) {
        return false;
    }
    /// Description: Executes the releaseSphBuffers operation.
    releaseSphBuffers();
    d_sphDensity = static_cast<float*>(grav_x::CudaMemoryPool::allocate(numParticles * sizeof(float)));
    d_sphPressure = static_cast<float*>(grav_x::CudaMemoryPool::allocate(numParticles * sizeof(float)));

    if (!d_sphDensity || !d_sphPressure) {
        /// Description: Executes the releaseSphBuffers operation.
        releaseSphBuffers();
        return false;
    }
    return true;
}

/// Description: Executes the releaseSphBuffers operation.
void ParticleSystem::releaseSphBuffers()
{
    if (d_sphDensity) { grav_x::CudaMemoryPool::deallocate(d_sphDensity); d_sphDensity = nullptr; }
    if (d_sphPressure) { grav_x::CudaMemoryPool::deallocate(d_sphPressure); d_sphPressure = nullptr; }
}

/// Description: Executes the allocateSphGridBuffers operation.
bool ParticleSystem::allocateSphGridBuffers(int numParticles)
{
    if (!_cudaRuntimeAvailable) {
        return false;
    }
    /// Description: Executes the releaseSphGridBuffers operation.
    releaseSphGridBuffers();
    const std::size_t particleBytes = static_cast<std::size_t>(numParticles) * sizeof(int);
    d_sphCellHash = static_cast<int*>(grav_x::CudaMemoryPool::allocate(particleBytes));
    d_sphSortedIndex = static_cast<int*>(grav_x::CudaMemoryPool::allocate(particleBytes));

    if (!d_sphCellHash || !d_sphSortedIndex) {
        /// Description: Executes the releaseSphGridBuffers operation.
        releaseSphGridBuffers();
        return false;
    }
    _hostCellHash.resize(numParticles);
    _hostSortedIndex.resize(numParticles);
    return true;
}

/// Description: Executes the releaseSphGridBuffers operation.
void ParticleSystem::releaseSphGridBuffers()
{
    if (d_sphCellHash) { grav_x::CudaMemoryPool::deallocate(d_sphCellHash); d_sphCellHash = nullptr; }
    if (d_sphSortedIndex) { grav_x::CudaMemoryPool::deallocate(d_sphSortedIndex); d_sphSortedIndex = nullptr; }
    if (d_sphCellStart) { grav_x::CudaMemoryPool::deallocate(d_sphCellStart); d_sphCellStart = nullptr; }
    if (d_sphCellEnd) { grav_x::CudaMemoryPool::deallocate(d_sphCellEnd); d_sphCellEnd = nullptr; }
    _hostCellHash.clear();
    _hostSortedIndex.clear();
}

/// Description: Executes the seedDeviceState operation.
bool ParticleSystem::seedDeviceState()
{
    if (_particles.empty()) return true;
    /// Description: Executes the syncDeviceState operation.
    syncDeviceState();
    return true;
}
