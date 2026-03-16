__device__ bool octreeNodeContains(const GpuOctreeNode &node, const Vector3 &pos)
{
    return fabsf(pos.x - node.centerX) <= node.halfSize
        && fabsf(pos.y - node.centerY) <= node.halfSize
        && fabsf(pos.z - node.centerZ) <= node.halfSize;
}

__global__ void updateParticlesOctree(
    ParticleSoAView lastState,
    ParticleSoAView particlesOut,
    int numParticles,
    OctreeNodeConstHandle nodes,
    int rootIndex,
    IndexConstHandle leafIndices,
    float theta,
    float softening,
    float deltaTime,
    float maxAcceleration,
    float minSoftening,
    float minDistance2,
    float minTheta
) {
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles || rootIndex < 0) {
        return;
    }

    const Vector3 selfPos = lastState.getPosition(i);
    const float clampedTheta = clampThetaValue(theta, minTheta);
    const float clampedSoftening = clampSofteningValue(softening, minSoftening);
    Vector3 force(0.0f, 0.0f, 0.0f);

    constexpr int kStackCapacity = 64;
    int stack[kStackCapacity];
    int top = 0;
    stack[top++] = rootIndex;

    while (top > 0) {
        const GpuOctreeNode node = nodes[stack[--top]];
        if (node.mass <= 0.0f) continue;

        if (node.childMask == 0) {
            for (int k = 0; k < node.leafCount; ++k) {
                const int otherIndex = leafIndices[node.leafStart + k];
                if (otherIndex == i) continue;
                force += gravityAccelerationFromSource(
                    selfPos,
                    lastState.getPosition(otherIndex),
                    lastState.mass[otherIndex],
                    clampedSoftening,
                    minSoftening,
                    minDistance2);
            }
            continue;
        }

        const Vector3 direction(node.comX - selfPos.x, node.comY - selfPos.y, node.comZ - selfPos.z);
        const float dist2 = dot(direction, direction) + clampedSoftening * clampedSoftening;
        const float dist = sqrtf(dist2);
        const bool containsSelf = octreeNodeContains(node, selfPos);
        const float size = node.halfSize * 2.0f;

        if (!containsSelf && (size / dist) < clampedTheta) {
            force += gravityAccelerationFromSource(
                selfPos, Vector3(node.comX, node.comY, node.comZ),
                node.mass, clampedSoftening, minSoftening, minDistance2);
            continue;
        }

        for (int child = 0; child < 8; ++child) {
            if ((node.childMask & (1u << child)) == 0) continue;
            const int childIndex = node.children[child];
            if (childIndex >= 0 && top < kStackCapacity) stack[top++] = childIndex;
        }
    }

    force = clampAcceleration(force, maxAcceleration);
    const Vector3 vel = lastState.getVelocity(i);
    const Vector3 nextVel = vel + force * deltaTime;
    const Vector3 nextPos = selfPos + nextVel * deltaTime;

    particlesOut.setPressure(i, force * 100.0f);
    particlesOut.setVelocity(i, nextVel);
    particlesOut.setPosition(i, nextPos);
    particlesOut.mass[i] = lastState.mass[i];
    particlesOut.temp[i] = lastState.temp[i];
    particlesOut.dens[i] = lastState.dens[i];
}
