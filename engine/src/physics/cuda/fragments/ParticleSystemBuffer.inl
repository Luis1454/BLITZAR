/*
 * Module: physics/cuda
 * Responsibility: Manage particle-system buffer allocation and release paths.
 */

#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {

constexpr std::size_t kVramBudgetBytes = 6656ull * 1024ull * 1024ull;
constexpr std::size_t kDefaultEnergySampleLimit = 65536u;
constexpr std::size_t kPlanAEnergySampleLimit = 4096u;
constexpr int kDefaultOctreeLeafCapacity = 128;
constexpr int kPlanBOctreeLeafCapacity = 4096;

double bytesToMiB(std::size_t bytes)
{
    return static_cast<double>(bytes) / (1024.0 * 1024.0);
}

} // namespace

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
            std::max(2, expectedLeaves * (leafDepth + 1) * 4 + 8);
        octreeBufferBytes += static_cast<std::size_t>(requiredNodeCapacity) * sizeof(GpuOctreeNode);
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
    out << std::fixed << std::setprecision(2)
        << "[info] [memory] Base/Integrator: " << bytesToMiB(baseAndIntegratorBytes) << " MB\n"
        << "[info] [memory] SPH Buffers: " << bytesToMiB(sphBytes) << " MB\n"
        << "[info] [memory] Octree Scratch/Nodes: " << bytesToMiB(octreeBytes) << " MB\n"
        << "[info] [memory] TOTAL: " << bytesToMiB(totalBytes) << " MB / " << bytesToMiB(budgetBytes)
        << " MB (" << (100.0 * static_cast<double>(totalBytes) / static_cast<double>(budgetBytes)) << "%)";
    return out.str();
}

void ParticleSystem::initializeRuntimeState(std::size_t particleCapacity)
{
    const cudaError_t mapHostStatus = cudaSetDeviceFlags(cudaDeviceMapHost);
    if (mapHostStatus != cudaSuccess && mapHostStatus != cudaErrorSetOnActiveProcess) {
        checkCudaStatus(mapHostStatus, "cudaSetDeviceFlags(cudaDeviceMapHost)");
    }
    grav_x::CudaMemoryPool::initialize();
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
            std::string("[memory] strict admission-control rejected configuration\n")
            + formatMemoryBreakdown(
                baseAndIntegratorBytes,
                sphBytes,
                octreeBytes,
                totalBytes,
                kVramBudgetBytes)
            + "\n[memory] plan Z: strict mode abort");
    }

    allocateMappedMetrics();
}

bool ParticleSystem::allocateParticleBuffers(std::size_t particleCapacity)
{
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
        releaseParticleBuffers();
        return false;
    }
    return true;
}

void ParticleSystem::releaseParticleBuffers()
{
    releaseRk4Buffers();
    releaseSphBuffers();
    releaseSphGridBuffers();
    
    grav_x::CudaMemoryPool::deallocate(d_soaPosX); d_soaPosX = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaPosY); d_soaPosY = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaPosZ); d_soaPosZ = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaVelX); d_soaVelX = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaVelY); d_soaVelY = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaVelZ); d_soaVelZ = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaPressX); d_soaPressX = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaPressY); d_soaPressY = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaPressZ); d_soaPressZ = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaMass); d_soaMass = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaTemp); d_soaTemp = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaDens); d_soaDens = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaNextPosX); d_soaNextPosX = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaNextPosY); d_soaNextPosY = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaNextPosZ); d_soaNextPosZ = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaNextVelX); d_soaNextVelX = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaNextVelY); d_soaNextVelY = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaNextVelZ); d_soaNextVelZ = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_vHalf); d_vHalf = nullptr;
    _leapfrogPrimed = false;

    if (g_dOctreeNodes) {
        grav_x::CudaMemoryPool::deallocate(g_dOctreeNodes);
        g_dOctreeNodes = nullptr;
    }
    if (g_dOctreeLeafIndices) {
        grav_x::CudaMemoryPool::deallocate(g_dOctreeLeafIndices);
        g_dOctreeLeafIndices = nullptr;
    }
    if (d_octreeMortonKeys) {
        grav_x::CudaMemoryPool::deallocate(d_octreeMortonKeys);
        d_octreeMortonKeys = nullptr;
    }
    if (d_octreePrefixesA) {
        grav_x::CudaMemoryPool::deallocate(d_octreePrefixesA);
        d_octreePrefixesA = nullptr;
    }
    if (d_octreePrefixesB) {
        grav_x::CudaMemoryPool::deallocate(d_octreePrefixesB);
        d_octreePrefixesB = nullptr;
    }
    if (d_octreeLevelIndicesA) {
        grav_x::CudaMemoryPool::deallocate(d_octreeLevelIndicesA);
        d_octreeLevelIndicesA = nullptr;
    }
    if (d_octreeLevelIndicesB) {
        grav_x::CudaMemoryPool::deallocate(d_octreeLevelIndicesB);
        d_octreeLevelIndicesB = nullptr;
    }
    if (d_octreeParentCounts) {
        grav_x::CudaMemoryPool::deallocate(d_octreeParentCounts);
        d_octreeParentCounts = nullptr;
    }
    if (d_octreeParentOffsets) {
        grav_x::CudaMemoryPool::deallocate(d_octreeParentOffsets);
        d_octreeParentOffsets = nullptr;
    }
    if (d_energyKineticBlocks) {
        grav_x::CudaMemoryPool::deallocate(d_energyKineticBlocks);
        d_energyKineticBlocks = nullptr;
    }
    if (d_energyThermalBlocks) {
        grav_x::CudaMemoryPool::deallocate(d_energyThermalBlocks);
        d_energyThermalBlocks = nullptr;
    }
    if (d_energyPotentialPartials) {
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
    releaseMappedMetrics();
}

bool ParticleSystem::allocateMappedMetrics()
{
    releaseMappedMetrics();
    void *hostPtr = nullptr;
    cudaError_t status = cudaHostAlloc(
        &hostPtr,
        sizeof(GpuSystemMetrics),
        cudaHostAllocMapped | cudaHostAllocPortable);
    if (!checkCudaStatus(status, "cudaHostAlloc(mapped metrics)")) {
        return false;
    }

    std::memset(hostPtr, 0, sizeof(GpuSystemMetrics));
    _mappedMetricsHost = static_cast<GpuSystemMetrics *>(hostPtr);

    void *devicePtr = nullptr;
    status = cudaHostGetDevicePointer(&devicePtr, hostPtr, 0);
    if (!checkCudaStatus(status, "cudaHostGetDevicePointer(mapped metrics)")) {
        cudaFreeHost(hostPtr);
        _mappedMetricsHost = nullptr;
        return false;
    }
    _mappedMetricsDevice = static_cast<GpuSystemMetrics *>(devicePtr);
    _metricsStepId = 0u;
    _metricsSimTime = 0.0f;
    return true;
}

void ParticleSystem::releaseMappedMetrics()
{
    if (_mappedMetricsHost != nullptr) {
        cudaFreeHost(_mappedMetricsHost);
    }
    _mappedMetricsHost = nullptr;
    _mappedMetricsDevice = nullptr;
    _metricsStepId = 0u;
    _metricsSimTime = 0.0f;
}

bool ParticleSystem::ensureLinearOctreeScratchCapacity(int numParticles)
{
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
            grav_x::CudaMemoryPool::deallocate(g_dOctreeLeafIndices);
            g_dOctreeLeafIndices = nullptr;
        }
        g_dOctreeLeafIndices = static_cast<int*>(
            grav_x::CudaMemoryPool::allocate(static_cast<std::size_t>(numParticles) * sizeof(int)));
        if (!g_dOctreeLeafIndices) {
            g_dOctreeLeafCapacity = 0;
            return false;
        }
        g_dOctreeLeafCapacity = static_cast<std::size_t>(numParticles);
    }

    if (d_octreeMortonCapacity < static_cast<std::size_t>(numParticles)) {
        if (d_octreeMortonKeys) {
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
            grav_x::CudaMemoryPool::deallocate(d_octreePrefixesA);
            d_octreePrefixesA = nullptr;
        }
        if (d_octreePrefixesB) {
            grav_x::CudaMemoryPool::deallocate(d_octreePrefixesB);
            d_octreePrefixesB = nullptr;
        }
        d_octreePrefixesA = static_cast<unsigned long long*>(grav_x::CudaMemoryPool::allocate(
            static_cast<std::size_t>(numParticles) * sizeof(unsigned long long)));
        d_octreePrefixesB = static_cast<unsigned long long*>(grav_x::CudaMemoryPool::allocate(
            static_cast<std::size_t>(numParticles) * sizeof(unsigned long long)));
        if (!d_octreePrefixesA || !d_octreePrefixesB) {
            if (d_octreePrefixesA) {
                grav_x::CudaMemoryPool::deallocate(d_octreePrefixesA);
                d_octreePrefixesA = nullptr;
            }
            if (d_octreePrefixesB) {
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
            grav_x::CudaMemoryPool::deallocate(d_octreeLevelIndicesA);
            d_octreeLevelIndicesA = nullptr;
        }
        if (d_octreeLevelIndicesB) {
            grav_x::CudaMemoryPool::deallocate(d_octreeLevelIndicesB);
            d_octreeLevelIndicesB = nullptr;
        }
        if (d_octreeParentCounts) {
            grav_x::CudaMemoryPool::deallocate(d_octreeParentCounts);
            d_octreeParentCounts = nullptr;
        }
        if (d_octreeParentOffsets) {
            grav_x::CudaMemoryPool::deallocate(d_octreeParentOffsets);
            d_octreeParentOffsets = nullptr;
        }

        d_octreeLevelIndicesA = static_cast<int*>(
            grav_x::CudaMemoryPool::allocate(static_cast<std::size_t>(numParticles) * sizeof(int)));
        d_octreeLevelIndicesB = static_cast<int*>(
            grav_x::CudaMemoryPool::allocate(static_cast<std::size_t>(numParticles) * sizeof(int)));
        d_octreeParentCounts = static_cast<int*>(
            grav_x::CudaMemoryPool::allocate(static_cast<std::size_t>(numParticles) * sizeof(int)));
        d_octreeParentOffsets = static_cast<int*>(
            grav_x::CudaMemoryPool::allocate(static_cast<std::size_t>(numParticles) * sizeof(int)));
        if (!d_octreeLevelIndicesA || !d_octreeLevelIndicesB || !d_octreeParentCounts ||
            !d_octreeParentOffsets) {
            if (d_octreeLevelIndicesA) {
                grav_x::CudaMemoryPool::deallocate(d_octreeLevelIndicesA);
                d_octreeLevelIndicesA = nullptr;
            }
            if (d_octreeLevelIndicesB) {
                grav_x::CudaMemoryPool::deallocate(d_octreeLevelIndicesB);
                d_octreeLevelIndicesB = nullptr;
            }
            if (d_octreeParentCounts) {
                grav_x::CudaMemoryPool::deallocate(d_octreeParentCounts);
                d_octreeParentCounts = nullptr;
            }
            if (d_octreeParentOffsets) {
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
            grav_x::CudaMemoryPool::deallocate(g_dOctreeNodes);
            g_dOctreeNodes = nullptr;
        }
        g_dOctreeNodes = static_cast<GpuOctreeNode*>(grav_x::CudaMemoryPool::allocate(
            static_cast<std::size_t>(requiredNodeCapacity) * sizeof(GpuOctreeNode)));
        if (!g_dOctreeNodes) {
            g_dOctreeNodeCapacity = 0;
            return false;
        }
        g_dOctreeNodeCapacity = static_cast<std::size_t>(requiredNodeCapacity);
    }

    return true;
}

bool ParticleSystem::ensureEnergyScratchCapacity(int numParticles, int sampleCount)
{
    if (numParticles <= 0 || sampleCount <= 0) {
        return false;
    }
    const int blockCount =
        (numParticles + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    if (d_energyBlockCapacity < static_cast<std::size_t>(blockCount)) {
        if (d_energyKineticBlocks) {
            grav_x::CudaMemoryPool::deallocate(d_energyKineticBlocks);
            d_energyKineticBlocks = nullptr;
        }
        if (d_energyThermalBlocks) {
            grav_x::CudaMemoryPool::deallocate(d_energyThermalBlocks);
            d_energyThermalBlocks = nullptr;
        }
        d_energyKineticBlocks = static_cast<float*>(grav_x::CudaMemoryPool::allocate(
            static_cast<std::size_t>(blockCount) * sizeof(float)));
        d_energyThermalBlocks = static_cast<float*>(grav_x::CudaMemoryPool::allocate(
            static_cast<std::size_t>(blockCount) * sizeof(float)));
        if (!d_energyKineticBlocks || !d_energyThermalBlocks) {
            if (d_energyKineticBlocks) {
                grav_x::CudaMemoryPool::deallocate(d_energyKineticBlocks);
                d_energyKineticBlocks = nullptr;
            }
            if (d_energyThermalBlocks) {
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

bool ParticleSystem::allocateRk4Buffers(int numParticles)
{
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
        releaseRk4Buffers();
        return false;
    }
    return true;
}

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

bool ParticleSystem::allocateSphBuffers(int numParticles)
{
    releaseSphBuffers();
    d_sphDensity = static_cast<float*>(grav_x::CudaMemoryPool::allocate(numParticles * sizeof(float)));
    d_sphPressure = static_cast<float*>(grav_x::CudaMemoryPool::allocate(numParticles * sizeof(float)));

    if (!d_sphDensity || !d_sphPressure) {
        releaseSphBuffers();
        return false;
    }
    return true;
}

void ParticleSystem::releaseSphBuffers()
{
    if (d_sphDensity) { grav_x::CudaMemoryPool::deallocate(d_sphDensity); d_sphDensity = nullptr; }
    if (d_sphPressure) { grav_x::CudaMemoryPool::deallocate(d_sphPressure); d_sphPressure = nullptr; }
}

bool ParticleSystem::allocateSphGridBuffers(int numParticles)
{
    releaseSphGridBuffers();
    const std::size_t particleBytes = static_cast<std::size_t>(numParticles) * sizeof(int);
    d_sphCellHash = static_cast<int*>(grav_x::CudaMemoryPool::allocate(particleBytes));
    d_sphSortedIndex = static_cast<int*>(grav_x::CudaMemoryPool::allocate(particleBytes));

    if (!d_sphCellHash || !d_sphSortedIndex) {
        releaseSphGridBuffers();
        return false;
    }
    _hostCellHash.resize(numParticles);
    _hostSortedIndex.resize(numParticles);
    return true;
}

void ParticleSystem::releaseSphGridBuffers()
{
    if (d_sphCellHash) { grav_x::CudaMemoryPool::deallocate(d_sphCellHash); d_sphCellHash = nullptr; }
    if (d_sphSortedIndex) { grav_x::CudaMemoryPool::deallocate(d_sphSortedIndex); d_sphSortedIndex = nullptr; }
    if (d_sphCellStart) { grav_x::CudaMemoryPool::deallocate(d_sphCellStart); d_sphCellStart = nullptr; }
    if (d_sphCellEnd) { grav_x::CudaMemoryPool::deallocate(d_sphCellEnd); d_sphCellEnd = nullptr; }
    _hostCellHash.clear();
    _hostSortedIndex.clear();
}

bool ParticleSystem::seedDeviceState()
{
    if (_particles.empty()) return true;
    syncDeviceState();
    return true;
}
