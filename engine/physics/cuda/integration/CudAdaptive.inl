/*
 * @file engine/physics/cuda/integration/CudAdaptive.inl
 * @brief GPU-native dyadic individual time-step kernels.
 */

/*
 * The tree and PM structures are deliberately held constant during one global
 * step. Particle positions are predicted on every micro-tick, while the force
 * for an active particle is recomputed against that predicted state.
 */

struct AdaptiveGpuForceContext final {
    int mode;
    const GpuOctreeNodeHotData* nodeHot;
    const GpuOctreeNodeNavData* nodeNav;
    IndexConstHandle nodeFirstChild;
    IndexConstHandle leafStarts;
    IndexConstHandle leafCounts;
    int rootIndex;
    IndexConstHandle leafIndices;
    ForceLawPolicy forceLaw;
    float maxAcceleration;
    int openingCriterion;
    TreePmGridParams grid;
    IndexConstHandle sortedIndex;
    IndexConstHandle cellStart;
    IndexConstHandle cellEnd;
    const float* pmAccelX;
    const float* pmAccelY;
    const float* pmAccelZ;
    const unsigned int* cellMask;
    float cutoffSquared;
    int cellRadius;
    int maxLocalNeighbors;
    int denseCellThreshold;
    const float* sortedPosX;
    const float* sortedPosY;
    const float* sortedPosZ;
    const float* sortedMass;
};

__device__ __forceinline__ float adaptiveStableDt(Vector3 acceleration, Vector3 velocity,
                                                   float eta, float softening, float globalDt)
{
    const float safeSoftening = fmaxf(softening, 1.0e-6f);
    const float accelerationMagnitude = sqrtf(dot(acceleration, acceleration));
    const float velocityMagnitude = sqrtf(dot(velocity, velocity));
    const float accelerationDt = accelerationMagnitude > 1.0e-6f
                                     ? eta * sqrtf(safeSoftening / accelerationMagnitude)
                                     : globalDt;
    const float velocityDt = velocityMagnitude > 1.0e-6f
                                 ? eta * safeSoftening / velocityMagnitude
                                 : globalDt;
    return fminf(globalDt, fminf(accelerationDt, velocityDt));
}

__device__ __forceinline__ unsigned char adaptiveChooseLevel(
    Vector3 acceleration, Vector3 velocity, float eta, float softening, float globalDt,
    int maxLevel)
{
    const float stableDt = adaptiveStableDt(acceleration, velocity, eta, softening, globalDt);
    unsigned char selected = static_cast<unsigned char>(maxLevel);
    for (int level = 0; level <= maxLevel; ++level) {
        const float levelDt = globalDt / static_cast<float>(1u << level);
        if (levelDt <= stableDt) {
            selected = static_cast<unsigned char>(level);
            break;
        }
    }
    return selected;
}

__device__ __forceinline__ Vector3 adaptiveClampVelocity(Vector3 velocity, float maxSpeed)
{
    if (maxSpeed <= 0.0f) {
        return velocity;
    }
    const float magnitude = sqrtf(dot(velocity, velocity));
    return magnitude > maxSpeed ? velocity * (maxSpeed / magnitude) : velocity;
}

__device__ __forceinline__ Vector3 adaptiveComputeOctreeForce(
    ParticleSoAView state, int particleIndex, const AdaptiveGpuForceContext& context)
{
    if (context.mode == 1) {
        return treepm::treePmComputeLocalGridAcceleration(
            state, particleIndex, context.grid, context.sortedIndex, context.cellStart,
            context.cellEnd, context.forceLaw, context.maxAcceleration, context.pmAccelX,
            context.pmAccelY, context.pmAccelZ, context.cellMask, context.cutoffSquared,
            context.cellRadius, context.maxLocalNeighbors, context.sortedPosX,
            context.sortedPosY, context.sortedPosZ, context.sortedMass);
    }
    if (context.mode == 2) {
        return treepm::treePmComputeAcceleration(
            state, particleIndex, context.nodeHot, context.nodeNav, context.nodeFirstChild,
            context.leafStarts, context.leafCounts, context.rootIndex, context.leafIndices,
            context.forceLaw, context.maxAcceleration, context.openingCriterion,
            context.cutoffSquared, context.grid, context.pmAccelX, context.pmAccelY,
            context.pmAccelZ);
    }
    if (context.mode == 3) {
        return treepm::treePmComputeHybridAcceleration(
            state, particleIndex, context.nodeHot, context.nodeNav, context.nodeFirstChild,
            context.leafStarts, context.leafCounts, context.rootIndex, context.leafIndices,
            context.forceLaw, context.maxAcceleration, context.openingCriterion,
            context.cutoffSquared, context.grid, context.sortedIndex, context.cellStart,
            context.cellEnd, context.pmAccelX, context.pmAccelY, context.pmAccelZ,
            context.cellMask, context.cellRadius, context.maxLocalNeighbors,
            context.denseCellThreshold, context.sortedPosX, context.sortedPosY,
            context.sortedPosZ, context.sortedMass);
    }
    return computeOctreeAccelerationStacklessCompact(
        state, particleIndex, context.nodeHot, context.nodeNav, context.nodeFirstChild,
        context.leafStarts, context.leafCounts, context.rootIndex, context.leafIndices,
        context.forceLaw, context.maxAcceleration, context.openingCriterion,
        context.cutoffSquared);
}

__global__ void initializeAdaptiveScheduleKernel(
    ParticleSoAView state, Vector3ConstHandle acceleration, unsigned char* levels,
    unsigned long long* lastForceTicks, int numParticles, int maxLevel, float eta,
    float softening, float globalDt)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }
    levels[i] = adaptiveChooseLevel(acceleration[i], getSoAVelocity(state, i), eta, softening,
                                    globalDt, maxLevel);
    lastForceTicks[i] = 0ull;
}

__global__ void computeAdaptiveForceKernel(ParticleSoAView state, Vector3Handle output,
                                           int numParticles, AdaptiveGpuForceContext context)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < numParticles) {
        output[i] = adaptiveComputeOctreeForce(state, i, context);
    }
}

__global__ void adaptiveDriftKernel(ParticleSoAView current, ParticleSoAView output,
                                    Vector3ConstHandle acceleration, int numParticles,
                                    float quantum, float maxSpeed)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }
    const Vector3 position = getSoAPosition(current, i);
    const Vector3 velocity = getSoAVelocity(current, i);
    const Vector3 nextPosition = position + velocity * quantum +
                                 acceleration[i] * (0.5f * quantum * quantum);
    setSoAPosition(output, i, nextPosition);
    setSoAVelocity(output, i, adaptiveClampVelocity(velocity + acceleration[i] * quantum,
                                                    maxSpeed));
    setSoAPressure(output, i, getSoAPressure(current, i));
    output.mass[i] = __ldg(&current.mass[i]);
    if (output.temp != nullptr && current.temp != nullptr) {
        output.temp[i] = __ldg(&current.temp[i]);
    }
    if (output.dens != nullptr && current.dens != nullptr) {
        output.dens[i] = __ldg(&current.dens[i]);
    }
}

__device__ __forceinline__ bool adaptiveParticleIsActive(
    unsigned char level, unsigned long long targetTick, int maxLevel)
{
    const unsigned int boundedLevel = min(static_cast<unsigned int>(level),
                                          static_cast<unsigned int>(maxLevel));
    const unsigned int cadence = 1u << (maxLevel - boundedLevel);
    return targetTick % static_cast<unsigned long long>(cadence) == 0ull;
}

__global__ void adaptivePairwiseCorrectKernel(
    ParticleSoAView state, Vector3Handle acceleration, unsigned char* levels,
    unsigned long long* lastForceTicks, int numParticles, ForceLawPolicy forceLaw,
    float maxAcceleration, float quantum, int maxLevel, float eta, float softening,
    float globalDt, unsigned long long targetTick, float maxSpeed)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }
    const Vector3 oldAcceleration = acceleration[i];
    Vector3 force = oldAcceleration;
    if (adaptiveParticleIsActive(levels[i], targetTick, maxLevel)) {
        force = computePairwiseAcceleration(state, numParticles, i, forceLaw, maxAcceleration);
        const float localDt = static_cast<float>(targetTick - lastForceTicks[i]) * quantum;
        const Vector3 correction = force - oldAcceleration;
        setSoAPosition(state, i, getSoAPosition(state, i) + correction * (0.5f * localDt * localDt));
        setSoAVelocity(state, i, adaptiveClampVelocity(
                                      getSoAVelocity(state, i) + correction * localDt, maxSpeed));
        acceleration[i] = force;
        lastForceTicks[i] = targetTick;
        levels[i] = adaptiveChooseLevel(force, getSoAVelocity(state, i), eta, softening, globalDt,
                                        maxLevel);
    }
    setSoAPressure(state, i, force * 100.0f);
}

__global__ void adaptiveOctreeCorrectKernel(
    ParticleSoAView state, Vector3Handle acceleration, unsigned char* levels,
    unsigned long long* lastForceTicks, int numParticles, AdaptiveGpuForceContext context,
    float quantum, int maxLevel, float eta, float softening, float globalDt,
    unsigned long long targetTick, float maxSpeed)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }
    const Vector3 oldAcceleration = acceleration[i];
    Vector3 force = oldAcceleration;
    if (adaptiveParticleIsActive(levels[i], targetTick, maxLevel)) {
        force = adaptiveComputeOctreeForce(state, i, context);
        const float localDt = static_cast<float>(targetTick - lastForceTicks[i]) * quantum;
        const Vector3 correction = force - oldAcceleration;
        setSoAPosition(state, i, getSoAPosition(state, i) + correction * (0.5f * localDt * localDt));
        setSoAVelocity(state, i, adaptiveClampVelocity(
                                      getSoAVelocity(state, i) + correction * localDt, maxSpeed));
        acceleration[i] = force;
        lastForceTicks[i] = targetTick;
        levels[i] = adaptiveChooseLevel(force, getSoAVelocity(state, i), eta, softening, globalDt,
                                        maxLevel);
    }
    setSoAPressure(state, i, force * 100.0f);
}
