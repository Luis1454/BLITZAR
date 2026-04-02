/*
 * Module: physics/cuda
 * Responsibility: Implement GPU octree helpers and traversal kernels.
 */

__device__ bool octreeNodeContains(const GpuOctreeNode &node, const Vector3 &pos)
{
    return fabsf(pos.x - node.centerX) <= node.halfSize
        && fabsf(pos.y - node.centerY) <= node.halfSize
        && fabsf(pos.z - node.centerZ) <= node.halfSize;
}

__device__ float octreeNodeDistanceToBounds(const GpuOctreeNode &node, const Vector3 &pos)
{
    const float dx = fmaxf(fabsf(pos.x - node.centerX) - node.halfSize, 0.0f);
    const float dy = fmaxf(fabsf(pos.y - node.centerY) - node.halfSize, 0.0f);
    const float dz = fmaxf(fabsf(pos.z - node.centerZ) - node.halfSize, 0.0f);
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

__device__ int octreeFirstChildIndex(const GpuOctreeNode &node)
{
    for (int child = 0; child < 8; ++child) {
        if ((node.childMask & (1u << child)) != 0u) {
            return node.children[child];
        }
    }
    return -1;
}

__device__ Vector3 computeOctreeAccelerationStackless(
    ParticleSoAView state,
    int selfIndex,
    OctreeNodeConstHandle nodes,
    int rootIndex,
    IndexConstHandle leafIndices,
    ForceLawPolicy forceLaw,
    float maxAcceleration,
    int openingCriterion
)
{
    constexpr float kBarnesHutTheta = 0.5f;
    constexpr int kMaxTraversalIterations = 8192;

    const Vector3 selfPos = getSoAPosition(state, selfIndex);
    Vector3 force(0.0f, 0.0f, 0.0f);

    int traversalIterations = 0;
    int nodeIndex = rootIndex;
    while (nodeIndex >= 0) {
        if (++traversalIterations > kMaxTraversalIterations) {
            break;
        }

        const GpuOctreeNode node = nodes[nodeIndex];
        if (node.mass <= 0.0f) {
            nodeIndex = node.nextIndex;
            continue;
        }

        if (node.childMask == 0u) {
            for (int k = 0; k < node.leafCount; ++k) {
                const int otherIndex = leafIndices[node.leafStart + k];
                if (otherIndex == selfIndex) {
                    continue;
                }
                force += gravityAccelerationFromSource(
                    selfPos,
                    getSoAPosition(state, otherIndex),
                    state.mass[otherIndex],
                    forceLaw);
            }
            nodeIndex = node.nextIndex;
            continue;
        }

        const Vector3 direction(node.comX - selfPos.x, node.comY - selfPos.y, node.comZ - selfPos.z);
        const float dist2 = softenedDistanceSquared(direction, forceLaw);
        const bool containsSelf = octreeNodeContains(node, selfPos);
        const float size = node.halfSize * 2.0f;
        const float criterionDistance = openingCriterion == 1
            ? fmaxf(octreeNodeDistanceToBounds(node, selfPos), 1.0e-6f)
            : sqrtf(dist2);

        if (!containsSelf && (size / criterionDistance) < kBarnesHutTheta) {
            force += gravityAccelerationFromSource(
                selfPos,
                Vector3(node.comX, node.comY, node.comZ),
                node.mass,
                forceLaw);
            nodeIndex = node.nextIndex;
            continue;
        }

        const int childIndex = octreeFirstChildIndex(node);
        nodeIndex = childIndex >= 0 ? childIndex : node.nextIndex;
    }

    return clampAcceleration(force, maxAcceleration);
}

__global__ void computeOctreeAccelerationKernel(
    ParticleSoAView state,
    Vector3Handle outAcceleration,
    int numParticles,
    OctreeNodeConstHandle nodes,
    int rootIndex,
    IndexConstHandle leafIndices,
    ForceLawPolicy forceLaw,
    float maxAcceleration,
    int openingCriterion
) {
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles || rootIndex < 0) {
        return;
    }

    outAcceleration[i] = computeOctreeAccelerationStackless(
        state, i, nodes, rootIndex, leafIndices, forceLaw, maxAcceleration, openingCriterion);
}

__global__ void updateParticlesOctree(
    ParticleSoAView lastState,
    ParticleSoAView particlesOut,
    int numParticles,
    OctreeNodeConstHandle nodes,
    int rootIndex,
    IndexConstHandle leafIndices,
    ForceLawPolicy forceLaw,
    float deltaTime,
    float maxAcceleration,
    int openingCriterion
) {
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles || rootIndex < 0) {
        return;
    }

    const Vector3 selfPos = getSoAPosition(lastState, i);
    const Vector3 force = computeOctreeAccelerationStackless(
        lastState, i, nodes, rootIndex, leafIndices, forceLaw, maxAcceleration, openingCriterion);
    const Vector3 vel = getSoAVelocity(lastState, i);
    const Vector3 nextVel = vel + force * deltaTime;
    const Vector3 nextPos = selfPos + nextVel * deltaTime;

    setSoAPressure(particlesOut, i, force * 100.0f);
    setSoAVelocity(particlesOut, i, nextVel);
    setSoAPosition(particlesOut, i, nextPos);

    particlesOut.mass[i] = lastState.mass[i];
    particlesOut.temp[i] = lastState.temp[i];
    particlesOut.dens[i] = lastState.dens[i];
}
