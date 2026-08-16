/*
 * @file engine/src/cuda/fragments/jit/Cache.inl
 * @brief CUDA driver setup, cache keys, compilation and module loading.
 */

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

