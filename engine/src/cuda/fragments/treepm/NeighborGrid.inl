/*
 * @file engine/src/cuda/fragments/treepm/NeighborGrid.inl
 * @project BLITZAR
 * @brief TreePM sorted neighbor-grid construction.
 */

bool ParticleSystem::ensureTreePmNeighborGridCapacity(int numParticles, int totalCells,
                                                       bool gatherParticles)
{
    if (!_device._cudaRuntimeAvailable || numParticles <= 0 || totalCells <= 0) {
        return false;
    }
    const std::size_t particleCapacity = static_cast<std::size_t>(numParticles);
    const bool missingParticleBuffers =
        _device.d_treePmNeighborParticleCapacity < particleCapacity || !_device.d_sphCellHash ||
        !_device.d_sphSortedIndex || !_device.d_treePmSortKeys ||
        !_device.d_treePmSortIndices || !_device.d_treePmSortedCellHash;
    if (missingParticleBuffers) {
        if (_device.d_sphCellHash) {
            bltzr_x::MemoryPool::deallocate(_device.d_sphCellHash);
            _device.d_sphCellHash = nullptr;
        }
        if (_device.d_sphSortedIndex) {
            bltzr_x::MemoryPool::deallocate(_device.d_sphSortedIndex);
            _device.d_sphSortedIndex = nullptr;
        }
        if (_device.d_treePmSortKeys) {
            bltzr_x::MemoryPool::deallocate(_device.d_treePmSortKeys);
            _device.d_treePmSortKeys = nullptr;
        }
        if (_device.d_treePmSortIndices) {
            bltzr_x::MemoryPool::deallocate(_device.d_treePmSortIndices);
            _device.d_treePmSortIndices = nullptr;
        }
        if (_device.d_treePmSortedCellHash) {
            bltzr_x::MemoryPool::deallocate(_device.d_treePmSortedCellHash);
            _device.d_treePmSortedCellHash = nullptr;
        }
        if (_device.d_treePmSortTempStorage) {
            bltzr_x::MemoryPool::deallocate(_device.d_treePmSortTempStorage);
            _device.d_treePmSortTempStorage = nullptr;
            _device.d_treePmSortTempCapacity = 0u;
        }
        const std::size_t bytes = particleCapacity * sizeof(int);
        _device.d_sphCellHash = static_cast<int*>(bltzr_x::MemoryPool::allocate(bytes));
        _device.d_sphSortedIndex = static_cast<int*>(bltzr_x::MemoryPool::allocate(bytes));
        _device.d_treePmSortKeys = static_cast<int*>(bltzr_x::MemoryPool::allocate(bytes));
        _device.d_treePmSortIndices = static_cast<int*>(bltzr_x::MemoryPool::allocate(bytes));
        _device.d_treePmSortedCellHash = static_cast<int*>(bltzr_x::MemoryPool::allocate(bytes));
        if (!_device.d_sphCellHash || !_device.d_sphSortedIndex || !_device.d_treePmSortKeys ||
            !_device.d_treePmSortIndices || !_device.d_treePmSortedCellHash) {
            releaseSphGridBuffers();
            return false;
        }
        _device.d_treePmNeighborParticleCapacity = particleCapacity;
    }

    if (_device.d_treePmSortTempStorage == nullptr) {
        cub::DoubleBuffer<int> keys(_device.d_sphCellHash, _device.d_treePmSortKeys);
        cub::DoubleBuffer<int> values(_device.d_sphSortedIndex, _device.d_treePmSortIndices);
        std::size_t tempBytes = 0u;
        if (!checkCudaStatus(
                cub::DeviceRadixSort::SortPairs(nullptr, tempBytes, keys, values, numParticles, 0,
                                                21),
                "treepm CUB radix sort temp query")) {
            return false;
        }
        _device.d_treePmSortTempStorage = bltzr_x::MemoryPool::allocate(tempBytes);
        if (_device.d_treePmSortTempStorage == nullptr) {
            return false;
        }
        _device.d_treePmSortTempCapacity = tempBytes;
    }

    if (gatherParticles &&
        (_device.d_treePmSortedParticleCapacity < particleCapacity ||
         !_device.d_treePmSortedPosX || !_device.d_treePmSortedPosY ||
         !_device.d_treePmSortedPosZ || !_device.d_treePmSortedMass)) {
        bltzr_x::MemoryPool::deallocate(_device.d_treePmSortedPosX);
        bltzr_x::MemoryPool::deallocate(_device.d_treePmSortedPosY);
        bltzr_x::MemoryPool::deallocate(_device.d_treePmSortedPosZ);
        bltzr_x::MemoryPool::deallocate(_device.d_treePmSortedMass);
        const std::size_t bytes = particleCapacity * sizeof(float);
        _device.d_treePmSortedPosX = static_cast<float*>(bltzr_x::MemoryPool::allocate(bytes));
        _device.d_treePmSortedPosY = static_cast<float*>(bltzr_x::MemoryPool::allocate(bytes));
        _device.d_treePmSortedPosZ = static_cast<float*>(bltzr_x::MemoryPool::allocate(bytes));
        _device.d_treePmSortedMass = static_cast<float*>(bltzr_x::MemoryPool::allocate(bytes));
        if (!_device.d_treePmSortedPosX || !_device.d_treePmSortedPosY ||
            !_device.d_treePmSortedPosZ || !_device.d_treePmSortedMass) {
            releaseSphGridBuffers();
            return false;
        }
        _device.d_treePmSortedParticleCapacity = particleCapacity;
    }

    const std::size_t cellCapacity = static_cast<std::size_t>(totalCells);
    if (_device.d_treePmNeighborCellCapacity < cellCapacity || !_device.d_sphCellStart ||
        !_device.d_sphCellEnd) {
        if (_device.d_sphCellStart) {
            bltzr_x::MemoryPool::deallocate(_device.d_sphCellStart);
            _device.d_sphCellStart = nullptr;
        }
        if (_device.d_sphCellEnd) {
            bltzr_x::MemoryPool::deallocate(_device.d_sphCellEnd);
            _device.d_sphCellEnd = nullptr;
        }
        const std::size_t bytes = cellCapacity * sizeof(int);
        _device.d_sphCellStart = static_cast<int*>(bltzr_x::MemoryPool::allocate(bytes));
        _device.d_sphCellEnd = static_cast<int*>(bltzr_x::MemoryPool::allocate(bytes));
        if (!_device.d_sphCellStart || !_device.d_sphCellEnd) {
            releaseSphGridBuffers();
            return false;
        }
        _device.d_treePmNeighborCellCapacity = cellCapacity;
    }
    return true;
}
bool ParticleSystem::buildTreePmNeighborGrid(ParticleSoAView currentView, int numParticles,
                                             const TreePmGridParams& grid)
{
    const bool gatherParticles = treePmGatherEnabled();
    if (!ensureTreePmNeighborGridCapacity(numParticles, grid.totalCells, gatherParticles)) {
        return false;
    }

    const int numBlocks =
        (numParticles + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    const bool useMorton = treePmMortonEnabled();
    treepm::treePmBuildCellHashKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
        currentView, numParticles, grid, _device.d_sphCellHash, _device.d_sphSortedIndex,
        useMorton ? 1 : 0);
    if (!checkCudaStatus(cudaGetLastError(), "treePmBuildCellHashKernel launch")) {
        return false;
    }

    cub::DoubleBuffer<int> keys(_device.d_sphCellHash, _device.d_treePmSortKeys);
    cub::DoubleBuffer<int> values(_device.d_sphSortedIndex, _device.d_treePmSortIndices);
    const int endBit = 21;
    if (!checkCudaStatus(
            cub::DeviceRadixSort::SortPairs(_device.d_treePmSortTempStorage,
                                            _device.d_treePmSortTempCapacity, keys, values,
                                            numParticles, 0, endBit),
            "treepm CUB radix sort")) {
        return false;
    }
    if (keys.Current() != _device.d_sphCellHash.get()) {
        _device.d_sphCellHash.swap(_device.d_treePmSortKeys);
    }
    if (values.Current() != _device.d_sphSortedIndex.get()) {
        _device.d_sphSortedIndex.swap(_device.d_treePmSortIndices);
    }

    IndexConstHandle sortedHash = _device.d_sphCellHash;
    if (useMorton) {
        treepm::treePmBuildSortedCellHashKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView, _device.d_sphSortedIndex, _device.d_treePmSortedCellHash, numParticles,
            grid);
        if (!checkCudaStatus(cudaGetLastError(), "treePmBuildSortedCellHashKernel launch")) {
            return false;
        }
        sortedHash = _device.d_treePmSortedCellHash;
    }

    if (gatherParticles) {
        treepm::treePmGatherSortedParticlesKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView, _device.d_sphSortedIndex, _device.d_treePmSortedPosX,
            _device.d_treePmSortedPosY, _device.d_treePmSortedPosZ, _device.d_treePmSortedMass,
            numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "treePmGatherSortedParticlesKernel launch")) {
            return false;
        }
    }

    const int cellBlocks =
        (grid.totalCells + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    treepm::treePmResetCellBoundsKernel<<<cellBlocks, Particle::kDefaultCudaBlockSize>>>(
        _device.d_sphCellStart, _device.d_sphCellEnd, grid.totalCells);
    if (!checkCudaStatus(cudaGetLastError(), "treePmResetCellBoundsKernel launch")) {
        return false;
    }
    treepm::treePmFindCellBoundsKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
        sortedHash, _device.d_sphCellStart, _device.d_sphCellEnd, numParticles);
    if (!checkCudaStatus(cudaGetLastError(), "treePmFindCellBoundsKernel launch")) {
        return false;
    }
    return checkCudaStatus(cudaDeviceSynchronize(), "treepm neighbor grid sync");
}
