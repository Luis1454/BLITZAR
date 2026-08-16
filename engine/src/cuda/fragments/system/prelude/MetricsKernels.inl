/*
 * @file engine/src/cuda/fragments/system/prelude/MetricsKernels.inl
 * @project BLITZAR
 * @brief Shared CUDA system helper or kernel implementation fragment.
 */

__global__ void publishMetricsKernel(GpuSystemMetrics* mappedMetrics, ParticleSoAView state,
                                     int numParticles, std::uint64_t stepId, float simTime,
                                     float dt, std::uint64_t vramUsedBytes,
                                     std::uint64_t vramPeakBytes)
{
    if (blockIdx.x != 0 || threadIdx.x != 0 || mappedMetrics == nullptr) {
        return;
    }

    GpuMetricsPayload payload{};
    payload.flags = static_cast<std::uint32_t>(kGpuMetricsValid | kGpuMetricsEstimated);
    payload.stepId = stepId;
    payload.simTime = simTime;
    payload.dt = dt;
    payload.particleCount = static_cast<std::uint32_t>(numParticles > 0 ? numParticles : 0);
    payload.nanCount = 0u;
    payload.infCount = 0u;
    payload.minSpeed = 0.0f;
    payload.maxSpeed = 0.0f;
    payload.kineticEnergy = 0.0f;
    payload.potentialEnergy = 0.0f;
    payload.totalEnergy = 0.0f;
    payload.vramUsedBytes = vramUsedBytes;
    payload.vramPeakBytes = vramPeakBytes;

    if (numParticles > 0) {
        constexpr int kMaxSamples = 512;
        const int sampleCount = numParticles < kMaxSamples ? numParticles : kMaxSamples;
        const int stride = sampleCount > 0 ? max(1, numParticles / sampleCount) : 1;
        float minSpeed = FLT_MAX;
        float maxSpeed = 0.0f;
        float kinetic = 0.0f;
        int counted = 0;

        for (int i = 0; i < numParticles && counted < sampleCount; i += stride, ++counted) {
            const float vx = state.velX[i];
            const float vy = state.velY[i];
            const float vz = state.velZ[i];
            const float mass = state.mass[i];
            const float speed2 = vx * vx + vy * vy + vz * vz;
            const float speed = sqrtf(speed2);

            if (!isfinite(speed) || !isfinite(mass)) {
                if (!isfinite(speed)) {
                    payload.infCount += 1u;
                }
                if (isnan(speed) || isnan(mass)) {
                    payload.nanCount += 1u;
                }
                continue;
            }

            minSpeed = fminf(minSpeed, speed);
            maxSpeed = fmaxf(maxSpeed, speed);
            kinetic += 0.5f * mass * speed2;
        }

        if (counted > 0 && sampleCount > 0) {
            const float scale = static_cast<float>(numParticles) / static_cast<float>(sampleCount);
            payload.minSpeed = minSpeed == FLT_MAX ? 0.0f : minSpeed;
            payload.maxSpeed = maxSpeed;
            payload.kineticEnergy = kinetic * scale;
            payload.totalEnergy = payload.kineticEnergy;
        }
    }

    volatile std::uint32_t* sequence = &mappedMetrics->sequence;
    const std::uint32_t observed = *sequence;
    const std::uint32_t evenBase = (observed & 1u) == 0u ? observed : (observed + 1u);
    const std::uint32_t odd = evenBase + 1u;
    const std::uint32_t even = evenBase + 2u;

    *sequence = odd;
    __threadfence_system();

    mappedMetrics->flags = payload.flags;
    mappedMetrics->stepId = payload.stepId;
    mappedMetrics->simTime = payload.simTime;
    mappedMetrics->dt = payload.dt;
    mappedMetrics->particleCount = payload.particleCount;
    mappedMetrics->nanCount = payload.nanCount;
    mappedMetrics->infCount = payload.infCount;
    mappedMetrics->minSpeed = payload.minSpeed;
    mappedMetrics->maxSpeed = payload.maxSpeed;
    mappedMetrics->kineticEnergy = payload.kineticEnergy;
    mappedMetrics->potentialEnergy = payload.potentialEnergy;
    mappedMetrics->totalEnergy = payload.totalEnergy;
    mappedMetrics->vramUsedBytes = payload.vramUsedBytes;
    mappedMetrics->vramPeakBytes = payload.vramPeakBytes;
    mappedMetrics->reserved0 = 0u;
    mappedMetrics->reservedAlignment = 0u;
    mappedMetrics->reserved1 = 0u;
    mappedMetrics->reserved2 = 0u;
    mappedMetrics->reserved3 = 0u;
    mappedMetrics->reserved4 = 0u;
    mappedMetrics->reserved5 = 0u;
    mappedMetrics->reserved6 = 0u;

    __threadfence_system();
    *sequence = even;
}

/*
 * @brief Documents the update particles operation contract.
 * @param last Input value used by this contract.
 * @param current Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @param deltaTime Input value used by this contract.
 * @param policy Input value used by this contract.
 * @param maxAcceleration Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
