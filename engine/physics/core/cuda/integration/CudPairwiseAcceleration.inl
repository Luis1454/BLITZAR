/*
 * @file engine/physics/core/cuda/integration/CudPairwiseAcceleration.inl
 * @author Luis1454
 * @project BLITZAR
 * @brief Launch the specialized or static pairwise CUDA force kernel.
 */

/*
 * Module: cuda
 * Responsibility: Dispatch force-tile JIT with a static fallback.
 */

bool ParticleSystem::launchPairwiseAcceleration(ParticleSoAView view, Vector3* output,
                                                int numParticles, const ForceLawPolicy& forceLaw)
{
    constexpr int kPairwiseCudaBlockSize = 128;
    constexpr std::size_t kPairwiseSharedBytes = 4u * kPairwiseCudaBlockSize * sizeof(float);

    CudaJitRequest jitRequest;
    jitRequest.family = CudaJitFamily::ForceTile;
    jitRequest.blockSize = kPairwiseCudaBlockSize;
    jitRequest.tileSize = kPairwiseCudaBlockSize;
    jitRequest.softeningMode = forceLaw.softening > 0.0f ? 1 : 0;
    jitRequest.softening = forceLaw.softening;
    CudaJitMetrics jitMetrics;
    const bool usedJit =
        _device->_cudaJit != nullptr &&
        _device->_cudaJit->launchForceTile(view.posX, view.posY, view.posZ, view.mass, output,
                                          numParticles, forceLaw.softening, forceLaw.minDistance2,
                                          _physicsMaxAcceleration, jitRequest, &jitMetrics);
    if (_device->_cudaJit != nullptr && !_device->_cudaJitForceMarkerPrinted) {
        fprintf(stderr,
                "[cuda-jit] family=force_tile backend=%s cache=%s accepted=%u registers=%u "
                "shared_bytes=%u active_blocks_sm=%u occupancy=%.3f "
                "divergent_warp_fraction=%.6f warmup_static_ms=%.4f "
                "warmup_jit_ms=%.4f compile_ms=%.3f\n",
                usedJit ? "jit" : "static-fallback",
                jitMetrics.cacheSource.empty() ? "unknown" : jitMetrics.cacheSource.c_str(),
                jitMetrics.warmupAccepted ? 1u : 0u, jitMetrics.registersPerThread,
                jitMetrics.staticSharedBytes, jitMetrics.activeBlocksPerSm, jitMetrics.occupancy,
                jitMetrics.divergentWarpFraction, jitMetrics.staticMs, jitMetrics.jitMs,
                jitMetrics.compileMs);
        _device->_cudaJitForceMarkerPrinted = true;
    }
    if (usedJit) {
        return true;
    }

    computePairwiseAccelerationKernelTiled<<<(numParticles + kPairwiseCudaBlockSize - 1) /
                                                 kPairwiseCudaBlockSize,
                                             kPairwiseCudaBlockSize, kPairwiseSharedBytes>>>(
        view, output, numParticles, forceLaw, _physicsMaxAcceleration);
    return checkCudaStatus(cudaGetLastError(), "computePairwiseAccelerationKernelTiled launch");
}
