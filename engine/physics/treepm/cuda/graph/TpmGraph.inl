/*
 * @file engine/physics/treepm/cuda/graph/TpmGraph.inl
 * @project BLITZAR
 * @brief TreePM CUDA Graph capture and lifecycle.
 */

bool ParticleSystem::captureTreePmGraph(int slot, ParticleSoAView currentView,
                                        ParticleSoAView nextView, int numParticles,
                                        int particleLimit, const TreePmGridParams& grid,
                                        float cutoffSquared, ForceLawPolicy forceLaw,
                                        float deltaTime, float maxAcceleration)
{
    if (slot < 0 || slot > 1 || numParticles <= 0 || particleLimit <= 0 ||
        !_device->_treePmFftActive || _device->_treePmFftPlan.get() == 0 ||
        _device->_treePmFftInversePlan.get() == 0) {
        return false;
    }
    cudaGraph_t graph = nullptr;
    cudaGraphExec_t executable = nullptr;
    cudaStream_t stream = static_cast<cudaStream_t>(_device->_treePmGraphStream.get());
    if (stream == nullptr &&
        cudaStreamCreateWithFlags(&stream, cudaStreamNonBlocking) == cudaSuccess) {
        _device->_treePmGraphStream = static_cast<void*>(stream);
    }
    if (stream == nullptr) {
        return false;
    }
    const cufftHandle forwardPlan = static_cast<cufftHandle>(_device->_treePmFftPlan.get());
    const cufftHandle inversePlan = static_cast<cufftHandle>(_device->_treePmFftInversePlan.get());
    if (cufftSetStream(forwardPlan, stream) != CUFFT_SUCCESS ||
        cufftSetStream(inversePlan, stream) != CUFFT_SUCCESS) {
        cufftSetStream(forwardPlan, nullptr);
        cufftSetStream(inversePlan, nullptr);
        return false;
    }
    if (cudaStreamBeginCapture(stream, cudaStreamCaptureModeGlobal) != cudaSuccess) {
        cufftSetStream(forwardPlan, nullptr);
        cufftSetStream(inversePlan, nullptr);
        return false;
    }
    auto abortCapture = [&]() {
        cudaGraph_t abandoned = nullptr;
        cudaStreamEndCapture(stream, &abandoned);
        if (abandoned != nullptr) {
            cudaGraphDestroy(abandoned);
        }
        cufftSetStream(forwardPlan, nullptr);
        cufftSetStream(inversePlan, nullptr);
        return false;
    };

    const std::size_t densityBytes = static_cast<std::size_t>(grid.totalCells) * sizeof(float);
    const std::size_t maskBytes =
        (static_cast<std::size_t>(grid.totalCells) + 31u) / 32u * sizeof(unsigned int);
    if (cudaMemsetAsync(_device->d_treePmDensity, 0, densityBytes, stream) != cudaSuccess ||
        cudaMemsetAsync(_device->d_treePmCellMask, 0, maskBytes, stream) != cudaSuccess) {
        return abortCapture();
    }
    const int depositBlocks =
        (particleLimit + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    treepm::treePmDepositMassKernel<<<depositBlocks, Particle::kDefaultCudaBlockSize, 0, stream>>>(
        currentView, numParticles, particleLimit, grid, _device->d_treePmDensity,
        _device->d_treePmCellMask);
    if (cudaGetLastError() != cudaSuccess) {
        return abortCapture();
    }

        if (cufftExecR2C(forwardPlan, reinterpret_cast<cufftReal*>(_device->d_treePmDensity.get()),
                         reinterpret_cast<cufftComplex*>(_device->d_treePmSpectrum.get())) !=
            CUFFT_SUCCESS) {
        return abortCapture();
    }

    const int spectrumCells = grid.gridSize * grid.gridSize * (grid.gridSize / 2 + 1);
    const int spectrumBlocks =
        (spectrumCells + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    treepm::treePmApplyFftKernel<<<spectrumBlocks, Particle::kDefaultCudaBlockSize, 0, stream>>>(
          reinterpret_cast<const cufftComplex*>(_device->d_treePmSpectrum.get()),
          reinterpret_cast<cufftComplex*>(_device->d_treePmSpectrumX.get()), grid,
        std::max(_octreeSoftening, _physicsMinSoftening));
    if (cudaGetLastError() != cudaSuccess ||
            cufftExecC2R(inversePlan,
                         reinterpret_cast<cufftComplex*>(_device->d_treePmSpectrumX.get()),
                         reinterpret_cast<cufftReal*>(_device->d_treePmPotentialA.get())) !=
            CUFFT_SUCCESS) {
        return abortCapture();
    }

    const int fieldBlocks =
        (grid.totalCells + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    treepm::treePmNormalizeFftFieldKernel<<<fieldBlocks, Particle::kDefaultCudaBlockSize, 0, stream>>>(
        _device->d_treePmPotentialA, grid.totalCells);
    treepm::treePmPotentialGradientKernel<<<fieldBlocks, Particle::kDefaultCudaBlockSize, 0, stream>>>(
        _device->d_treePmPotentialA, _device->d_treePmAccelX, _device->d_treePmAccelY,
        _device->d_treePmAccelZ, grid);
    const int updateBlocks =
        (numParticles + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    treepm::updateParticlesTreePmLocalGridKernel<<<updateBlocks, Particle::kDefaultCudaBlockSize, 0,
                                           stream>>>(
        currentView, nextView, numParticles, grid, nullptr, nullptr, nullptr, forceLaw, deltaTime,
        maxAcceleration, _device->d_treePmAccelX, _device->d_treePmAccelY, _device->d_treePmAccelZ,
        _device->d_treePmCellMask, cutoffSquared, 1, 0, nullptr, nullptr, nullptr, nullptr);
    const cudaError_t launchStatus = cudaGetLastError();
    const cudaError_t captureStatus = cudaStreamEndCapture(stream, &graph);
    if (launchStatus != cudaSuccess || captureStatus != cudaSuccess || graph == nullptr ||
        cudaGraphInstantiate(&executable, graph, nullptr, nullptr, 0) != cudaSuccess) {
        if (graph != nullptr) {
            cudaGraphDestroy(graph);
        }
        if (executable != nullptr) {
            cudaGraphExecDestroy(executable);
        }
        cufftSetStream(forwardPlan, nullptr);
        cufftSetStream(inversePlan, nullptr);
        return false;
    }
    cudaGraphDestroy(graph);
    cufftSetStream(forwardPlan, nullptr);
    cufftSetStream(inversePlan, nullptr);
    _device->_treePmGraphExec[slot] = static_cast<void*>(executable);
    _device->_treePmGraphCaptured[slot] = true;
    return true;
}
bool ParticleSystem::launchTreePmGraph(int slot)
{
    if (slot < 0 || slot > 1 || !_device->_treePmGraphCaptured[slot] ||
        _device->_treePmGraphExec[slot].get() == nullptr) {
        return false;
    }
    return checkCudaStatus(
        cudaGraphLaunch(static_cast<cudaGraphExec_t>(_device->_treePmGraphExec[slot].get()), nullptr),
        "cudaGraphLaunch(treepm)");
}

void ParticleSystem::releaseTreePmGraph()
{
    for (int slot = 0; slot < 2; ++slot) {
        _device->_treePmGraphExec[slot].reset();
        _device->_treePmGraphCaptured[slot] = false;
    }
    _device->_treePmGraphStream.reset();
    _device->_treePmGraphSlot = 0;
}
