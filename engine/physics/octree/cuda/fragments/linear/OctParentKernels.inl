/*
 * @file engine/physics/octree/cuda/fragments/linear/OctParentKernels.inl
 * @project BLITZAR
 * @brief GPU linear octree implementation fragment.
 */

__global__ void buildLinearOctreeParentNodesKernel8(OctreeNodeHandle nodes,
                                                    IndexConstHandle currentLevelIndices,
                                                    const unsigned long long* currentPrefixes,
                                                    IndexConstHandle parentOffsets,
                                                    IndexConstHandle parentCounts, int parentCount,
                                                    int parentNodeBase,
                                                    IndexHandle nextLevelIndices)
{
    const int parentId = blockIdx.x * blockDim.x + threadIdx.x;
    if (parentId >= parentCount) {
        return;
    }

    const int childStart = parentOffsets[parentId];
    const int childCount = parentCounts[parentId];

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

    GpuOctreeNode node{};
    for (int c = 0; c < 8; ++c) {
        node.children[c] = -1;
    }
    node.parentIndex = -1;
    node.nextIndex = -1;
    node.childMask = 0u;

    const int nodeIndex = parentNodeBase + parentId;

    for (int localChild = 0; localChild < childCount; ++localChild) {
        const int childSlot = childStart + localChild;
        const int childIndex = currentLevelIndices[childSlot];
        const unsigned long long childPrefix = currentPrefixes[childSlot];
        int octant = static_cast<int>(childPrefix & 0x7ULL);

        if (octant < 0 || octant > 7 || node.children[octant] >= 0) {
            octant = 0;
            while (octant < 8 && node.children[octant] >= 0) {
                ++octant;
            }
            if (octant >= 8) {
                continue;
            }
        }

        node.children[octant] = childIndex;
        node.childMask |= static_cast<unsigned char>(1u << octant);

        GpuOctreeNode childNode = nodes[childIndex];
        childNode.parentIndex = nodeIndex;
        nodes[childIndex] = childNode;

        const GpuOctreeNode child = nodes[childIndex];
        minX = fminf(minX, child.centerX - child.halfSize);
        minY = fminf(minY, child.centerY - child.halfSize);
        minZ = fminf(minZ, child.centerZ - child.halfSize);
        maxX = fmaxf(maxX, child.centerX + child.halfSize);
        maxY = fmaxf(maxY, child.centerY + child.halfSize);
        maxZ = fmaxf(maxZ, child.centerZ + child.halfSize);

        totalMass += static_cast<double>(child.mass);
        weightedX += static_cast<double>(child.comX) * static_cast<double>(child.mass);
        weightedY += static_cast<double>(child.comY) * static_cast<double>(child.mass);
        weightedZ += static_cast<double>(child.comZ) * static_cast<double>(child.mass);
    }

    if (node.childMask == 0u) {
        minX = 0.0f;
        minY = 0.0f;
        minZ = 0.0f;
        maxX = 0.0f;
        maxY = 0.0f;
        maxZ = 0.0f;
    }

    node.centerX = 0.5f * (minX + maxX);
    node.centerY = 0.5f * (minY + maxY);
    node.centerZ = 0.5f * (minZ + maxZ);
    node.halfSize = 0.5f * fmaxf(fmaxf(maxX - minX, maxY - minY), maxZ - minZ) + 1.0e-6f;
    node.mass = static_cast<float>(totalMass);
    node.comX = totalMass > 0.0 ? static_cast<float>(weightedX / totalMass) : node.centerX;
    node.comY = totalMass > 0.0 ? static_cast<float>(weightedY / totalMass) : node.centerY;
    node.comZ = totalMass > 0.0 ? static_cast<float>(weightedZ / totalMass) : node.centerZ;
    node.leafStart = 0;
    node.leafCount = 0;

    nodes[nodeIndex] = node;
    nextLevelIndices[parentId] = nodeIndex;
}

/*
 * @brief Documents the build linear octree next links kernel operation contract.
 * @param nodes Input value used by this contract.
 * @param nodeCount Input value used by this contract.
 * @param rootIndex Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
