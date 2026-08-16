/*
 * @file engine/src/cuda/fragments/jit/Source.inl
 * @brief Static fallback kernels and NVRTC source template.
 */

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

