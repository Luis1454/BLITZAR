/*
 * Module: physics/cuda
 * Responsibility: Build a true 8-way octree fully on GPU.
 */

#include <cfloat>
#include <chrono>
#include <cstddef>
#include <thrust/device_ptr.h>
#include <thrust/execution_policy.h>
#include <thrust/iterator/constant_iterator.h>
#include <thrust/iterator/zip_iterator.h>
#include <thrust/reduce.h>
#include <thrust/scan.h>
#include <thrust/sort.h>
#include <thrust/transform_reduce.h>
#include <thrust/tuple.h>

struct ThrustPoolAllocator {
    typedef char value_type;

    char* allocate(std::ptrdiff_t numBytes)
    {
        const std::size_t bytes = static_cast<std::size_t>(std::max<std::ptrdiff_t>(0, numBytes));
        return static_cast<char*>(grav_x::CudaMemoryPool::allocate(bytes));
    }

    void deallocate(char* ptr, std::size_t)
    {
        grav_x::CudaMemoryPool::deallocate(ptr);
    }
};

struct OctreeAabb {
    float minX;
    float minY;
    float minZ;
    float maxX;
    float maxY;
    float maxZ;
};

struct OctreeAabbFromTuple {
    __host__ __device__ OctreeAabb operator()(const thrust::tuple<float, float, float>& value) const
    {
        const float x = thrust::get<0>(value);
        const float y = thrust::get<1>(value);
        const float z = thrust::get<2>(value);
        OctreeAabb out{};
        out.minX = x;
        out.minY = y;
        out.minZ = z;
        out.maxX = x;
        out.maxY = y;
        out.maxZ = z;
        return out;
    }
};

struct OctreeAabbMerge {
    __host__ __device__ OctreeAabb operator()(const OctreeAabb& lhs, const OctreeAabb& rhs) const
    {
        OctreeAabb out{};
        out.minX = fminf(lhs.minX, rhs.minX);
        out.minY = fminf(lhs.minY, rhs.minY);
        out.minZ = fminf(lhs.minZ, rhs.minZ);
        out.maxX = fmaxf(lhs.maxX, rhs.maxX);
        out.maxY = fmaxf(lhs.maxY, rhs.maxY);
        out.maxZ = fmaxf(lhs.maxZ, rhs.maxZ);
        return out;
    }
};

__global__ void buildMortonCodesKernel(ParticleSoAView state, int numParticles, float minX,
                                       float minY, float minZ, float maxX, float maxY, float maxZ,
                                       unsigned long long* mortonKeys, int* particleIndices)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }

    const Vector3 pos = getSoAPosition(state, i);
    const float invX = 1.0f / fmaxf(maxX - minX, 1.0e-12f);
    const float invY = 1.0f / fmaxf(maxY - minY, 1.0e-12f);
    const float invZ = 1.0f / fmaxf(maxZ - minZ, 1.0e-12f);

    const float nx = fminf(fmaxf((pos.x - minX) * invX, 0.0f), 1.0f);
    const float ny = fminf(fmaxf((pos.y - minY) * invY, 0.0f), 1.0f);
    const float nz = fminf(fmaxf((pos.z - minZ) * invZ, 0.0f), 1.0f);

    const unsigned int qx = min(2097151u, static_cast<unsigned int>(nx * 2097151.0f + 0.5f));
    const unsigned int qy = min(2097151u, static_cast<unsigned int>(ny * 2097151.0f + 0.5f));
    const unsigned int qz = min(2097151u, static_cast<unsigned int>(nz * 2097151.0f + 0.5f));

    mortonKeys[i] = mortonEncode63(qx, qy, qz);
    particleIndices[i] = i;
}

__global__ void buildLeafPrefixesKernel(const unsigned long long* sortedKeys, int count,
                                        int shiftBits, unsigned long long* outLeafPrefixes)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= count) {
        return;
    }
    outLeafPrefixes[i] = sortedKeys[i] >> shiftBits;
}

__global__ void buildParentPrefixesKernel(const unsigned long long* currentPrefixes, int count,
                                          unsigned long long* outParentPrefixes)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= count) {
        return;
    }
    outParentPrefixes[i] = currentPrefixes[i] >> 3;
}

__global__ void initLevelIndicesKernel(int* levelIndices, int count)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < count) {
        levelIndices[i] = i;
    }
}

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

__global__ void buildLinearOctreeNextLinksKernel(OctreeNodeHandle nodes, int nodeCount, int rootIndex)
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

__global__ void packLinearOctreeCompactKernel(
    const GpuOctreeNode* nodes,
    int nodeCount,
    GpuOctreeNodeHotData* hot,
    GpuOctreeNodeNavData* nav,
    int* firstChild,
    int* leafStarts,
    int* leafCounts)
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

bool ParticleSystem::buildLinearOctreeGpu(ParticleSoAView currentView, int numParticles)
{
    cudaStream_t stream = 0;
    const bool hardAuditMode = parseBoolEnv("GRAVITY_LINEAR_OCTREE_AUDIT", false);
    const bool profileFlashMode = parseBoolEnv("GRAVITY_OCTREE_PROFILE_FLASH", false);
    const auto buildStartTime = std::chrono::high_resolution_clock::now();
    double sortByKeyMs = 0.0;
    if (numParticles <= 0) {
        return false;
    }

    const int threads = Particle::kDefaultCudaBlockSize;
    const int blocks = (numParticles + threads - 1) / threads;

    const int leafCapacity = std::max(16, _linearOctreeLeafCapacity);

    int leafDepth = 1;
    while (leafDepth < 21) {
        const double avgParticlesPerBucket =
            static_cast<double>(numParticles) / static_cast<double>(1ull << (3 * leafDepth));
        if (avgParticlesPerBucket <= static_cast<double>(leafCapacity)) {
            break;
        }
        ++leafDepth;
    }
    const int leafShiftBits = 3 * (21 - leafDepth);

    if (g_dOctreeLeafCapacity < static_cast<std::size_t>(numParticles)
        || d_octreeMortonCapacity < static_cast<std::size_t>(numParticles)
        || d_octreePrefixCapacity < static_cast<std::size_t>(numParticles)
        || d_octreeLevelCapacity < static_cast<std::size_t>(numParticles)
        || !g_dOctreeLeafIndices
        || !d_octreeMortonKeys
        || !d_octreePrefixesA
        || !d_octreePrefixesB
        || !d_octreeLevelIndicesA
        || !d_octreeLevelIndicesB
        || !d_octreeParentCounts
        || !d_octreeParentOffsets
        || !d_octreeNodeHot
        || !d_octreeNodeNav
        || !d_octreeFirstChild
        || !d_octreeLeafStarts
        || !d_octreeLeafCounts) {
        fprintf(stderr,
                "[cuda-critical] linear octree scratch is not preallocated for %d entries\n",
                numParticles);
        return false;
    }

    ThrustPoolAllocator thrustAllocator;
    auto exec = thrust::cuda::par(thrustAllocator).on(stream);

    thrust::device_ptr<float> posX(currentView.posX);
    thrust::device_ptr<float> posY(currentView.posY);
    thrust::device_ptr<float> posZ(currentView.posZ);
    const auto zipBegin = thrust::make_zip_iterator(thrust::make_tuple(posX, posY, posZ));
    const auto zipEnd = zipBegin + numParticles;

    OctreeAabb initAabb{};
    initAabb.minX = FLT_MAX;
    initAabb.minY = FLT_MAX;
    initAabb.minZ = FLT_MAX;
    initAabb.maxX = -FLT_MAX;
    initAabb.maxY = -FLT_MAX;
    initAabb.maxZ = -FLT_MAX;
    const OctreeAabb bbox =
        thrust::transform_reduce(exec, zipBegin, zipEnd, OctreeAabbFromTuple{}, initAabb,
                                 OctreeAabbMerge{});

    buildMortonCodesKernel<<<blocks, threads, 0, stream>>>(
        currentView, numParticles, bbox.minX, bbox.minY, bbox.minZ, bbox.maxX, bbox.maxY, bbox.maxZ,
        d_octreeMortonKeys, g_dOctreeLeafIndices);
    if (!checkCudaStatus(cudaGetLastError(), "buildMortonCodes kernel launch")) {
        return false;
    }

    thrust::device_ptr<unsigned long long> sortedKeys(d_octreeMortonKeys);
    thrust::device_ptr<int> sortedIndices(g_dOctreeLeafIndices);
    thrust::device_ptr<unsigned long long> prefixesA(d_octreePrefixesA);
    thrust::device_ptr<unsigned long long> prefixesB(d_octreePrefixesB);
    thrust::device_ptr<int> levelIndicesA(d_octreeLevelIndicesA);
    thrust::device_ptr<int> levelIndicesB(d_octreeLevelIndicesB);
    thrust::device_ptr<int> parentCounts(d_octreeParentCounts);
    thrust::device_ptr<int> parentOffsets(d_octreeParentOffsets);

    if (profileFlashMode) {
        if (!checkCudaStatus(cudaStreamSynchronize(stream), "linear octree pre-sort sync")) {
            return false;
        }
    }
    const auto sortStartTime = std::chrono::high_resolution_clock::now();
    thrust::sort_by_key(exec, sortedKeys, sortedKeys + numParticles, sortedIndices);
    if (!checkCudaStatus(cudaGetLastError(), "linear octree sort_by_key")) {
        return false;
    }
    if (profileFlashMode) {
        if (!checkCudaStatus(cudaStreamSynchronize(stream), "linear octree post-sort sync")) {
            return false;
        }
        const auto sortStopTime = std::chrono::high_resolution_clock::now();
        sortByKeyMs = std::chrono::duration<double, std::milli>(sortStopTime - sortStartTime).count();
    }

    buildLeafPrefixesKernel<<<blocks, threads, 0, stream>>>(d_octreeMortonKeys, numParticles,
                                                            leafShiftBits, d_octreePrefixesA);
    if (!checkCudaStatus(cudaGetLastError(), "buildLeafPrefixes kernel launch")) {
        return false;
    }

    const thrust::pair<thrust::device_ptr<unsigned long long>, thrust::device_ptr<int>> leafEnd =
        thrust::reduce_by_key(exec, prefixesA, prefixesA + numParticles,
                              thrust::make_constant_iterator<int>(1), prefixesB, parentCounts);
    const int leafCount = static_cast<int>(leafEnd.first - prefixesB);
    if (leafCount <= 0) {
        fprintf(stderr, "[cuda-critical] linear octree produced zero leaves\n");
        return false;
    }

    thrust::exclusive_scan(exec, parentCounts, parentCounts + leafCount, parentOffsets);

    // Worst-case bound: parent count may stay close to leafCount for several levels
    // before high Morton bits collapse. Keep enough capacity for all levels.
    const int requiredNodeCapacity = std::max(2, leafCount * (leafDepth + 1) + 8);
    if (g_dOctreeNodeCapacity < static_cast<std::size_t>(requiredNodeCapacity) || !g_dOctreeNodes) {
        fprintf(stderr,
                "[cuda-critical] linear octree node scratch is too small: need=%d cap=%zu\n",
                requiredNodeCapacity,
                g_dOctreeNodeCapacity);
        return false;
    }

    const int leafBlocks = (leafCount + threads - 1) / threads;
    buildLinearOctreeLeafNodesKernel<<<leafBlocks, threads, 0, stream>>>(
        g_dOctreeNodes, g_dOctreeLeafIndices, d_octreeParentOffsets, d_octreeParentCounts,
        currentView, leafCount);
    if (!checkCudaStatus(cudaGetLastError(), "buildLinearOctreeLeafNodes kernel launch")) {
        return false;
    }

    initLevelIndicesKernel<<<leafBlocks, threads, 0, stream>>>(d_octreeLevelIndicesA, leafCount);
    if (!checkCudaStatus(cudaGetLastError(), "initLinearOctreeLevelIndices kernel launch")) {
        return false;
    }

    thrust::device_ptr<unsigned long long> currentPrefixes = prefixesB;
    thrust::device_ptr<int> currentLevelIndices = levelIndicesA;
    thrust::device_ptr<int> nextLevelIndices = levelIndicesB;
    int currentCount = leafCount;
    int nextNodeBase = leafCount;
    int totalNodeCount = leafCount;

    while (currentCount > 1) {
        const int currentBlocks = (currentCount + threads - 1) / threads;
        buildParentPrefixesKernel<<<currentBlocks, threads, 0, stream>>>(
            thrust::raw_pointer_cast(currentPrefixes), currentCount, d_octreePrefixesA);
        if (!checkCudaStatus(cudaGetLastError(), "buildParentPrefixes kernel launch")) {
            return false;
        }

        const thrust::pair<thrust::device_ptr<unsigned long long>, thrust::device_ptr<int>>
            parentEnd = thrust::reduce_by_key(exec, prefixesA, prefixesA + currentCount,
                                              thrust::make_constant_iterator<int>(1), prefixesB,
                                              parentCounts);
        const int parentCount = static_cast<int>(parentEnd.first - prefixesB);
        if (parentCount <= 0) {
            fprintf(stderr, "[cuda-critical] linear octree produced zero parents\n");
            return false;
        }
        if (nextNodeBase + parentCount > static_cast<int>(g_dOctreeNodeCapacity)) {
            fprintf(stderr,
                    "[cuda-critical] linear octree node capacity overflow: need=%d cap=%zu\n",
                    nextNodeBase + parentCount, g_dOctreeNodeCapacity);
            return false;
        }

        thrust::exclusive_scan(exec, parentCounts, parentCounts + parentCount, parentOffsets);

        const int parentBlocks = (parentCount + threads - 1) / threads;
        buildLinearOctreeParentNodesKernel8<<<parentBlocks, threads, 0, stream>>>(
            g_dOctreeNodes, thrust::raw_pointer_cast(currentLevelIndices),
            thrust::raw_pointer_cast(currentPrefixes), d_octreeParentOffsets, d_octreeParentCounts,
            parentCount, nextNodeBase, thrust::raw_pointer_cast(nextLevelIndices));
        if (!checkCudaStatus(cudaGetLastError(), "buildLinearOctreeParentNodes8 kernel launch")) {
            return false;
        }

        totalNodeCount = nextNodeBase + parentCount;
        nextNodeBase += parentCount;
        currentCount = parentCount;
        currentPrefixes = prefixesB;

        thrust::device_ptr<int> swapTmp = currentLevelIndices;
        currentLevelIndices = nextLevelIndices;
        nextLevelIndices = swapTmp;
    }

    _gpuOctreeLeafCount = numParticles;
    _gpuOctreeNodeCount = totalNodeCount;
    _gpuOctreeRootIndex = totalNodeCount - 1;

    if (_gpuOctreeRootIndex >= 0) {
        GpuOctreeNode rootNode = {};
        if (!checkCudaStatus(cudaMemcpyAsync(&rootNode,
                                             g_dOctreeNodes + _gpuOctreeRootIndex,
                                             sizeof(GpuOctreeNode),
                                             cudaMemcpyDeviceToHost,
                                             stream),
                             "copy linear octree root node to host")) {
            return false;
        }
        rootNode.parentIndex = -1;
        rootNode.nextIndex = -1;
        if (!checkCudaStatus(cudaMemcpyAsync(g_dOctreeNodes + _gpuOctreeRootIndex,
                                             &rootNode,
                                             sizeof(GpuOctreeNode),
                                             cudaMemcpyHostToDevice,
                                             stream),
                             "write linear octree root links")) {
            return false;
        }

        const int linkBlocks = (_gpuOctreeNodeCount + threads - 1) / threads;
        buildLinearOctreeNextLinksKernel<<<linkBlocks, threads, 0, stream>>>(
            g_dOctreeNodes, _gpuOctreeNodeCount, _gpuOctreeRootIndex);
        if (!checkCudaStatus(cudaGetLastError(), "buildLinearOctreeNextLinks kernel launch")) {
            return false;
        }

        const int packBlocks = (_gpuOctreeNodeCount + threads - 1) / threads;
        packLinearOctreeCompactKernel<<<packBlocks, threads, 0, stream>>>(
            g_dOctreeNodes,
            _gpuOctreeNodeCount,
            d_octreeNodeHot,
            d_octreeNodeNav,
            d_octreeFirstChild,
            d_octreeLeafStarts,
            d_octreeLeafCounts);
        if (!checkCudaStatus(cudaGetLastError(), "packLinearOctreeCompact kernel launch")) {
            return false;
        }
    }

    if (hardAuditMode) {
        fprintf(stderr,
                "[octree-audit] linear-gpu 8-way build leaf_capacity=%d leaf_depth=%d leaves=%d "
                "nodes=%d root=%d\n",
                leafCapacity, leafDepth, leafCount, _gpuOctreeNodeCount, _gpuOctreeRootIndex);
    }

    if (profileFlashMode) {
        if (!checkCudaStatus(cudaStreamSynchronize(stream), "linear octree profiling sync")) {
            return false;
        }
        const auto buildStopTime = std::chrono::high_resolution_clock::now();
        const double buildMs =
            std::chrono::duration<double, std::milli>(buildStopTime - buildStartTime).count();
        fprintf(stderr,
                "[octree-profile] buildLinearOctree_ms=%.3f sort_ms=%.3f leaf_capacity=%d\n",
                buildMs,
                sortByKeyMs,
                leafCapacity);
    }

    return _gpuOctreeRootIndex >= 0;
}
