/*
 * Module: physics/cuda
 * Responsibility: Implement GPU octree helpers and traversal kernels.
 */

/// Description: Describes the octree node contains hot operation contract.
__device__ __forceinline__ bool octreeNodeContainsHot(const GpuOctreeNodeHotData &node,
                                                       const Vector3 &pos)
{
    return fabsf(pos.x - node.centerX) <= node.halfSize
        && fabsf(pos.y - node.centerY) <= node.halfSize
        && fabsf(pos.z - node.centerZ) <= node.halfSize;
}

/// Description: Describes the octree node distance to bounds hot operation contract.
__device__ __forceinline__ float octreeNodeDistanceToBoundsHot(const GpuOctreeNodeHotData &node,
                                                                const Vector3 &pos)
{
    const float dx = fmaxf(fabsf(pos.x - node.centerX) - node.halfSize, 0.0f);
    const float dy = fmaxf(fabsf(pos.y - node.centerY) - node.halfSize, 0.0f);
    const float dz = fmaxf(fabsf(pos.z - node.centerZ) - node.halfSize, 0.0f);
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

/// Description: Executes the octreeLoadParticlePosition operation.
__device__ __forceinline__ Vector3 octreeLoadParticlePosition(ParticleSoAView state, int index)
{
    return Vector3(__ldg(&state.posX[index]), __ldg(&state.posY[index]), __ldg(&state.posZ[index]));
}

/// Description: Executes the octreeLoadParticleMass operation.
__device__ __forceinline__ float octreeLoadParticleMass(ParticleSoAView state, int index)
{
    return __ldg(&state.mass[index]);
}

/// Description: Executes the octreeLoadParticleVelocity operation.
__device__ __forceinline__ Vector3 octreeLoadParticleVelocity(ParticleSoAView state, int index)
{
    return Vector3(__ldg(&state.velX[index]), __ldg(&state.velY[index]), __ldg(&state.velZ[index]));
}

__device__ Vector3 computeOctreeAccelerationStacklessCompact(
    ParticleSoAView state,
    int selfIndex,
    const GpuOctreeNodeHotData *nodeHot,
    const GpuOctreeNodeNavData *nodeNav,
    IndexConstHandle nodeFirstChild,
    IndexConstHandle leafStarts,
    IndexConstHandle leafCounts,
    int rootIndex,
    IndexConstHandle leafIndices,
    ForceLawPolicy forceLaw,
    float maxAcceleration,
    int openingCriterion)
{
    constexpr int kMaxTraversalIterations = 8192;

    const Vector3 selfPos = octreeLoadParticlePosition(state, selfIndex);
    Vector3 force(0.0f, 0.0f, 0.0f);

    int traversalIterations = 0;
    int nodeIndex = rootIndex;
    while (nodeIndex >= 0) {
        if (++traversalIterations > kMaxTraversalIterations) {
            break;
        }

        const GpuOctreeNodeHotData node = nodeHot[nodeIndex];
        const GpuOctreeNodeNavData nav = nodeNav[nodeIndex];
        if (node.mass <= 0.0f) {
            nodeIndex = nav.nextIndex;
            continue;
        }

        if (nav.childMask == 0u) {
            const int leafStart = __ldg(&leafStarts[nodeIndex]);
            const int leafCount = __ldg(&leafCounts[nodeIndex]);
            for (int k = 0; k < leafCount; ++k) {
                const int otherIndex = __ldg(&leafIndices[leafStart + k]);
                if (otherIndex == selfIndex) {
                    continue;
                }
                force += gravityAccelerationFromSource(
                    selfPos,
                    octreeLoadParticlePosition(state, otherIndex),
                    octreeLoadParticleMass(state, otherIndex),
                    forceLaw);
            }
            nodeIndex = nav.nextIndex;
            continue;
        }

        const Vector3 direction(node.comX - selfPos.x, node.comY - selfPos.y, node.comZ - selfPos.z);
        const float dist2 = softenedDistanceSquared(direction, forceLaw);
        const bool containsSelf = octreeNodeContainsHot(node, selfPos);
        const float size = node.halfSize * 2.0f;
        const float criterionDistance = openingCriterion == 1
            ? fmaxf(octreeNodeDistanceToBoundsHot(node, selfPos), 1.0e-6f)
            : sqrtf(dist2);
        const bool acceptNode = !containsSelf && (size / criterionDistance) < forceLaw.theta;

        if (acceptNode) {
            force += gravityAccelerationFromSource(
                selfPos,
                Vector3(node.comX, node.comY, node.comZ),
                node.mass,
                forceLaw);
            nodeIndex = nav.nextIndex;
            continue;
        }

        const int firstChild = __ldg(&nodeFirstChild[nodeIndex]);
        nodeIndex = firstChild >= 0 ? firstChild : nav.nextIndex;
    }

    return clampAcceleration(force, maxAcceleration);
}

__global__ void computeOctreeAccelerationKernel(
    ParticleSoAView state,
    Vector3Handle outAcceleration,
    int numParticles,
    const GpuOctreeNodeHotData *nodeHot,
    const GpuOctreeNodeNavData *nodeNav,
    IndexConstHandle nodeFirstChild,
    IndexConstHandle leafStarts,
    IndexConstHandle leafCounts,
    int rootIndex,
    IndexConstHandle leafIndices,
    ForceLawPolicy forceLaw,
    float maxAcceleration,
    int openingCriterion)
{
    const int particleIndex = blockIdx.x * blockDim.x + threadIdx.x;
    if (particleIndex >= numParticles || rootIndex < 0) {
        return;
    }

    outAcceleration[particleIndex] = computeOctreeAccelerationStacklessCompact(
        state,
        particleIndex,
        nodeHot,
        nodeNav,
        nodeFirstChild,
        leafStarts,
        leafCounts,
        rootIndex,
        leafIndices,
        forceLaw,
        maxAcceleration,
        openingCriterion);
}

__global__ void updateParticlesOctree(
    ParticleSoAView lastState,
    ParticleSoAView particlesOut,
    int numParticles,
    const GpuOctreeNodeHotData *nodeHot,
    const GpuOctreeNodeNavData *nodeNav,
    IndexConstHandle nodeFirstChild,
    IndexConstHandle leafStarts,
    IndexConstHandle leafCounts,
    int rootIndex,
    IndexConstHandle leafIndices,
    ForceLawPolicy forceLaw,
    float deltaTime,
    float maxAcceleration,
    int openingCriterion)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles || rootIndex < 0) {
        return;
    }

    const Vector3 selfPos = octreeLoadParticlePosition(lastState, i);
    const Vector3 force = computeOctreeAccelerationStacklessCompact(
        lastState,
        i,
        nodeHot,
        nodeNav,
        nodeFirstChild,
        leafStarts,
        leafCounts,
        rootIndex,
        leafIndices,
        forceLaw,
        maxAcceleration,
        openingCriterion);
    const Vector3 vel = octreeLoadParticleVelocity(lastState, i);
    const Vector3 nextVel = vel + force * deltaTime;
    const Vector3 nextPos = selfPos + nextVel * deltaTime;

    setSoAPressure(particlesOut, i, force * 100.0f);
    setSoAVelocity(particlesOut, i, nextVel);
    setSoAPosition(particlesOut, i, nextPos);

    particlesOut.mass[i] = __ldg(&lastState.mass[i]);
    particlesOut.temp[i] = __ldg(&lastState.temp[i]);
    particlesOut.dens[i] = __ldg(&lastState.dens[i]);
}
