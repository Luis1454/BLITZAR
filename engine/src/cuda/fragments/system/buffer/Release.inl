/*
 * @file engine/src/cuda/fragments/system/buffer/Release.inl
 * @project BLITZAR
 * @brief Particle-system buffer lifecycle implementation fragment.
 */

void ParticleSystem::releaseParticleBuffers()
{
    releaseTreePmGraph();
    releaseRk4Buffers();
    releaseSphBuffers();
    releaseSphGridBuffers();

    bltzr_x::MemoryPool::deallocate(_device->d_soaPosX);
    _device->d_soaPosX = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_soaPosY);
    _device->d_soaPosY = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_soaPosZ);
    _device->d_soaPosZ = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_soaVelX);
    _device->d_soaVelX = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_soaVelY);
    _device->d_soaVelY = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_soaVelZ);

    bltzr_x::MemoryPool::deallocate(_device->d_treePmDensity);
    _device->d_treePmDensity = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_treePmPotentialA);
    _device->d_treePmPotentialA = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_treePmPotentialB);
    _device->d_treePmPotentialB = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_treePmAccelX);
    _device->d_treePmAccelX = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_treePmAccelY);
    _device->d_treePmAccelY = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_treePmAccelZ);
    _device->d_treePmAccelZ = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_treePmBoundsPartial);
    _device->d_treePmBoundsPartial = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_treePmBounds);
    _device->d_treePmBounds = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_treePmRadialMassHistogram);
    _device->d_treePmRadialMassHistogram = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_adaptiveAcceleration);
    _device->d_adaptiveAcceleration = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_adaptiveLevels);
    _device->d_adaptiveLevels = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_adaptiveLastForceTicks);
    _device->d_adaptiveLastForceTicks = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_treePmSpectrum);
    _device->d_treePmSpectrum = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_treePmSpectrumX);
    _device->d_treePmSpectrumX = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_treePmSpectrumY);
    _device->d_treePmSpectrumY = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_treePmSpectrumZ);
    _device->d_treePmSpectrumZ = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_treePmCellMask);
    _device->d_treePmCellMask = nullptr;
    _device->d_soaVelZ = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_soaPressX);
    _device->d_soaPressX = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_soaPressY);
    _device->d_soaPressY = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_soaPressZ);
    _device->d_soaPressZ = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_soaMass);
    _device->d_soaMass = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_soaTemp);
    _device->d_soaTemp = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_soaDens);
    _device->d_soaDens = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_soaNextPosX);
    _device->d_soaNextPosX = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_soaNextPosY);
    _device->d_soaNextPosY = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_soaNextPosZ);
    _device->d_soaNextPosZ = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_soaNextVelX);
    _device->d_soaNextVelX = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_soaNextVelY);
    _device->d_soaNextVelY = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_soaNextVelZ);
    _device->d_soaNextVelZ = nullptr;
    bltzr_x::MemoryPool::deallocate(_device->d_vHalf);
    _device->d_vHalf = nullptr;
    _device->_leapfrogPrimed = false;

    if (_device->g_dOctreeNodes) {
        bltzr_x::MemoryPool::deallocate(_device->g_dOctreeNodes);
        _device->g_dOctreeNodes = nullptr;
    }
    if (_device->g_dOctreeLeafIndices) {
        bltzr_x::MemoryPool::deallocate(_device->g_dOctreeLeafIndices);
        _device->g_dOctreeLeafIndices = nullptr;
    }
    if (_device->d_octreeMortonKeys) {
        bltzr_x::MemoryPool::deallocate(_device->d_octreeMortonKeys);
        _device->d_octreeMortonKeys = nullptr;
    }
    if (_device->d_octreePrefixesA) {
        bltzr_x::MemoryPool::deallocate(_device->d_octreePrefixesA);
        _device->d_octreePrefixesA = nullptr;
    }
    if (_device->d_octreePrefixesB) {
        bltzr_x::MemoryPool::deallocate(_device->d_octreePrefixesB);
        _device->d_octreePrefixesB = nullptr;
    }
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
    if (_device->d_energyKineticBlocks) {
        bltzr_x::MemoryPool::deallocate(_device->d_energyKineticBlocks);
        _device->d_energyKineticBlocks = nullptr;
    }
    if (_device->d_energyThermalBlocks) {
        bltzr_x::MemoryPool::deallocate(_device->d_energyThermalBlocks);
        _device->d_energyThermalBlocks = nullptr;
    }
    if (_device->d_energyPotentialPartials) {
        bltzr_x::MemoryPool::deallocate(_device->d_energyPotentialPartials);
        _device->d_energyPotentialPartials = nullptr;
    }
    _device->_deviceParticleCapacity = 0;
    _device->g_dOctreeNodeCapacity = 0;
    _device->g_dOctreeLeafCapacity = 0;
    _device->d_octreeMortonCapacity = 0;
    _device->d_octreePrefixCapacity = 0;
    _device->d_octreeLevelCapacity = 0;
    _device->d_treePmCapacity = 0;
    _device->d_treePmSpectrumCapacity = 0;
    _device->d_treePmMaskWordCapacity = 0;
    _device->d_treePmBoundsBlockCapacity = 0;
    _device->d_treePmNeighborParticleCapacity = 0;
    _device->d_treePmNeighborCellCapacity = 0;
    _device->d_adaptiveCapacity = 0;
    _device->_treePmFftPlan.reset();
    _device->_treePmFftInversePlan.reset();
    _device->_treePmFftPlanGridSize = 0;
    _device->_treePmFftActive = false;
    _device->d_energyBlockCapacity = 0;
    _device->d_energySampleCapacity = 0;
    _device->_gpuOctreeRootIndex = -1;
    _device->_gpuOctreeNodeCount = 0;
    _device->_gpuOctreeLeafCount = 0;
    releaseMappedMetrics();
}

/*
 * @brief Documents the allocate mapped metrics operation contract.
 * @param None This contract does not take explicit parameters.
 * @return bool ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
