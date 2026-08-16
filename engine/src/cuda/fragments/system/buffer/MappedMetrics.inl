/*
 * @file engine/src/cuda/fragments/system/buffer/MappedMetrics.inl
 * @project BLITZAR
 * @brief Particle-system buffer lifecycle implementation fragment.
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
        _device._mappedMetricsHost.reset();
        _device._mappedMetricsHost = nullptr;
        return false;
    }
    _device._mappedMetricsDevice = reinterpret_cast<std::uintptr_t>(devicePtr);
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
    _device._mappedMetricsHost.reset();
    _device._mappedMetricsDevice = 0u;
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
