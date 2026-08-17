/*
 * @file engine/physics/cuda/fragments/system/buffer/SphScratch.inl
 * @project BLITZAR
 * @brief Particle-system buffer lifecycle implementation fragment.
 */

bool ParticleSystem::allocateSphBuffers(int numParticles)
{
    if (!_device->_cudaRuntimeAvailable) {
        return false;
    }
    releaseSphBuffers();
    _device->d_sphDensity =
        static_cast<float*>(bltzr_x::MemoryPool::allocate(numParticles * sizeof(float)));
    _device->d_sphPressure =
        static_cast<float*>(bltzr_x::MemoryPool::allocate(numParticles * sizeof(float)));

    if (!_device->d_sphDensity || !_device->d_sphPressure) {
        releaseSphBuffers();
        return false;
    }
    return true;
}

/*
 * @brief Documents the release sph buffers operation contract.
 * @param None This contract does not take explicit parameters.
 * @return void ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void ParticleSystem::releaseSphBuffers()
{
    if (_device->d_sphDensity) {
        bltzr_x::MemoryPool::deallocate(_device->d_sphDensity);
        _device->d_sphDensity = nullptr;
    }
    if (_device->d_sphPressure) {
        bltzr_x::MemoryPool::deallocate(_device->d_sphPressure);
        _device->d_sphPressure = nullptr;
    }
}

/*
 * @brief Documents the allocate sph grid buffers operation contract.
 * @param numParticles Input value used by this contract.
 * @return bool ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool ParticleSystem::allocateSphGridBuffers(int numParticles)
{
    if (!_device->_cudaRuntimeAvailable) {
        return false;
    }
    releaseSphGridBuffers();
    const std::size_t particleBytes = static_cast<std::size_t>(numParticles) * sizeof(int);
    _device->d_sphCellHash = static_cast<int*>(bltzr_x::MemoryPool::allocate(particleBytes));
    _device->d_sphSortedIndex = static_cast<int*>(bltzr_x::MemoryPool::allocate(particleBytes));

    if (!_device->d_sphCellHash || !_device->d_sphSortedIndex) {
        releaseSphGridBuffers();
        return false;
    }
    _hostCellHash.resize(numParticles);
    _hostSortedIndex.resize(numParticles);
    return true;
}

/*
 * @brief Documents the release sph grid buffers operation contract.
 * @param None This contract does not take explicit parameters.
 * @return void ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void ParticleSystem::releaseSphGridBuffers()
{
    if (_device->d_sphCellHash) {
        bltzr_x::MemoryPool::deallocate(_device->d_sphCellHash);
        _device->d_sphCellHash = nullptr;
    }
    if (_device->d_sphSortedIndex) {
        bltzr_x::MemoryPool::deallocate(_device->d_sphSortedIndex);
        _device->d_sphSortedIndex = nullptr;
    }
    if (_device->d_sphCellStart) {
        bltzr_x::MemoryPool::deallocate(_device->d_sphCellStart);
        _device->d_sphCellStart = nullptr;
    }
    if (_device->d_sphCellEnd) {
        bltzr_x::MemoryPool::deallocate(_device->d_sphCellEnd);
        _device->d_sphCellEnd = nullptr;
    }
    if (_device->d_treePmSortKeys) {
        bltzr_x::MemoryPool::deallocate(_device->d_treePmSortKeys);
        _device->d_treePmSortKeys = nullptr;
    }
    if (_device->d_treePmSortIndices) {
        bltzr_x::MemoryPool::deallocate(_device->d_treePmSortIndices);
        _device->d_treePmSortIndices = nullptr;
    }
    if (_device->d_treePmSortedCellHash) {
        bltzr_x::MemoryPool::deallocate(_device->d_treePmSortedCellHash);
        _device->d_treePmSortedCellHash = nullptr;
    }
    if (_device->d_treePmSortTempStorage) {
        bltzr_x::MemoryPool::deallocate(_device->d_treePmSortTempStorage);
        _device->d_treePmSortTempStorage = nullptr;
    }
    if (_device->d_treePmSortedPosX) {
        bltzr_x::MemoryPool::deallocate(_device->d_treePmSortedPosX);
        _device->d_treePmSortedPosX = nullptr;
    }
    if (_device->d_treePmSortedPosY) {
        bltzr_x::MemoryPool::deallocate(_device->d_treePmSortedPosY);
        _device->d_treePmSortedPosY = nullptr;
    }
    if (_device->d_treePmSortedPosZ) {
        bltzr_x::MemoryPool::deallocate(_device->d_treePmSortedPosZ);
        _device->d_treePmSortedPosZ = nullptr;
    }
    if (_device->d_treePmSortedMass) {
        bltzr_x::MemoryPool::deallocate(_device->d_treePmSortedMass);
        _device->d_treePmSortedMass = nullptr;
    }
    if (_device->d_treePmRadialMassHistogram) {
        bltzr_x::MemoryPool::deallocate(_device->d_treePmRadialMassHistogram);
        _device->d_treePmRadialMassHistogram = nullptr;
    }
    _hostCellHash.clear();
    _hostSortedIndex.clear();
    _device->d_treePmNeighborParticleCapacity = 0;
    _device->d_treePmNeighborCellCapacity = 0;
    _device->d_treePmSortTempCapacity = 0;
    _device->d_treePmSortedParticleCapacity = 0;
    _device->_treePmLayoutModeInitialized = false;
    _device->_treePmLayoutMode = 0;
    _device->_treePmAutoLayoutResolved = false;
    _device->_treePmAutoGather = false;
    _device->_treePmAutoMorton = false;
    _device->_treePmAutoR80Ratio = 1.0f;
}

/*
 * @brief Documents the seed device state operation contract.
 * @param None This contract does not take explicit parameters.
 * @return bool ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
