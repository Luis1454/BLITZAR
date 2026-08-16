/*
 * @file engine/src/cuda/JitRuntime.cu
 * @brief NVRTC specialization, cache, warm-up and CUDA Graph runtime.
 */

#include "physics/cuda/CudaJit.hpp"

#include <cuda_runtime_api.h>

#if BLITZAR_HAS_CUDA_DRIVER
#include <cuda.h>
#endif

#if BLITZAR_HAS_NVRTC
#include <nvrtc.h>
#endif

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "fragments/jit/Source.inl"

struct CudaJitRuntime::Impl final {
#if BLITZAR_HAS_CUDA_DRIVER
    struct ModuleEntry final {
        CUmodule module = nullptr;
        CUfunction function = nullptr;
    };

    CUdevice device = 0;
    CUcontext context = nullptr;
    bool ownsPrimaryContext = false;
    int computeMajor = 0;
    int computeMinor = 0;
    int nvrtcMajor = 0;
    int nvrtcMinor = 0;
    std::uint64_t sourceHash = 0u;
    std::filesystem::path cacheDirectory;
    std::unordered_map<std::string, ModuleEntry> modules;
    std::string currentKey;
    ModuleEntry currentModule;
    CudaJitMetrics currentMetrics;
    unsigned int* divergenceCounter = nullptr;
    cudaStream_t graphStream = nullptr;
    cudaGraph_t graph = nullptr;
    cudaGraphExec_t graphExec = nullptr;
    float* graphX = nullptr;
    float* graphY = nullptr;
    float* graphZ = nullptr;
    int graphCells = 0;
    int graphBlockSize = 0;
    float graphScale = 0.0f;
#endif
};

#if BLITZAR_HAS_CUDA_DRIVER

#include "fragments/jit/Cache.inl"
#include "fragments/jit/Execution.inl"
#include "fragments/jit/Benchmark.inl"
#endif

CudaJitRuntime::CudaJitRuntime(std::string cacheDirectory) : _impl(std::make_unique<Impl>())
{
#if BLITZAR_HAS_CUDA_DRIVER
    if (!blitzar_cuda_jit_runtime::jitEnabledFromEnvironment()) {
        return;
    }
    _impl->cacheDirectory = std::move(cacheDirectory);
    if (!ensureDriverContext(*_impl) ||
        cudaMalloc(&_impl->divergenceCounter, sizeof(unsigned int)) != cudaSuccess ||
        cudaStreamCreateWithFlags(&_impl->graphStream, cudaStreamNonBlocking) != cudaSuccess) {
        return;
    }
#else
    static_cast<void>(cacheDirectory);
#endif
}

CudaJitRuntime::~CudaJitRuntime()
{
#if BLITZAR_HAS_CUDA_DRIVER
    destroyGraph(*_impl);
    if (_impl->divergenceCounter != nullptr) {
        cudaFree(_impl->divergenceCounter);
    }
    if (_impl->graphStream != nullptr) {
        cudaStreamDestroy(_impl->graphStream);
    }
    for (const auto& item : _impl->modules) {
        if (item.second.module != nullptr) {
            cuModuleUnload(item.second.module);
        }
    }
    if (_impl->ownsPrimaryContext && _impl->context != nullptr) {
        cuDevicePrimaryCtxRelease(_impl->device);
    }
#endif
}

bool CudaJitRuntime::available() const noexcept
{
#if BLITZAR_HAS_CUDA_DRIVER
    return _impl != nullptr && _impl->context != nullptr && _impl->divergenceCounter != nullptr;
#else
    return false;
#endif
}

bool CudaJitRuntime::launchTreePmNormalize(float* fieldX, float* fieldY, float* fieldZ,
                                           int totalCells, float scale,
                                           const CudaJitRequest& request, CudaJitMetrics* metrics)
{
#if BLITZAR_HAS_CUDA_DRIVER
    CudaJitMetrics localMetrics;
    CudaJitMetrics& result = metrics != nullptr ? *metrics : localMetrics;
    result = CudaJitMetrics{};
    if (!available() || fieldX == nullptr || fieldY == nullptr || fieldZ == nullptr ||
        totalCells <= 0 || request.blockSize <= 0 || request.blockSize > 1024) {
        result.cacheSource = available() ? "invalid-launch" : "runtime-unavailable";
        return false;
    }
    const std::string key = makeKey(request, *_impl);
    if (_impl->currentKey == key) {
        result = _impl->currentMetrics;
        result.cacheHit = true;
        result.cacheSource = "ram";
    }
    else {
        if (!loadOrCompile(*_impl, request, &result)) {
            result.cacheSource = "module-load-failed";
            return false;
        }
        _impl->currentMetrics = result;
    }
    if (!result.warmupAccepted && result.staticMs == 0.0 && result.jitMs == 0.0) {
        // Warm-up must not modify the production FFT field; normalization is measured at scale 1.
        if (!warmup(*_impl, fieldX, totalCells, 1.0f, request, &result)) {
            result.cacheSource = "warmup-error";
            return false;
        }
        _impl->currentMetrics = result;
    }
    if (!result.warmupAccepted) {
        return false;
    }
    result.usedJit = true;
    const bool sameGraph = _impl->graphExec != nullptr && _impl->graphX == fieldX &&
                           _impl->graphY == fieldY && _impl->graphZ == fieldZ &&
                           _impl->graphCells == totalCells && _impl->graphScale == scale &&
                           _impl->graphBlockSize == request.blockSize;
    if (!sameGraph &&
        !captureGraph(*_impl, fieldX, fieldY, fieldZ, totalCells, scale, request.blockSize)) {
        result.graphCaptured = false;
        cudaGetLastError();
        if (!launchJit(*_impl, fieldX, totalCells, scale, request.blockSize) ||
            !launchJit(*_impl, fieldY, totalCells, scale, request.blockSize) ||
            !launchJit(*_impl, fieldZ, totalCells, scale, request.blockSize)) {
            return false;
        }
    }
    else {
        result.graphCaptured = true;
        if (cudaGraphLaunch(_impl->graphExec, _impl->graphStream) != cudaSuccess) {
            return false;
        }
    }
    return cudaGetLastError() == cudaSuccess;
#else
    static_cast<void>(fieldX);
    static_cast<void>(fieldY);
    static_cast<void>(fieldZ);
    static_cast<void>(totalCells);
    static_cast<void>(scale);
    static_cast<void>(request);
    if (metrics != nullptr) {
        *metrics = CudaJitMetrics{};
    }
    return false;
#endif
}

bool CudaJitRuntime::launchForceTile(float* posX, float* posY, float* posZ, float* mass,
                                     Vector3* output, int particleCount, float softening,
                                     float minDistance2, float maxAcceleration,
                                     const CudaJitRequest& request, CudaJitMetrics* metrics)
{
#if BLITZAR_HAS_CUDA_DRIVER
    CudaJitMetrics localMetrics;
    CudaJitMetrics& result = metrics != nullptr ? *metrics : localMetrics;
    result = CudaJitMetrics{};
    if (!available() || posX == nullptr || posY == nullptr || posZ == nullptr || mass == nullptr ||
        output == nullptr || particleCount <= 0 || request.blockSize <= 0 ||
        request.blockSize > 1024 || request.tileSize <= 0 || request.tileSize > 1024) {
        result.cacheSource = available() ? "invalid-launch" : "runtime-unavailable";
        return false;
    }
    const std::string key = makeKey(request, *_impl);
    if (_impl->currentKey == key) {
        result = _impl->currentMetrics;
        result.cacheHit = true;
        result.cacheSource = "ram";
    }
    else {
        if (!loadOrCompile(*_impl, request, &result)) {
            result.cacheSource = "module-load-failed";
            return false;
        }
        _impl->currentMetrics = result;
    }
    if (!result.warmupAccepted && result.staticMs == 0.0 && result.jitMs == 0.0) {
        if (!warmupForce(*_impl, posX, posY, posZ, mass, output, particleCount, softening,
                         minDistance2, maxAcceleration, request, &result)) {
            result.cacheSource = "warmup-error";
            return false;
        }
        _impl->currentMetrics = result;
    }
    if (!result.warmupAccepted) {
        return false;
    }
    result.usedJit = true;
    return launchJitForce(*_impl, posX, posY, posZ, mass, output, particleCount, softening,
                          minDistance2, maxAcceleration, request.blockSize, request.tileSize) &&
           cudaGetLastError() == cudaSuccess;
#else
    static_cast<void>(posX);
    static_cast<void>(posY);
    static_cast<void>(posZ);
    static_cast<void>(mass);
    static_cast<void>(output);
    static_cast<void>(particleCount);
    static_cast<void>(softening);
    static_cast<void>(minDistance2);
    static_cast<void>(maxAcceleration);
    static_cast<void>(request);
    if (metrics != nullptr) {
        *metrics = CudaJitMetrics{};
    }
    return false;
#endif
}
