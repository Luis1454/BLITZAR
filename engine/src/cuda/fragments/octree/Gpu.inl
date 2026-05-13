/*
 * @file engine/src/cuda/fragments/octree/Gpu.inl
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Physics and CUDA implementation for the deterministic simulation core.
 */

/*
 * Module: cuda
 * Responsibility: Implement GPU octree helpers and traversal kernels.
 */

/*
 * @brief Documents the octree node contains hot operation contract.
 * @param node Input value used by this contract.
 * @param pos Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__device__ __forceinline__ bool octreeNodeContainsHot(const GpuOctreeNodeHotData& node,
                                                      const Vector3& pos)
{
    return fabsf(pos.x - node.centerX) <= node.halfSize &&
           fabsf(pos.y - node.centerY) <= node.halfSize &&
           fabsf(pos.z - node.centerZ) <= node.halfSize;
}

/*
 * @brief Documents the octree node distance to bounds hot operation contract.
 * @param node Input value used by this contract.
 * @param pos Input value used by this contract.
 * @return float value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__device__ __forceinline__ float octreeNodeDistanceToBoundsHot(const GpuOctreeNodeHotData& node,
                                                               const Vector3& pos)
{
    const float dx = fmaxf(fabsf(pos.x - node.centerX) - node.halfSize, 0.0f);
    const float dy = fmaxf(fabsf(pos.y - node.centerY) - node.halfSize, 0.0f);
    const float dz = fmaxf(fabsf(pos.z - node.centerZ) - node.halfSize, 0.0f);
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

/*
 * @brief Documents the octree node squared distance to bounds operation contract.
 * @param node Input value used by this contract.
 * @param pos Input value used by this contract.
 * @return float value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__device__ __forceinline__ float octreeNodeDistanceToBoundsSquaredHot(
    const GpuOctreeNodeHotData& node, const Vector3& pos)
{
    const float dx = fmaxf(fabsf(pos.x - node.centerX) - node.halfSize, 0.0f);
    const float dy = fmaxf(fabsf(pos.y - node.centerY) - node.halfSize, 0.0f);
    const float dz = fmaxf(fabsf(pos.z - node.centerZ) - node.halfSize, 0.0f);
    return dx * dx + dy * dy + dz * dz;
}

/*
 * @brief Documents the octree load particle position operation contract.
 * @param state Input value used by this contract.
 * @param index Input value used by this contract.
 * @return Vector3 value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__device__ __forceinline__ Vector3 octreeLoadParticlePosition(ParticleSoAView state, int index)
{
    return Vector3(__ldg(&state.posX[index]), __ldg(&state.posY[index]), __ldg(&state.posZ[index]));
}

/*
 * @brief Documents the octree load particle mass operation contract.
 * @param state Input value used by this contract.
 * @param index Input value used by this contract.
 * @return float value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__device__ __forceinline__ float octreeLoadParticleMass(ParticleSoAView state, int index)
{
    return __ldg(&state.mass[index]);
}

/*
 * @brief Documents the octree load particle velocity operation contract.
 * @param state Input value used by this contract.
 * @param index Input value used by this contract.
 * @return Vector3 value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__device__ __forceinline__ Vector3 octreeLoadParticleVelocity(ParticleSoAView state, int index)
{
    return Vector3(__ldg(&state.velX[index]), __ldg(&state.velY[index]), __ldg(&state.velZ[index]));
}

/*
 * @brief Optimized traversal: 4x leaf unrolling + restrict pointers + prefetching.
 * Uses early exits, loop unrolling, and __ldg for cache locality.
 */
__device__ Vector3 computeOctreeAccelerationStacklessCompact(
    ParticleSoAView state, int selfIndex, const GpuOctreeNodeHotData* __restrict__ nodeHot,
    const GpuOctreeNodeNavData* __restrict__ nodeNav, IndexConstHandle nodeFirstChild,
    IndexConstHandle leafStarts, IndexConstHandle leafCounts, int rootIndex,
    IndexConstHandle leafIndices, ForceLawPolicy forceLaw, float maxAcceleration,
    int openingCriterion, float cutoffSquared)
{
    constexpr int kMaxTraversalIterations = 8192;

    const Vector3 selfPos = octreeLoadParticlePosition(state, selfIndex);
    Vector3 force(0.0f, 0.0f, 0.0f);
    const bool cutoffEnabled = cutoffSquared > 0.0f;

    int traversalIterations = 0;
    int nodeIndex = rootIndex;

    while (nodeIndex >= 0) {
        if (++traversalIterations > kMaxTraversalIterations) {
            break;
        }

        const GpuOctreeNodeHotData node = nodeHot[nodeIndex];
        const GpuOctreeNodeNavData nav = nodeNav[nodeIndex];

        // Early exit for empty nodes
        if (node.mass <= 0.0f) {
            nodeIndex = nav.nextIndex;
            continue;
        }

        if (cutoffEnabled) {
            const float nodeDistanceSquared = octreeNodeDistanceToBoundsSquaredHot(node, selfPos);
            if (nodeDistanceSquared > cutoffSquared && !octreeNodeContainsHot(node, selfPos)) {
                nodeIndex = nav.nextIndex;
                continue;
            }
        }

        // Leaf node handling with 4x loop unrolling to expose ILP
        if (nav.childMask == 0u) {
            const int leafStart = __ldg(&leafStarts[nodeIndex]);
            const int leafCount = __ldg(&leafCounts[nodeIndex]);

            // 4x unroll: process 4 particles per iteration
            int k = 0;
            const int leafCountAligned = (leafCount / 4) * 4;

            for (; k < leafCountAligned; k += 4) {
                #pragma unroll 4
                for (int j = 0; j < 4; ++j) {
                    const int otherIndex = __ldg(&leafIndices[leafStart + k + j]);
                    if (otherIndex != selfIndex) {
                        const Vector3 sourcePos = octreeLoadParticlePosition(state, otherIndex);
                        if (cutoffEnabled) {
                            const Vector3 diff(selfPos.x - sourcePos.x, selfPos.y - sourcePos.y,
                                               selfPos.z - sourcePos.z);
                            if (softenedDistanceSquared(diff, forceLaw) > cutoffSquared) {
                                continue;
                            }
                        }
                        force += blitzarAccelerationFromSource(
                            selfPos, sourcePos,
                            octreeLoadParticleMass(state, otherIndex), forceLaw);
                    }
                }
            }

            // Handle remainder
            for (; k < leafCount; ++k) {
                const int otherIndex = __ldg(&leafIndices[leafStart + k]);
                if (otherIndex != selfIndex) {
                    const Vector3 sourcePos = octreeLoadParticlePosition(state, otherIndex);
                    if (cutoffEnabled) {
                        const Vector3 diff(selfPos.x - sourcePos.x, selfPos.y - sourcePos.y,
                                           selfPos.z - sourcePos.z);
                        if (softenedDistanceSquared(diff, forceLaw) > cutoffSquared) {
                            continue;
                        }
                    }
                    force += blitzarAccelerationFromSource(
                        selfPos, sourcePos,
                        octreeLoadParticleMass(state, otherIndex), forceLaw);
                }
            }

            nodeIndex = nav.nextIndex;
            continue;
        }

        // Internal node handling: test opening criterion
        const Vector3 direction(node.comX - selfPos.x, node.comY - selfPos.y,
                                node.comZ - selfPos.z);
        const float dist2 = softenedDistanceSquared(direction, forceLaw);
        const bool containsSelf = octreeNodeContainsHot(node, selfPos);
        const float size = node.halfSize * 2.0f;
        const float thetaSquared = forceLaw.theta * forceLaw.theta;
        const float sizeSquared = size * size;
        bool acceptNode = false;
        if (!containsSelf) {
            if (openingCriterion == 1) {
                const float boundsDistanceSquared =
                    fmaxf(octreeNodeDistanceToBoundsSquaredHot(node, selfPos), 1.0e-12f);
                acceptNode = sizeSquared < thetaSquared * boundsDistanceSquared;
            } else {
                acceptNode = sizeSquared < thetaSquared * dist2;
            }
        }

        if (acceptNode) {
            if (cutoffEnabled) {
                const float boundsDistanceSquared = octreeNodeDistanceToBoundsSquaredHot(node, selfPos);
                if (boundsDistanceSquared > cutoffSquared && !containsSelf) {
                    nodeIndex = nav.nextIndex;
                    continue;
                }
            }
            force += blitzarAccelerationFromSource(
                selfPos, Vector3(node.comX, node.comY, node.comZ), node.mass, forceLaw);
            nodeIndex = nav.nextIndex;
            continue;
        }

        // Traverse into children: prefetch first child to hide latency
        const int firstChild = __ldg(&nodeFirstChild[nodeIndex]);
        nodeIndex = firstChild >= 0 ? firstChild : nav.nextIndex;
    }

    return clampAcceleration(force, maxAcceleration);
}

__global__ void computeOctreeAccelerationKernel(
    ParticleSoAView state, Vector3Handle outAcceleration, int numParticles,
    const GpuOctreeNodeHotData* nodeHot, const GpuOctreeNodeNavData* nodeNav,
    IndexConstHandle nodeFirstChild, IndexConstHandle leafStarts, IndexConstHandle leafCounts,
    int rootIndex, IndexConstHandle leafIndices, ForceLawPolicy forceLaw, float maxAcceleration,
    int openingCriterion, float cutoffSquared)
{
    const int particleIndex = blockIdx.x * blockDim.x + threadIdx.x;
    if (particleIndex >= numParticles || rootIndex < 0) {
        return;
    }

    outAcceleration[particleIndex] = computeOctreeAccelerationStacklessCompact(
        state, particleIndex, nodeHot, nodeNav, nodeFirstChild, leafStarts, leafCounts, rootIndex,
        leafIndices, forceLaw, maxAcceleration, openingCriterion, cutoffSquared);
}

__global__ void updateParticlesOctree(ParticleSoAView lastState, ParticleSoAView particlesOut,
                                      int numParticles, const GpuOctreeNodeHotData* nodeHot,
                                      const GpuOctreeNodeNavData* nodeNav,
                                      IndexConstHandle nodeFirstChild, IndexConstHandle leafStarts,
                                      IndexConstHandle leafCounts, int rootIndex,
                                      IndexConstHandle leafIndices, ForceLawPolicy forceLaw,
                                      float deltaTime, float maxAcceleration, int openingCriterion,
                                      float cutoffSquared)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles || rootIndex < 0) {
        return;
    }

    const Vector3 selfPos = octreeLoadParticlePosition(lastState, i);
    const Vector3 force = computeOctreeAccelerationStacklessCompact(
        lastState, i, nodeHot, nodeNav, nodeFirstChild, leafStarts, leafCounts, rootIndex,
        leafIndices, forceLaw, maxAcceleration, openingCriterion, cutoffSquared);
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
