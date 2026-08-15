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

namespace blitzar_cuda_jit_runtime {

bool jitEnabledFromEnvironment()
{
    const char* value = std::getenv("BLITZAR_CUDA_JIT");
    return value == nullptr || std::string(value) != "0";
}

__device__ void recordBoundaryDivergence(unsigned int* counter, bool active)
{
    if (counter == nullptr) {
        return;
    }
    const unsigned int mask = __ballot_sync(0xffffffffu, active);
    if ((threadIdx.x & 31) == 0 && mask != 0u && mask != 0xffffffffu) {
        atomicAdd(counter, 1u);
    }
}

__global__ void staticTreePmNormalizeKernel(float* field, int totalCells, float scale,
                                            unsigned int* divergenceCounter)
{
    const int index = blockIdx.x * blockDim.x + threadIdx.x;
    const bool active = index < totalCells;
    recordBoundaryDivergence(divergenceCounter, active);
    if (active) {
        field[index] *= scale;
    }
}

__global__ void staticForceTileKernel(const float* posX, const float* posY, const float* posZ,
                                      const float* mass, Vector3* output, int particleCount,
                                      float softening, float minDistance2, float maxAcceleration,
                                      unsigned int* divergenceCounter)
{
    constexpr int kTileSize = 128;
    const int targetIndex = blockIdx.x * blockDim.x + threadIdx.x;
    const bool active = targetIndex < particleCount;
    recordBoundaryDivergence(divergenceCounter, active);
    const float targetX = active ? posX[targetIndex] : 0.0f;
    const float targetY = active ? posY[targetIndex] : 0.0f;
    const float targetZ = active ? posZ[targetIndex] : 0.0f;
    extern __shared__ float shared[];
    float* tileX = shared;
    float* tileY = shared + kTileSize;
    float* tileZ = shared + 2 * kTileSize;
    float* tileMass = shared + 3 * kTileSize;
    float forceX = 0.0f;
    float forceY = 0.0f;
    float forceZ = 0.0f;
    const float softeningSquared = softening * softening;
    for (int tileStart = 0; tileStart < particleCount; tileStart += kTileSize) {
        const int tileCount = min(kTileSize, particleCount - tileStart);
        for (int i = threadIdx.x; i < tileCount; i += blockDim.x) {
            const int sourceIndex = tileStart + i;
            tileX[i] = posX[sourceIndex];
            tileY[i] = posY[sourceIndex];
            tileZ[i] = posZ[sourceIndex];
            tileMass[i] = mass[sourceIndex];
        }
        __syncthreads();
        if (active) {
            for (int i = 0; i < tileCount; ++i) {
                const int sourceIndex = tileStart + i;
                if (sourceIndex == targetIndex) {
                    continue;
                }
                const float dx = tileX[i] - targetX;
                const float dy = tileY[i] - targetY;
                const float dz = tileZ[i] - targetZ;
                const float distanceSquared = dx * dx + dy * dy + dz * dz + softeningSquared;
                if (distanceSquared > minDistance2) {
                    const float inverseDistance = rsqrtf(distanceSquared);
                    const float factor =
                        tileMass[i] * inverseDistance * inverseDistance * inverseDistance;
                    forceX += dx * factor;
                    forceY += dy * factor;
                    forceZ += dz * factor;
                }
            }
        }
        __syncthreads();
    }
    if (active) {
        const float magnitudeSquared = forceX * forceX + forceY * forceY + forceZ * forceZ;
        if (magnitudeSquared > maxAcceleration * maxAcceleration) {
            const float scale = maxAcceleration * rsqrtf(magnitudeSquared);
            forceX *= scale;
            forceY *= scale;
            forceZ *= scale;
        }
        output[targetIndex] = Vector3(forceX, forceY, forceZ);
    }
}

constexpr const char* kJitSource = R"CUDA(
extern "C" __device__ __forceinline__ void recordBoundaryDivergence(unsigned int* counter,
                                                                     bool active)
{
    if (counter == nullptr) {
        return;
    }
    const unsigned int mask = __ballot_sync(0xffffffffu, active);
    if ((threadIdx.x & 31) == 0 && mask != 0u && mask != 0xffffffffu) {
        atomicAdd(counter, 1u);
    }
}

extern "C" __global__ void blitzarTreePmStencil(float* field, int totalCells, float scale,
                                                 unsigned int* divergenceCounter)
{
    constexpr int kTileSize = TILE_SIZE;
    constexpr int kAssignment = ASSIGNMENT;
    constexpr int kSofteningMode = SOFTENING_MODE;
    const int index = blockIdx.x * blockDim.x + threadIdx.x;
    const bool active = index < totalCells;
    recordBoundaryDivergence(divergenceCounter, active);
    if (active) {
        // Keep the specialization constants in device code so the cache key maps to codegen.
        const float specialization = 1.0f + 0.0f * static_cast<float>(
            kTileSize + kAssignment + kSofteningMode);
        field[index] *= scale * specialization;
    }
}

extern "C" __global__ void blitzarForceTile(const float* posX, const float* posY,
                                             const float* posZ, const float* mass,
                                             float3* output, int particleCount, float softening,
                                             float minDistance2, float maxAcceleration,
                                             unsigned int* divergenceCounter)
{
    constexpr int kTileSize = TILE_SIZE;
    const int targetIndex = blockIdx.x * blockDim.x + threadIdx.x;
    const bool active = targetIndex < particleCount;
    recordBoundaryDivergence(divergenceCounter, active);
    const float targetX = active ? posX[targetIndex] : 0.0f;
    const float targetY = active ? posY[targetIndex] : 0.0f;
    const float targetZ = active ? posZ[targetIndex] : 0.0f;
    extern __shared__ float shared[];
    float* tileX = shared;
    float* tileY = shared + kTileSize;
    float* tileZ = shared + 2 * kTileSize;
    float* tileMass = shared + 3 * kTileSize;
    float forceX = 0.0f;
    float forceY = 0.0f;
    float forceZ = 0.0f;
    const float softeningSquared = softening * softening;
    for (int tileStart = 0; tileStart < particleCount; tileStart += kTileSize) {
        const int tileCount = min(kTileSize, particleCount - tileStart);
        for (int i = threadIdx.x; i < tileCount; i += blockDim.x) {
            const int sourceIndex = tileStart + i;
            tileX[i] = posX[sourceIndex];
            tileY[i] = posY[sourceIndex];
            tileZ[i] = posZ[sourceIndex];
            tileMass[i] = mass[sourceIndex];
        }
        __syncthreads();
        if (active) {
            for (int i = 0; i < tileCount; ++i) {
                const int sourceIndex = tileStart + i;
                if (sourceIndex == targetIndex) {
                    continue;
                }
                const float dx = tileX[i] - targetX;
                const float dy = tileY[i] - targetY;
                const float dz = tileZ[i] - targetZ;
                const float distanceSquared = dx * dx + dy * dy + dz * dz + softeningSquared;
                if (distanceSquared > minDistance2) {
                    const float inverseDistance = rsqrtf(distanceSquared);
                    const float factor = tileMass[i] * inverseDistance * inverseDistance *
                                         inverseDistance;
                    forceX += dx * factor;
                    forceY += dy * factor;
                    forceZ += dz * factor;
                }
            }
        }
        __syncthreads();
    }
    if (active) {
        const float magnitudeSquared = forceX * forceX + forceY * forceY + forceZ * forceZ;
        if (magnitudeSquared > maxAcceleration * maxAcceleration) {
            const float scale = maxAcceleration * rsqrtf(magnitudeSquared);
            forceX *= scale;
            forceY *= scale;
            forceZ *= scale;
        }
        output[targetIndex] = make_float3(forceX, forceY, forceZ);
    }
}
)CUDA";

std::string familyName(CudaJitFamily family)
{
    switch (family) {
    case CudaJitFamily::ForceTile:
        return "force_tile";
    case CudaJitFamily::Softening:
        return "softening";
    case CudaJitFamily::TreePmStencil:
    default:
        return "treepm_stencil";
    }
}

const char* functionName(CudaJitFamily family)
{
    return family == CudaJitFamily::ForceTile ? "blitzarForceTile" : "blitzarTreePmStencil";
}

std::uint64_t fnv1a(const std::string& value)
{
    std::uint64_t hash = 1469598103934665603ull;
    for (const unsigned char byte : value) {
        hash ^= byte;
        hash *= 1099511628211ull;
    }
    return hash;
}

std::string hexKey(std::uint64_t value)
{
    std::ostringstream stream;
    stream << std::hex << std::setfill('0') << std::setw(16) << value;
    return stream.str();
}

double elapsedMs(cudaEvent_t begin, cudaEvent_t end)
{
    float milliseconds = 0.0f;
    if (cudaEventElapsedTime(&milliseconds, begin, end) != cudaSuccess) {
        return 0.0;
    }
    return static_cast<double>(milliseconds);
}

} // namespace blitzar_cuda_jit_runtime

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

static bool ensureDriverContext(CudaJitRuntime::Impl& impl)
{
    if (cuInit(0) != CUDA_SUCCESS || cuDeviceGet(&impl.device, 0) != CUDA_SUCCESS) {
        return false;
    }
    if (cudaFree(nullptr) != cudaSuccess) {
        cudaGetLastError();
        return false;
    }
    if (cuCtxGetCurrent(&impl.context) != CUDA_SUCCESS) {
        return false;
    }
    if (impl.context == nullptr) {
        if (cuDevicePrimaryCtxRetain(&impl.context, impl.device) != CUDA_SUCCESS ||
            cuCtxSetCurrent(impl.context) != CUDA_SUCCESS) {
            return false;
        }
        impl.ownsPrimaryContext = true;
    }
    if (cuDeviceGetAttribute(&impl.computeMajor, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR,
                             impl.device) != CUDA_SUCCESS ||
        cuDeviceGetAttribute(&impl.computeMinor, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR,
                             impl.device) != CUDA_SUCCESS) {
        return false;
    }
#if BLITZAR_HAS_NVRTC
    if (nvrtcVersion(&impl.nvrtcMajor, &impl.nvrtcMinor) != NVRTC_SUCCESS) {
        impl.nvrtcMajor = 0;
        impl.nvrtcMinor = 0;
    }
#endif
    impl.sourceHash = blitzar_cuda_jit_runtime::fnv1a(blitzar_cuda_jit_runtime::kJitSource);
    return impl.computeMajor >= 5;
}

static std::string makeKey(const CudaJitRequest& request, const CudaJitRuntime::Impl& impl)
{
    std::ostringstream material;
    material << "blitzar-jit-v2|" << blitzar_cuda_jit_runtime::familyName(request.family)
             << "|cc=" << impl.computeMajor
             << '.' << impl.computeMinor << "|nvrtc=" << impl.nvrtcMajor << '.'
             << impl.nvrtcMinor << "|source=" << blitzar_cuda_jit_runtime::hexKey(impl.sourceHash)
             << "|block=" << request.blockSize
             << "|tile=" << request.tileSize << "|assignment=" << request.assignment
             << "|softening-mode=" << request.softeningMode << "|softening=" << std::setprecision(9)
             << request.softening;
    return blitzar_cuda_jit_runtime::hexKey(blitzar_cuda_jit_runtime::fnv1a(material.str()));
}

static std::filesystem::path defaultCacheDirectory()
{
    if (const char* configured = std::getenv("BLITZAR_CUDA_JIT_CACHE");
        configured != nullptr && configured[0] != '\0') {
        return std::filesystem::path(configured);
    }
    return std::filesystem::temp_directory_path() / "blitzar" / "cuda-jit";
}

static void reportDriverError(const char* operation, CUresult status)
{
    const char* name = nullptr;
    const char* description = nullptr;
    cuGetErrorName(status, &name);
    cuGetErrorString(status, &description);
    fprintf(stderr, "[cuda-jit] %s failed: %s (%s)\n", operation,
            name == nullptr ? "unknown" : name,
            description == nullptr ? "no description" : description);
}

static bool loadPtx(const std::filesystem::path& path, std::vector<char>* ptx)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return false;
    }
    input.seekg(0, std::ios::end);
    const std::streamsize size = input.tellg();
    if (size <= 0) {
        return false;
    }
    input.seekg(0, std::ios::beg);
    ptx->resize(static_cast<std::size_t>(size));
    return input.read(ptx->data(), size).good();
}

static bool compilePtx(const CudaJitRequest& request, const CudaJitRuntime::Impl& impl,
                       std::vector<char>* ptx, double* compileMs)
{
#if BLITZAR_HAS_NVRTC
    const std::string architecture = "--gpu-architecture=sm_" + std::to_string(impl.computeMajor) +
                                     std::to_string(impl.computeMinor);
    const std::string block = "--define-macro=BLOCK_SIZE=" + std::to_string(request.blockSize);
    const std::string tile = "--define-macro=TILE_SIZE=" + std::to_string(request.tileSize);
    const std::string assignment =
        "--define-macro=ASSIGNMENT=" + std::to_string(request.assignment);
    const std::string softeningMode =
        "--define-macro=SOFTENING_MODE=" + std::to_string(request.softeningMode);
    const char* options[] = {"--std=c++17",      architecture.c_str(),  block.c_str(), tile.c_str(),
                             assignment.c_str(), softeningMode.c_str(), "--fmad=true"};
    nvrtcProgram program = nullptr;
    const auto begin = std::chrono::steady_clock::now();
    nvrtcResult result =
        nvrtcCreateProgram(&program, blitzar_cuda_jit_runtime::kJitSource, "blitzar_jit.cu", 0,
                           nullptr, nullptr);
    if (result == NVRTC_SUCCESS) {
        result = nvrtcCompileProgram(
            program, static_cast<int>(sizeof(options) / sizeof(options[0])), options);
    }
    if (result != NVRTC_SUCCESS) {
        std::size_t logSize = 0u;
        if (program != nullptr && nvrtcGetProgramLogSize(program, &logSize) == NVRTC_SUCCESS &&
            logSize > 1u) {
            std::string log(logSize, '\0');
            nvrtcGetProgramLog(program, log.data());
            fprintf(stderr, "[cuda-jit] NVRTC compile failed: %s\n", log.c_str());
        }
        if (program != nullptr) {
            nvrtcDestroyProgram(&program);
        }
        return false;
    }
    std::size_t cubinSize = 0u;
    result = nvrtcGetCUBINSize(program, &cubinSize);
    if (result == NVRTC_SUCCESS) {
        ptx->resize(cubinSize);
        result = nvrtcGetCUBIN(program, ptx->data());
    }
    nvrtcDestroyProgram(&program);
    const auto end = std::chrono::steady_clock::now();
    *compileMs = std::chrono::duration<double, std::milli>(end - begin).count();
    return result == NVRTC_SUCCESS && !ptx->empty();
#else
    static_cast<void>(request);
    static_cast<void>(impl);
    static_cast<void>(ptx);
    static_cast<void>(compileMs);
    return false;
#endif
}

static bool collectFunctionMetrics(CudaJitRuntime::Impl& impl, CudaJitMetrics* metrics,
                                   int blockSize)
{
    int registers = 0;
    int sharedBytes = 0;
    int activeBlocks = 0;
    int maxThreadsPerSm = 0;
    if (cuFuncGetAttribute(&registers, CU_FUNC_ATTRIBUTE_NUM_REGS, impl.currentModule.function) !=
            CUDA_SUCCESS ||
        cuFuncGetAttribute(&sharedBytes, CU_FUNC_ATTRIBUTE_SHARED_SIZE_BYTES,
                           impl.currentModule.function) != CUDA_SUCCESS ||
        cuDeviceGetAttribute(&maxThreadsPerSm, CU_DEVICE_ATTRIBUTE_MAX_THREADS_PER_MULTIPROCESSOR,
                             impl.device) != CUDA_SUCCESS ||
        cuOccupancyMaxActiveBlocksPerMultiprocessor(&activeBlocks, impl.currentModule.function,
                                                    blockSize, 0u) != CUDA_SUCCESS) {
        return false;
    }
    metrics->registersPerThread = static_cast<std::uint32_t>(std::max(0, registers));
    metrics->staticSharedBytes = static_cast<std::uint32_t>(std::max(0, sharedBytes));
    metrics->activeBlocksPerSm = static_cast<std::uint32_t>(std::max(0, activeBlocks));
    metrics->maxThreadsPerSm = static_cast<std::uint32_t>(std::max(0, maxThreadsPerSm));
    metrics->occupancy = metrics->maxThreadsPerSm == 0u
                             ? 0.0f
                             : std::clamp(static_cast<float>(activeBlocks * blockSize) /
                                              static_cast<float>(metrics->maxThreadsPerSm),
                                          0.0f, 1.0f);
    return true;
}

static bool loadOrCompile(CudaJitRuntime::Impl& impl, const CudaJitRequest& request,
                          CudaJitMetrics* metrics)
{
    impl.cacheDirectory =
        impl.cacheDirectory.empty() ? defaultCacheDirectory() : impl.cacheDirectory;
    std::error_code error;
    std::filesystem::create_directories(impl.cacheDirectory, error);
    const std::string key = makeKey(request, impl);
    const auto memoryEntry = impl.modules.find(key);
    if (memoryEntry != impl.modules.end()) {
        impl.currentKey = key;
        impl.currentModule = memoryEntry->second;
        metrics->available = true;
        metrics->cacheHit = true;
        metrics->cacheSource = "ram";
        return collectFunctionMetrics(impl, metrics, request.blockSize);
    }
    const auto diskPath = impl.cacheDirectory / (key + ".cubin");
    std::vector<char> ptx;
    const bool loadedFromDisk = loadPtx(diskPath, &ptx);
    if (loadedFromDisk) {
        metrics->cacheHit = true;
        metrics->cacheSource = "disk";
    }
    else {
        if (!compilePtx(request, impl, &ptx, &metrics->compileMs)) {
            return false;
        }
        std::ofstream output(diskPath, std::ios::binary | std::ios::trunc);
        if (output) {
            output.write(ptx.data(), static_cast<std::streamsize>(ptx.size()));
        }
        metrics->cacheSource = "compile";
    }
    CudaJitRuntime::Impl::ModuleEntry entry;
    CUresult loadStatus = cuModuleLoadData(&entry.module, ptx.data());
    if (loadStatus != CUDA_SUCCESS && loadedFromDisk) {
        reportDriverError("disk module load", loadStatus);
        std::filesystem::remove(diskPath, error);
        ptx.clear();
        if (!compilePtx(request, impl, &ptx, &metrics->compileMs)) {
            return false;
        }
        metrics->cacheHit = false;
        metrics->cacheSource = "recompile";
        std::ofstream output(diskPath, std::ios::binary | std::ios::trunc);
        if (output) {
            output.write(ptx.data(), static_cast<std::streamsize>(ptx.size()));
        }
        loadStatus = cuModuleLoadData(&entry.module, ptx.data());
    }
    if (loadStatus != CUDA_SUCCESS) {
        reportDriverError("module load", loadStatus);
        return false;
    }
    const CUresult functionStatus =
        cuModuleGetFunction(&entry.function, entry.module,
                            blitzar_cuda_jit_runtime::functionName(request.family));
    if (functionStatus != CUDA_SUCCESS) {
        reportDriverError("module function lookup", functionStatus);
        cuModuleUnload(entry.module);
        return false;
    }
    impl.modules[key] = entry;
    impl.currentKey = key;
    impl.currentModule = entry;
    metrics->available = true;
    return collectFunctionMetrics(impl, metrics, request.blockSize);
}

static bool launchJit(CudaJitRuntime::Impl& impl, float* field, int totalCells, float scale,
                      int blockSize, cudaStream_t stream = nullptr)
{
    void* arguments[] = {&field, &totalCells, &scale, &impl.divergenceCounter};
    const int blocks = (totalCells + blockSize - 1) / blockSize;
    return cuLaunchKernel(impl.currentModule.function, static_cast<unsigned int>(blocks), 1u, 1u,
                          static_cast<unsigned int>(blockSize), 1u, 1u, 0u, stream, arguments,
                          nullptr) == CUDA_SUCCESS;
}

static bool launchJitForce(CudaJitRuntime::Impl& impl, float* posX, float* posY, float* posZ,
                           float* mass, Vector3* output, int particleCount, float softening,
                           float minDistance2, float maxAcceleration, int blockSize, int tileSize)
{
    void* arguments[] = {&posX,
                         &posY,
                         &posZ,
                         &mass,
                         &output,
                         &particleCount,
                         &softening,
                         &minDistance2,
                         &maxAcceleration,
                         &impl.divergenceCounter};
    const int blocks = (particleCount + blockSize - 1) / blockSize;
    const std::size_t sharedBytes = static_cast<std::size_t>(tileSize) * 4u * sizeof(float);
    return cuLaunchKernel(impl.currentModule.function, static_cast<unsigned int>(blocks), 1u, 1u,
                          static_cast<unsigned int>(blockSize), 1u, 1u,
                          static_cast<unsigned int>(sharedBytes), nullptr, arguments,
                          nullptr) == CUDA_SUCCESS;
}

static bool launchStatic(float* field, int totalCells, float scale, int blockSize,
                         unsigned int* divergenceCounter)
{
    const int blocks = (totalCells + blockSize - 1) / blockSize;
    blitzar_cuda_jit_runtime::staticTreePmNormalizeKernel<<<blocks, blockSize>>>(
        field, totalCells, scale, divergenceCounter);
    return cudaGetLastError() == cudaSuccess;
}

static bool launchStaticForce(float* posX, float* posY, float* posZ, float* mass, Vector3* output,
                              int particleCount, float softening, float minDistance2,
                              float maxAcceleration, int blockSize, unsigned int* divergenceCounter)
{
    const int blocks = (particleCount + blockSize - 1) / blockSize;
    constexpr std::size_t kTileSize = 128u;
    const std::size_t sharedBytes = kTileSize * 4u * sizeof(float);
    blitzar_cuda_jit_runtime::staticForceTileKernel<<<blocks, blockSize, sharedBytes>>>(
        posX, posY, posZ, mass, output, particleCount, softening, minDistance2, maxAcceleration,
        divergenceCounter);
    return cudaGetLastError() == cudaSuccess;
}

static bool warmup(CudaJitRuntime::Impl& impl, float* field, int totalCells, float scale,
                   const CudaJitRequest& request, CudaJitMetrics* metrics)
{
    cudaEvent_t begin = nullptr;
    cudaEvent_t end = nullptr;
    if (cudaEventCreate(&begin) != cudaSuccess || cudaEventCreate(&end) != cudaSuccess) {
        return false;
    }
    const int repetitions = 3;
    cudaMemset(impl.divergenceCounter, 0, sizeof(unsigned int));
    cudaEventRecord(begin);
    for (int i = 0; i < repetitions; ++i) {
        if (!launchStatic(field, totalCells, scale, request.blockSize, impl.divergenceCounter)) {
            cudaEventDestroy(begin);
            cudaEventDestroy(end);
            return false;
        }
    }
    cudaEventRecord(end);
    cudaEventSynchronize(end);
    metrics->staticMs = blitzar_cuda_jit_runtime::elapsedMs(begin, end) /
                        static_cast<double>(repetitions);

    cudaMemset(impl.divergenceCounter, 0, sizeof(unsigned int));
    cudaEventRecord(begin);
    for (int i = 0; i < repetitions; ++i) {
        if (!launchJit(impl, field, totalCells, scale, request.blockSize)) {
            cudaEventDestroy(begin);
            cudaEventDestroy(end);
            return false;
        }
    }
    cudaEventRecord(end);
    cudaEventSynchronize(end);
    metrics->jitMs = blitzar_cuda_jit_runtime::elapsedMs(begin, end) /
                     static_cast<double>(repetitions);
    unsigned int divergentWarps = 0u;
    cudaMemcpy(&divergentWarps, impl.divergenceCounter, sizeof(divergentWarps),
               cudaMemcpyDeviceToHost);
    const int warpsPerBlock = (request.blockSize + 31) / 32;
    const double totalWarps =
        static_cast<double>((totalCells + request.blockSize - 1) / request.blockSize) *
        warpsPerBlock;
    metrics->divergentWarpFraction =
        totalWarps <= 0.0 ? 0.0f : static_cast<float>(divergentWarps / totalWarps);
    metrics->divergenceInstrumented = true;
    metrics->warmupAccepted = metrics->jitMs <= metrics->staticMs * 1.02;
    cudaEventDestroy(begin);
    cudaEventDestroy(end);
    return cudaDeviceSynchronize() == cudaSuccess;
}

static bool warmupForce(CudaJitRuntime::Impl& impl, float* posX, float* posY, float* posZ,
                        float* mass, Vector3* output, int particleCount, float softening,
                        float minDistance2, float maxAcceleration, const CudaJitRequest& request,
                        CudaJitMetrics* metrics)
{
    cudaEvent_t begin = nullptr;
    cudaEvent_t end = nullptr;
    if (cudaEventCreate(&begin) != cudaSuccess || cudaEventCreate(&end) != cudaSuccess) {
        return false;
    }
    const int repetitions = 2;
    cudaMemset(impl.divergenceCounter, 0, sizeof(unsigned int));
    cudaEventRecord(begin);
    for (int i = 0; i < repetitions; ++i) {
        if (!launchStaticForce(posX, posY, posZ, mass, output, particleCount, softening,
                               minDistance2, maxAcceleration, request.blockSize,
                               impl.divergenceCounter)) {
            cudaEventDestroy(begin);
            cudaEventDestroy(end);
            return false;
        }
    }
    cudaEventRecord(end);
    cudaEventSynchronize(end);
    metrics->staticMs = blitzar_cuda_jit_runtime::elapsedMs(begin, end) /
                        static_cast<double>(repetitions);

    cudaMemset(impl.divergenceCounter, 0, sizeof(unsigned int));
    cudaEventRecord(begin);
    for (int i = 0; i < repetitions; ++i) {
        if (!launchJitForce(impl, posX, posY, posZ, mass, output, particleCount, softening,
                            minDistance2, maxAcceleration, request.blockSize, request.tileSize)) {
            cudaEventDestroy(begin);
            cudaEventDestroy(end);
            return false;
        }
    }
    cudaEventRecord(end);
    cudaEventSynchronize(end);
    metrics->jitMs = blitzar_cuda_jit_runtime::elapsedMs(begin, end) /
                     static_cast<double>(repetitions);
    unsigned int divergentWarps = 0u;
    cudaMemcpy(&divergentWarps, impl.divergenceCounter, sizeof(divergentWarps),
               cudaMemcpyDeviceToHost);
    const int warpsPerBlock = (request.blockSize + 31) / 32;
    const double totalWarps =
        static_cast<double>((particleCount + request.blockSize - 1) / request.blockSize) *
        warpsPerBlock;
    metrics->divergentWarpFraction =
        totalWarps <= 0.0 ? 0.0f : static_cast<float>(divergentWarps / totalWarps);
    metrics->divergenceInstrumented = true;
    metrics->warmupAccepted = metrics->jitMs <= metrics->staticMs * 1.02;
    cudaEventDestroy(begin);
    cudaEventDestroy(end);
    return cudaDeviceSynchronize() == cudaSuccess;
}

static void destroyGraph(CudaJitRuntime::Impl& impl)
{
    if (impl.graphExec != nullptr) {
        cudaGraphExecDestroy(impl.graphExec);
        impl.graphExec = nullptr;
    }
    if (impl.graph != nullptr) {
        cudaGraphDestroy(impl.graph);
        impl.graph = nullptr;
    }
}

static bool captureGraph(CudaJitRuntime::Impl& impl, float* fieldX, float* fieldY, float* fieldZ,
                         int totalCells, float scale, int blockSize)
{
    destroyGraph(impl);
    if (cudaStreamBeginCapture(impl.graphStream, cudaStreamCaptureModeGlobal) != cudaSuccess) {
        return false;
    }
    const bool launched = launchJit(impl, fieldX, totalCells, scale, blockSize, impl.graphStream) &&
                          launchJit(impl, fieldY, totalCells, scale, blockSize, impl.graphStream) &&
                          launchJit(impl, fieldZ, totalCells, scale, blockSize, impl.graphStream);
    const cudaError_t endStatus = cudaStreamEndCapture(impl.graphStream, &impl.graph);
    if (!launched || endStatus != cudaSuccess) {
        cudaGetLastError();
        destroyGraph(impl);
        return false;
    }
    if (cudaGraphInstantiate(&impl.graphExec, impl.graph, nullptr, nullptr, 0u) != cudaSuccess) {
        destroyGraph(impl);
        return false;
    }
    impl.graphX = fieldX;
    impl.graphY = fieldY;
    impl.graphZ = fieldZ;
    impl.graphCells = totalCells;
    impl.graphScale = scale;
    impl.graphBlockSize = blockSize;
    return true;
}

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
