/*
 * @file engine/physics/cuda/fragments/jit/JitExecution.inl
 * @brief Static and JIT kernel launch helpers.
 */

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

