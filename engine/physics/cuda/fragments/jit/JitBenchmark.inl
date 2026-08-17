/*
 * @file engine/physics/cuda/fragments/jit/JitBenchmark.inl
 * @brief Warm-up comparison and CUDA Graph capture helpers.
 */

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

