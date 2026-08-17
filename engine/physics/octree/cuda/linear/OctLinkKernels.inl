/*
 * @file engine/physics/octree/cuda/linear/OctLinkKernels.inl
 * @project BLITZAR
 * @brief GPU linear octree implementation fragment.
 */

__global__ void buildLinearOctreeNextLinksKernel(OctreeNodeHandle nodes, int nodeCount,
                                                 int rootIndex)
{
    const int nodeIndex = blockIdx.x * blockDim.x + threadIdx.x;
    if (nodeIndex >= nodeCount || nodeIndex < 0) {
        return;
    }

    if (nodeIndex == rootIndex) {
        nodes[nodeIndex].nextIndex = -1;
        return;
    }

    int nextIndex = -1;
    int current = nodeIndex;
    int parent = nodes[current].parentIndex;

    while (parent >= 0) {
        const GpuOctreeNode parentNode = nodes[parent];

        int childSlot = -1;
        for (int c = 0; c < 8; ++c) {
            if (parentNode.children[c] == current) {
                childSlot = c;
                break;
            }
        }

        if (childSlot >= 0) {
            for (int c = childSlot + 1; c < 8; ++c) {
                if (parentNode.children[c] >= 0) {
                    nextIndex = parentNode.children[c];
                    break;
                }
            }
        }

        if (nextIndex >= 0) {
            break;
        }

        current = parent;
        parent = nodes[current].parentIndex;
    }

    nodes[nodeIndex].nextIndex = nextIndex;
}

__global__ void setLinearOctreeRootLinksKernel(OctreeNodeHandle nodes, int rootIndex)
{
    if (blockIdx.x != 0 || threadIdx.x != 0 || rootIndex < 0) {
        return;
    }
    nodes[rootIndex].parentIndex = -1;
    nodes[rootIndex].nextIndex = -1;
}

/*
 * @brief Documents the pack linear octree compact kernel operation contract.
 * @param nodes Input value used by this contract.
 * @param nodeCount Input value used by this contract.
 * @param hot Input value used by this contract.
 * @param nav Input value used by this contract.
 * @param firstChild Input value used by this contract.
 * @param leafStarts Input value used by this contract.
 * @param leafCounts Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void packLinearOctreeCompactKernel(const GpuOctreeNode* nodes, int nodeCount,
                                              GpuOctreeNodeHotData* hot, GpuOctreeNodeNavData* nav,
                                              int* firstChild, int* leafStarts, int* leafCounts)
{
    const int nodeIndex = blockIdx.x * blockDim.x + threadIdx.x;
    if (nodeIndex < 0 || nodeIndex >= nodeCount) {
        return;
    }

    const GpuOctreeNode node = nodes[nodeIndex];

    GpuOctreeNodeHotData h = {};
    h.centerX = node.centerX;
    h.centerY = node.centerY;
    h.centerZ = node.centerZ;
    h.halfSize = node.halfSize;
    h.mass = node.mass;
    h.comX = node.comX;
    h.comY = node.comY;
    h.comZ = node.comZ;
    hot[nodeIndex] = h;

    GpuOctreeNodeNavData n = {};
    n.nextIndex = node.nextIndex;
    n.childMask = node.childMask;
    nav[nodeIndex] = n;

    int fc = -1;
    for (int c = 0; c < 8; ++c) {
        if (node.children[c] >= 0) {
            fc = node.children[c];
            break;
        }
    }
    firstChild[nodeIndex] = fc;
    leafStarts[nodeIndex] = node.leafStart;
    leafCounts[nodeIndex] = node.leafCount;
}
