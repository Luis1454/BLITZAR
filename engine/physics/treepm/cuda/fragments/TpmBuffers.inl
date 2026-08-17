/*
 * @file engine/physics/treepm/cuda/fragments/TpmBuffers.inl
 * @project BLITZAR
 * @brief TreePM field and reduction buffer capacity management.
 */

bool ParticleSystem::ensureTreePmBoundsCapacity(int numParticles)
{
    if (!_device->_cudaRuntimeAvailable || numParticles <= 0) {
        return false;
    }
    const std::size_t blockCount =
        (static_cast<std::size_t>(numParticles) + treepm::kTreePmBoundsBlockSize - 1u) /
        treepm::kTreePmBoundsBlockSize;
    if (_device->d_treePmBoundsBlockCapacity >= blockCount && _device->d_treePmBoundsPartial != nullptr &&
        _device->d_treePmBounds != nullptr) {
        return true;
    }

    bltzr_x::MemoryPool::deallocate(_device->d_treePmBoundsPartial);
    bltzr_x::MemoryPool::deallocate(_device->d_treePmBounds);
    _device->d_treePmBoundsPartial = static_cast<float*>(bltzr_x::MemoryPool::allocate(
        blockCount * treepm::kTreePmBoundsFieldCount * sizeof(float)));
    _device->d_treePmBounds = static_cast<float*>(
        bltzr_x::MemoryPool::allocate(treepm::kTreePmBoundsFieldCount * sizeof(float)));
    if (_device->d_treePmBoundsPartial == nullptr || _device->d_treePmBounds == nullptr) {
        bltzr_x::MemoryPool::deallocate(_device->d_treePmBoundsPartial);
        bltzr_x::MemoryPool::deallocate(_device->d_treePmBounds);
        _device->d_treePmBoundsPartial = nullptr;
        _device->d_treePmBounds = nullptr;
        _device->d_treePmBoundsBlockCapacity = 0u;
        return false;
    }
    _device->d_treePmBoundsBlockCapacity = blockCount;
    return true;
}
bool ParticleSystem::ensureTreePmConcentrationCapacity()
{
    if (!_device->_cudaRuntimeAvailable) {
        return false;
    }
    if (_device->d_treePmRadialMassHistogram != nullptr) {
        return true;
    }
    _device->d_treePmRadialMassHistogram = static_cast<float*>(
        bltzr_x::MemoryPool::allocate(treepm::kTreePmConcentrationBinCount * sizeof(float)));
    return _device->d_treePmRadialMassHistogram != nullptr;
}

bool ParticleSystem::ensureTreePmScratchCapacity(int gridCells, int gridSize)
{
    if (!_device->_cudaRuntimeAvailable || gridCells <= 0 || gridSize <= 0) {
        return false;
    }
    const std::size_t cells = static_cast<std::size_t>(gridCells);
    const std::size_t spectrumCells = static_cast<std::size_t>(gridSize) *
                                      static_cast<std::size_t>(gridSize) *
                                      static_cast<std::size_t>(gridSize / 2 + 1);
    const std::size_t maskWords = (cells + 31u) / 32u;
    if (_device->d_treePmCapacity >= cells && _device->d_treePmMaskWordCapacity >= maskWords &&
        _device->d_treePmSpectrumCapacity >= spectrumCells && _device->d_treePmCellMask != nullptr &&
        _device->d_treePmSpectrumZ != nullptr) {
        return true;
    }

    bltzr_x::MemoryPool::deallocate(_device->d_treePmDensity);
    bltzr_x::MemoryPool::deallocate(_device->d_treePmPotentialA);
    bltzr_x::MemoryPool::deallocate(_device->d_treePmPotentialB);
    bltzr_x::MemoryPool::deallocate(_device->d_treePmAccelX);
    bltzr_x::MemoryPool::deallocate(_device->d_treePmAccelY);
    bltzr_x::MemoryPool::deallocate(_device->d_treePmAccelZ);
    bltzr_x::MemoryPool::deallocate(_device->d_treePmSpectrum);
    bltzr_x::MemoryPool::deallocate(_device->d_treePmSpectrumX);
    bltzr_x::MemoryPool::deallocate(_device->d_treePmSpectrumY);
    bltzr_x::MemoryPool::deallocate(_device->d_treePmSpectrumZ);
    bltzr_x::MemoryPool::deallocate(_device->d_treePmCellMask);
    _device->d_treePmDensity = nullptr;
    _device->d_treePmPotentialA = nullptr;
    _device->d_treePmPotentialB = nullptr;
    _device->d_treePmAccelX = nullptr;
    _device->d_treePmAccelY = nullptr;
    _device->d_treePmAccelZ = nullptr;
    _device->d_treePmSpectrum = nullptr;
    _device->d_treePmSpectrumX = nullptr;
    _device->d_treePmSpectrumY = nullptr;
    _device->d_treePmSpectrumZ = nullptr;
    _device->d_treePmCellMask = nullptr;
    _device->d_treePmCapacity = 0u;
    _device->d_treePmSpectrumCapacity = 0u;
    _device->d_treePmMaskWordCapacity = 0u;

    _device->d_treePmDensity =
        static_cast<float*>(bltzr_x::MemoryPool::allocate(cells * sizeof(float)));
    _device->d_treePmPotentialA =
        static_cast<float*>(bltzr_x::MemoryPool::allocate(cells * sizeof(float)));
    _device->d_treePmPotentialB =
        static_cast<float*>(bltzr_x::MemoryPool::allocate(cells * sizeof(float)));
    _device->d_treePmAccelX =
        static_cast<float*>(bltzr_x::MemoryPool::allocate(cells * sizeof(float)));
    _device->d_treePmAccelY =
        static_cast<float*>(bltzr_x::MemoryPool::allocate(cells * sizeof(float)));
    _device->d_treePmAccelZ =
        static_cast<float*>(bltzr_x::MemoryPool::allocate(cells * sizeof(float)));
    _device->d_treePmSpectrum =
        bltzr_x::MemoryPool::allocate(spectrumCells * sizeof(cufftComplex));
    _device->d_treePmSpectrumX =
        bltzr_x::MemoryPool::allocate(spectrumCells * sizeof(cufftComplex));
    _device->d_treePmSpectrumY =
        bltzr_x::MemoryPool::allocate(spectrumCells * sizeof(cufftComplex));
    _device->d_treePmSpectrumZ =
        bltzr_x::MemoryPool::allocate(spectrumCells * sizeof(cufftComplex));
    _device->d_treePmCellMask =
        static_cast<unsigned int*>(bltzr_x::MemoryPool::allocate(maskWords * sizeof(unsigned int)));
    if (!_device->d_treePmDensity || !_device->d_treePmPotentialA || !_device->d_treePmPotentialB ||
        !_device->d_treePmAccelX || !_device->d_treePmAccelY || !_device->d_treePmAccelZ ||
        !_device->d_treePmSpectrum || !_device->d_treePmSpectrumX || !_device->d_treePmSpectrumY ||
        !_device->d_treePmSpectrumZ || !_device->d_treePmCellMask) {
        bltzr_x::MemoryPool::deallocate(_device->d_treePmDensity);
        bltzr_x::MemoryPool::deallocate(_device->d_treePmPotentialA);
        bltzr_x::MemoryPool::deallocate(_device->d_treePmPotentialB);
        bltzr_x::MemoryPool::deallocate(_device->d_treePmAccelX);
        bltzr_x::MemoryPool::deallocate(_device->d_treePmAccelY);
        bltzr_x::MemoryPool::deallocate(_device->d_treePmAccelZ);
        bltzr_x::MemoryPool::deallocate(_device->d_treePmSpectrum);
        bltzr_x::MemoryPool::deallocate(_device->d_treePmSpectrumX);
        bltzr_x::MemoryPool::deallocate(_device->d_treePmSpectrumY);
        bltzr_x::MemoryPool::deallocate(_device->d_treePmSpectrumZ);
        bltzr_x::MemoryPool::deallocate(_device->d_treePmCellMask);
        _device->d_treePmDensity = nullptr;
        _device->d_treePmPotentialA = nullptr;
        _device->d_treePmPotentialB = nullptr;
        _device->d_treePmAccelX = nullptr;
        _device->d_treePmAccelY = nullptr;
        _device->d_treePmAccelZ = nullptr;
        _device->d_treePmSpectrum = nullptr;
        _device->d_treePmSpectrumX = nullptr;
        _device->d_treePmSpectrumY = nullptr;
        _device->d_treePmSpectrumZ = nullptr;
        _device->d_treePmCellMask = nullptr;
        return false;
    }

    _device->d_treePmCapacity = cells;
    _device->d_treePmSpectrumCapacity = spectrumCells;
    _device->d_treePmMaskWordCapacity = maskWords;
    return true;
}
