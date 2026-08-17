/*
 * @file engine/physics/octree/cuda/fragments/linear/OctLeafKernels.inl
 * @project BLITZAR
 * @brief GPU linear octree implementation fragment.
 */

__global__ void buildLinearOctreeLeafNodesKernel(OctreeNodeHandle nodes,
                                                 IndexConstHandle sortedParticleIndices,
                                                 IndexConstHandle leafStarts,
                                                 IndexConstHandle leafCounts, ParticleSoAView state,
                                                 int leafCount)
{
    const int leafId = blockIdx.x * blockDim.x + threadIdx.x;
    if (leafId >= leafCount) {
        return;
    }

    const int start = leafStarts[leafId];
    const int count = leafCounts[leafId];
    const int end = start + count;

    float minX = FLT_MAX;
    float minY = FLT_MAX;
    float minZ = FLT_MAX;
    float maxX = -FLT_MAX;
    float maxY = -FLT_MAX;
    float maxZ = -FLT_MAX;
    double totalMass = 0.0;
    double weightedX = 0.0;
    double weightedY = 0.0;
    double weightedZ = 0.0;

    for (int j = start; j < end; ++j) {
        const int particleIndex = sortedParticleIndices[j];
        const Vector3 pos = getSoAPosition(state, particleIndex);
        const float mass = state.mass[particleIndex];
        minX = fminf(minX, pos.x);
        minY = fminf(minY, pos.y);
        minZ = fminf(minZ, pos.z);
        maxX = fmaxf(maxX, pos.x);
        maxY = fmaxf(maxY, pos.y);
        maxZ = fmaxf(maxZ, pos.z);
        totalMass += static_cast<double>(mass);
        weightedX += static_cast<double>(pos.x) * static_cast<double>(mass);
        weightedY += static_cast<double>(pos.y) * static_cast<double>(mass);
        weightedZ += static_cast<double>(pos.z) * static_cast<double>(mass);
    }

    GpuOctreeNode node{};
    for (int c = 0; c < 8; ++c) {
        node.children[c] = -1;
    }

    const float centerX = 0.5f * (minX + maxX);
    const float centerY = 0.5f * (minY + maxY);
    const float centerZ = 0.5f * (minZ + maxZ);
    const float half = 0.5f * fmaxf(fmaxf(maxX - minX, maxY - minY), maxZ - minZ) + 1.0e-6f;

    node.centerX = centerX;
    node.centerY = centerY;
    node.centerZ = centerZ;
    node.halfSize = half;
    node.mass = static_cast<float>(totalMass);
    if (totalMass > 0.0) {
        node.comX = static_cast<float>(weightedX / totalMass);
        node.comY = static_cast<float>(weightedY / totalMass);
        node.comZ = static_cast<float>(weightedZ / totalMass);
    }
    else {
        node.comX = centerX;
        node.comY = centerY;
        node.comZ = centerZ;
    }
    node.leafStart = start;
    node.leafCount = count;
    node.parentIndex = -1;
    node.nextIndex = -1;
    node.childMask = 0u;

    nodes[leafId] = node;
}

/*
 * @brief Documents the build linear octree parent nodes kernel8 operation contract.
 * @param nodes Input value used by this contract.
 * @param currentLevelIndices Input value used by this contract.
 * @param currentPrefixes Input value used by this contract.
 * @param parentOffsets Input value used by this contract.
 * @param parentCounts Input value used by this contract.
 * @param parentCount Input value used by this contract.
 * @param parentNodeBase Input value used by this contract.
 * @param nextLevelIndices Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
