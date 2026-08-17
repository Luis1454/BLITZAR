/*
 * @file engine/physics/cuda/fragments/system/buffer/CudOctreeScratch.inl
 * @project BLITZAR
 * @brief Particle-system buffer lifecycle implementation fragment.
 */

bool ParticleSystem::ensureLinearOctreeScratchCapacity(int numParticles)
{
    if (!_device->_cudaRuntimeAvailable) {
        return false;
    }
    if (numParticles <= 0) {
        return false;
    }

    const int leafCapacity = std::max(16, _device->_linearOctreeLeafCapacity);

    int leafDepth = 1;
    while (leafDepth < 21) {
        const double avgParticlesPerBucket =
            static_cast<double>(numParticles) / static_cast<double>(1ull << (3 * leafDepth));
        if (avgParticlesPerBucket <= static_cast<double>(leafCapacity)) {
            break;
        }
        ++leafDepth;
    }

    if (_device->g_dOctreeLeafCapacity < static_cast<std::size_t>(numParticles)) {
        if (_device->g_dOctreeLeafIndices) {
            bltzr_x::MemoryPool::deallocate(_device->g_dOctreeLeafIndices);
            _device->g_dOctreeLeafIndices = nullptr;
        }
        _device->g_dOctreeLeafIndices = static_cast<int*>(
            bltzr_x::MemoryPool::allocate(static_cast<std::size_t>(numParticles) * sizeof(int)));
        if (!_device->g_dOctreeLeafIndices) {
            _device->g_dOctreeLeafCapacity = 0;
            return false;
        }
        _device->g_dOctreeLeafCapacity = static_cast<std::size_t>(numParticles);
    }

    if (_device->d_octreeMortonCapacity < static_cast<std::size_t>(numParticles)) {
        if (_device->d_octreeMortonKeys) {
            bltzr_x::MemoryPool::deallocate(_device->d_octreeMortonKeys);
            _device->d_octreeMortonKeys = nullptr;
        }
        _device->d_octreeMortonKeys = static_cast<unsigned long long*>(bltzr_x::MemoryPool::allocate(
            static_cast<std::size_t>(numParticles) * sizeof(unsigned long long)));
        if (!_device->d_octreeMortonKeys) {
            _device->d_octreeMortonCapacity = 0;
            return false;
        }
        _device->d_octreeMortonCapacity = static_cast<std::size_t>(numParticles);
    }

    if (_device->d_octreePrefixCapacity < static_cast<std::size_t>(numParticles)) {
        if (_device->d_octreePrefixesA) {
            bltzr_x::MemoryPool::deallocate(_device->d_octreePrefixesA);
            _device->d_octreePrefixesA = nullptr;
        }
        if (_device->d_octreePrefixesB) {
            bltzr_x::MemoryPool::deallocate(_device->d_octreePrefixesB);
            _device->d_octreePrefixesB = nullptr;
        }
        _device->d_octreePrefixesA = static_cast<unsigned long long*>(bltzr_x::MemoryPool::allocate(
            static_cast<std::size_t>(numParticles) * sizeof(unsigned long long)));
        _device->d_octreePrefixesB = static_cast<unsigned long long*>(bltzr_x::MemoryPool::allocate(
            static_cast<std::size_t>(numParticles) * sizeof(unsigned long long)));
        if (!_device->d_octreePrefixesA || !_device->d_octreePrefixesB) {
            if (_device->d_octreePrefixesA) {
                bltzr_x::MemoryPool::deallocate(_device->d_octreePrefixesA);
                _device->d_octreePrefixesA = nullptr;
            }
            if (_device->d_octreePrefixesB) {
                bltzr_x::MemoryPool::deallocate(_device->d_octreePrefixesB);
                _device->d_octreePrefixesB = nullptr;
            }
            _device->d_octreePrefixCapacity = 0;
            return false;
        }
        _device->d_octreePrefixCapacity = static_cast<std::size_t>(numParticles);
    }

    if (_device->d_octreeLevelCapacity < static_cast<std::size_t>(numParticles)) {
        if (_device->d_octreeLevelIndicesA) {
            bltzr_x::MemoryPool::deallocate(_device->d_octreeLevelIndicesA);
            _device->d_octreeLevelIndicesA = nullptr;
        }
        if (_device->d_octreeLevelIndicesB) {
            bltzr_x::MemoryPool::deallocate(_device->d_octreeLevelIndicesB);
            _device->d_octreeLevelIndicesB = nullptr;
        }
        if (_device->d_octreeParentCounts) {
            bltzr_x::MemoryPool::deallocate(_device->d_octreeParentCounts);
            _device->d_octreeParentCounts = nullptr;
        }
        if (_device->d_octreeParentOffsets) {
            bltzr_x::MemoryPool::deallocate(_device->d_octreeParentOffsets);
            _device->d_octreeParentOffsets = nullptr;
        }

        _device->d_octreeLevelIndicesA = static_cast<int*>(
            bltzr_x::MemoryPool::allocate(static_cast<std::size_t>(numParticles) * sizeof(int)));
        _device->d_octreeLevelIndicesB = static_cast<int*>(
            bltzr_x::MemoryPool::allocate(static_cast<std::size_t>(numParticles) * sizeof(int)));
        _device->d_octreeParentCounts = static_cast<int*>(
            bltzr_x::MemoryPool::allocate(static_cast<std::size_t>(numParticles) * sizeof(int)));
        _device->d_octreeParentOffsets = static_cast<int*>(
            bltzr_x::MemoryPool::allocate(static_cast<std::size_t>(numParticles) * sizeof(int)));
        if (!_device->d_octreeLevelIndicesA || !_device->d_octreeLevelIndicesB ||
            !_device->d_octreeParentCounts || !_device->d_octreeParentOffsets) {
            if (_device->d_octreeLevelIndicesA) {
                bltzr_x::MemoryPool::deallocate(_device->d_octreeLevelIndicesA);
                _device->d_octreeLevelIndicesA = nullptr;
            }
            if (_device->d_octreeLevelIndicesB) {
                bltzr_x::MemoryPool::deallocate(_device->d_octreeLevelIndicesB);
                _device->d_octreeLevelIndicesB = nullptr;
            }
            if (_device->d_octreeParentCounts) {
                bltzr_x::MemoryPool::deallocate(_device->d_octreeParentCounts);
                _device->d_octreeParentCounts = nullptr;
            }
            if (_device->d_octreeParentOffsets) {
                bltzr_x::MemoryPool::deallocate(_device->d_octreeParentOffsets);
                _device->d_octreeParentOffsets = nullptr;
            }
            _device->d_octreeLevelCapacity = 0;
            return false;
        }
        _device->d_octreeLevelCapacity = static_cast<std::size_t>(numParticles);
    }

    const int expectedLeaves = std::max(1, (numParticles + leafCapacity - 1) / leafCapacity);
    const int requiredNodeCapacity = std::max(2, expectedLeaves * (leafDepth + 1) * 4 + 8);
    if (_device->g_dOctreeNodeCapacity < static_cast<std::size_t>(requiredNodeCapacity)) {
        if (_device->g_dOctreeNodes) {
            bltzr_x::MemoryPool::deallocate(_device->g_dOctreeNodes);
            _device->g_dOctreeNodes = nullptr;
        }
        if (_device->d_octreeNodeHot) {
            bltzr_x::MemoryPool::deallocate(_device->d_octreeNodeHot);
            _device->d_octreeNodeHot = nullptr;
        }
        if (_device->d_octreeNodeNav) {
            bltzr_x::MemoryPool::deallocate(_device->d_octreeNodeNav);
            _device->d_octreeNodeNav = nullptr;
        }
        if (_device->d_octreeFirstChild) {
            bltzr_x::MemoryPool::deallocate(_device->d_octreeFirstChild);
            _device->d_octreeFirstChild = nullptr;
        }
        if (_device->d_octreeLeafStarts) {
            bltzr_x::MemoryPool::deallocate(_device->d_octreeLeafStarts);
            _device->d_octreeLeafStarts = nullptr;
        }
        if (_device->d_octreeLeafCounts) {
            bltzr_x::MemoryPool::deallocate(_device->d_octreeLeafCounts);
            _device->d_octreeLeafCounts = nullptr;
        }

        _device->g_dOctreeNodes = static_cast<GpuOctreeNode*>(bltzr_x::MemoryPool::allocate(
            static_cast<std::size_t>(requiredNodeCapacity) * sizeof(GpuOctreeNode)));
        _device->d_octreeNodeHot = static_cast<GpuOctreeNodeHotData*>(bltzr_x::MemoryPool::allocate(
            static_cast<std::size_t>(requiredNodeCapacity) * sizeof(GpuOctreeNodeHotData)));
        _device->d_octreeNodeNav = static_cast<GpuOctreeNodeNavData*>(bltzr_x::MemoryPool::allocate(
            static_cast<std::size_t>(requiredNodeCapacity) * sizeof(GpuOctreeNodeNavData)));
        _device->d_octreeFirstChild = static_cast<int*>(bltzr_x::MemoryPool::allocate(
            static_cast<std::size_t>(requiredNodeCapacity) * sizeof(int)));
        _device->d_octreeLeafStarts = static_cast<int*>(bltzr_x::MemoryPool::allocate(
            static_cast<std::size_t>(requiredNodeCapacity) * sizeof(int)));
        _device->d_octreeLeafCounts = static_cast<int*>(bltzr_x::MemoryPool::allocate(
            static_cast<std::size_t>(requiredNodeCapacity) * sizeof(int)));

        if (!_device->g_dOctreeNodes || !_device->d_octreeNodeHot || !_device->d_octreeNodeNav ||
            !_device->d_octreeFirstChild || !_device->d_octreeLeafStarts ||
            !_device->d_octreeLeafCounts) {
            if (_device->g_dOctreeNodes) {
                bltzr_x::MemoryPool::deallocate(_device->g_dOctreeNodes);
                _device->g_dOctreeNodes = nullptr;
            }
            if (_device->d_octreeNodeHot) {
                bltzr_x::MemoryPool::deallocate(_device->d_octreeNodeHot);
                _device->d_octreeNodeHot = nullptr;
            }
            if (_device->d_octreeNodeNav) {
                bltzr_x::MemoryPool::deallocate(_device->d_octreeNodeNav);
                _device->d_octreeNodeNav = nullptr;
            }
            if (_device->d_octreeFirstChild) {
                bltzr_x::MemoryPool::deallocate(_device->d_octreeFirstChild);
                _device->d_octreeFirstChild = nullptr;
            }
            if (_device->d_octreeLeafStarts) {
                bltzr_x::MemoryPool::deallocate(_device->d_octreeLeafStarts);
                _device->d_octreeLeafStarts = nullptr;
            }
            if (_device->d_octreeLeafCounts) {
                bltzr_x::MemoryPool::deallocate(_device->d_octreeLeafCounts);
                _device->d_octreeLeafCounts = nullptr;
            }
            _device->g_dOctreeNodeCapacity = 0;
            return false;
        }
        _device->g_dOctreeNodeCapacity = static_cast<std::size_t>(requiredNodeCapacity);
    }

    return true;
}

/*
 * @brief Documents the ensure energy scratch capacity operation contract.
 * @param numParticles Input value used by this contract.
 * @param sampleCount Input value used by this contract.
 * @return bool ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
