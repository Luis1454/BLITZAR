/*
 * @file engine/physics/cuda/include/CudaJit.hpp
 * @brief Runtime specialization contract for regular CUDA stages.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_PHYSICS_CUDAJIT_HPP_
#define BLITZAR_ENGINE_INCLUDE_PHYSICS_CUDAJIT_HPP_

#include <cstdint>
#include <memory>
#include <string>

#include "Vector.hpp"

enum class CudaJitFamily : std::uint8_t {
    TreePmStencil = 0,
    ForceTile = 1,
    Softening = 2
};

struct CudaJitRequest final {
    CudaJitFamily family = CudaJitFamily::TreePmStencil;
    int blockSize = 256;
    int tileSize = 4;
    int assignment = 0;
    int softeningMode = 0;
    float softening = 0.0f;
};

struct CudaJitMetrics final {
    bool available = false;
    bool usedJit = false;
    bool cacheHit = false;
    bool warmupAccepted = false;
    bool graphCaptured = false;
    bool divergenceInstrumented = false;
    std::uint32_t registersPerThread = 0u;
    std::uint32_t staticSharedBytes = 0u;
    std::uint32_t activeBlocksPerSm = 0u;
    std::uint32_t maxThreadsPerSm = 0u;
    float occupancy = 0.0f;
    float divergentWarpFraction = 0.0f;
    double compileMs = 0.0;
    double staticMs = 0.0;
    double jitMs = 0.0;
    std::string cacheSource;
};

class CudaJitRuntime final {
public:
    explicit CudaJitRuntime(std::string cacheDirectory = {});
    ~CudaJitRuntime();

    CudaJitRuntime(const CudaJitRuntime&) = delete;
    CudaJitRuntime& operator=(const CudaJitRuntime&) = delete;
    CudaJitRuntime(CudaJitRuntime&&) = delete;
    CudaJitRuntime& operator=(CudaJitRuntime&&) = delete;

    bool available() const noexcept;
    bool launchTreePmNormalize(float* fieldX, float* fieldY, float* fieldZ, int totalCells,
                               float scale, const CudaJitRequest& request, CudaJitMetrics* metrics);
    bool launchForceTile(float* posX, float* posY, float* posZ, float* mass, Vector3* output,
                         int particleCount, float softening, float minDistance2,
                         float maxAcceleration, const CudaJitRequest& request,
                         CudaJitMetrics* metrics);

public:
    // Opaque implementation kept public only so the CUDA translation unit can own helpers.
    struct Impl;

private:
    std::unique_ptr<Impl> _impl;
};

#endif // BLITZAR_ENGINE_INCLUDE_PHYSICS_CUDAJIT_HPP_
