/*
 * @file engine/src/cuda/fragments/octree/linear/Build.inl
 * @project BLITZAR
 * @brief GPU linear octree implementation fragment.
 */

/*
 * @brief Documents the build linear octree gpu operation contract.
 * @param currentView Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @return bool ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool ParticleSystem::buildLinearOctreeGpu(ParticleSoAView currentView, int numParticles)
{
    cudaStream_t stream = 0;
    const bool hardAuditMode = parseBoolEnv("BLITZAR_LINEAR_OCTREE_AUDIT", false);
    const bool profileFlashMode = parseBoolEnv("BLITZAR_OCTREE_PROFILE_FLASH", false);
    const auto buildStartTime = std::chrono::high_resolution_clock::now();
    double sortByKeyMs = 0.0;
    if (numParticles <= 0) {
        return false;
    }

    const int threads = Particle::kDefaultCudaBlockSize;
    const int blocks = (numParticles + threads - 1) / threads;

    const int leafCapacity = std::max(16, _device._linearOctreeLeafCapacity);

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

    if (_device.g_dOctreeLeafCapacity < static_cast<std::size_t>(numParticles) ||
        _device.d_octreeMortonCapacity < static_cast<std::size_t>(numParticles) ||
        _device.d_octreePrefixCapacity < static_cast<std::size_t>(numParticles) ||
        _device.d_octreeLevelCapacity < static_cast<std::size_t>(numParticles) || !_device.g_dOctreeLeafIndices ||
        !_device.d_octreeMortonKeys || !_device.d_octreePrefixesA || !_device.d_octreePrefixesB || !_device.d_octreeLevelIndicesA ||
        !_device.d_octreeLevelIndicesB || !_device.d_octreeParentCounts || !_device.d_octreeParentOffsets ||
        !_device.d_octreeNodeHot || !_device.d_octreeNodeNav || !_device.d_octreeFirstChild || !_device.d_octreeLeafStarts ||
        !_device.d_octreeLeafCounts) {
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
    const OctreeAabb bbox = thrust::transform_reduce(exec, zipBegin, zipEnd, OctreeAabbFromTuple{},
                                                     initAabb, OctreeAabbMerge{});

    buildMortonCodesKernel<<<blocks, threads, 0, stream>>>(
        currentView, numParticles, bbox.minX, bbox.minY, bbox.minZ, bbox.maxX, bbox.maxY, bbox.maxZ,
        _device.d_octreeMortonKeys, _device.g_dOctreeLeafIndices);
    if (!checkCudaStatus(cudaGetLastError(), "buildMortonCodes kernel launch")) {
        return false;
    }

    thrust::device_ptr<unsigned long long> sortedKeys(_device.d_octreeMortonKeys.get());
    thrust::device_ptr<int> sortedIndices(_device.g_dOctreeLeafIndices.get());
    thrust::device_ptr<unsigned long long> prefixesA(_device.d_octreePrefixesA.get());
    thrust::device_ptr<unsigned long long> prefixesB(_device.d_octreePrefixesB.get());
    thrust::device_ptr<int> levelIndicesA(_device.d_octreeLevelIndicesA.get());
    thrust::device_ptr<int> levelIndicesB(_device.d_octreeLevelIndicesB.get());
    thrust::device_ptr<int> parentCounts(_device.d_octreeParentCounts.get());
    thrust::device_ptr<int> parentOffsets(_device.d_octreeParentOffsets.get());

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
        sortByKeyMs =
            std::chrono::duration<double, std::milli>(sortStopTime - sortStartTime).count();
    }

    buildLeafPrefixesKernel<<<blocks, threads, 0, stream>>>(_device.d_octreeMortonKeys, numParticles,
                                                            leafShiftBits, _device.d_octreePrefixesA);
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
    if (_device.g_dOctreeNodeCapacity < static_cast<std::size_t>(requiredNodeCapacity) || !_device.g_dOctreeNodes) {
        fprintf(stderr,
                "[cuda-critical] linear octree node scratch is too small: need=%d cap=%zu\n",
                requiredNodeCapacity, _device.g_dOctreeNodeCapacity);
        return false;
    }

    const int leafBlocks = (leafCount + threads - 1) / threads;
    buildLinearOctreeLeafNodesKernel<<<leafBlocks, threads, 0, stream>>>(
        _device.g_dOctreeNodes, _device.g_dOctreeLeafIndices, _device.d_octreeParentOffsets, _device.d_octreeParentCounts,
        currentView, leafCount);
    if (!checkCudaStatus(cudaGetLastError(), "buildLinearOctreeLeafNodes kernel launch")) {
        return false;
    }

    initLevelIndicesKernel<<<leafBlocks, threads, 0, stream>>>(_device.d_octreeLevelIndicesA, leafCount);
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
            thrust::raw_pointer_cast(currentPrefixes), currentCount, _device.d_octreePrefixesA);
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
        if (nextNodeBase + parentCount > static_cast<int>(_device.g_dOctreeNodeCapacity)) {
            fprintf(stderr,
                    "[cuda-critical] linear octree node capacity overflow: need=%d cap=%zu\n",
                    nextNodeBase + parentCount, _device.g_dOctreeNodeCapacity);
            return false;
        }

        thrust::exclusive_scan(exec, parentCounts, parentCounts + parentCount, parentOffsets);

        const int parentBlocks = (parentCount + threads - 1) / threads;
        buildLinearOctreeParentNodesKernel8<<<parentBlocks, threads, 0, stream>>>(
            _device.g_dOctreeNodes, thrust::raw_pointer_cast(currentLevelIndices),
            thrust::raw_pointer_cast(currentPrefixes), _device.d_octreeParentOffsets, _device.d_octreeParentCounts,
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

    _device._gpuOctreeLeafCount = numParticles;
    _device._gpuOctreeNodeCount = totalNodeCount;
    _device._gpuOctreeRootIndex = totalNodeCount - 1;

    if (_device._gpuOctreeRootIndex >= 0) {
        setLinearOctreeRootLinksKernel<<<1, 1, 0, stream>>>(_device.g_dOctreeNodes,
                                                            _device._gpuOctreeRootIndex);
        if (!checkCudaStatus(cudaGetLastError(), "set linear octree root links launch")) {
            return false;
        }

        const int linkBlocks = (_device._gpuOctreeNodeCount + threads - 1) / threads;
        buildLinearOctreeNextLinksKernel<<<linkBlocks, threads, 0, stream>>>(
            _device.g_dOctreeNodes, _device._gpuOctreeNodeCount, _device._gpuOctreeRootIndex);
        if (!checkCudaStatus(cudaGetLastError(), "buildLinearOctreeNextLinks kernel launch")) {
            return false;
        }

        const int packBlocks = (_device._gpuOctreeNodeCount + threads - 1) / threads;
        packLinearOctreeCompactKernel<<<packBlocks, threads, 0, stream>>>(
            _device.g_dOctreeNodes, _device._gpuOctreeNodeCount, _device.d_octreeNodeHot, _device.d_octreeNodeNav,
            _device.d_octreeFirstChild, _device.d_octreeLeafStarts, _device.d_octreeLeafCounts);
        if (!checkCudaStatus(cudaGetLastError(), "packLinearOctreeCompact kernel launch")) {
            return false;
        }
    }

    if (hardAuditMode) {
        fprintf(stderr,
                "[octree-audit] linear-gpu 8-way build leaf_capacity=%d leaf_depth=%d leaves=%d "
                "nodes=%d root=%d\n",
                leafCapacity, leafDepth, leafCount, _device._gpuOctreeNodeCount, _device._gpuOctreeRootIndex);
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
                buildMs, sortByKeyMs, leafCapacity);
    }

    return _device._gpuOctreeRootIndex >= 0;
}
