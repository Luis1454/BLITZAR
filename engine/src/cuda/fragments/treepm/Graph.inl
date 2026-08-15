/*
 * @file engine/src/cuda/fragments/treepm/Graph.inl
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
        !_device._treePmFftActive || _device._treePmFftPlan == 0 ||
        _device._treePmFftInversePlan == 0) {
        return false;
    }
    cudaGraph_t graph = nullptr;
    cudaGraphExec_t executable = nullptr;
    cudaStream_t stream = static_cast<cudaStream_t>(_device._treePmGraphStream);
    if (stream == nullptr &&
        cudaStreamCreateWithFlags(&stream, cudaStreamNonBlocking) == cudaSuccess) {
        _device._treePmGraphStream = stream;
    }
    if (stream == nullptr) {
        return false;
    }
    const cufftHandle forwardPlan = static_cast<cufftHandle>(_device._treePmFftPlan);
    const cufftHandle inversePlan = static_cast<cufftHandle>(_device._treePmFftInversePlan);
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
    if (cudaMemsetAsync(_device.d_treePmDensity, 0, densityBytes, stream) != cudaSuccess ||
        cudaMemsetAsync(_device.d_treePmCellMask, 0, maskBytes, stream) != cudaSuccess) {
        return abortCapture();
    }
    const int depositBlocks =
        (particleLimit + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    treepm::treePmDepositMassKernel<<<depositBlocks, Particle::kDefaultCudaBlockSize, 0, stream>>>(
        currentView, numParticles, particleLimit, grid, _device.d_treePmDensity,
        _device.d_treePmCellMask);
    if (cudaGetLastError() != cudaSuccess) {
        return abortCapture();
    }

    if (cufftExecR2C(forwardPlan, reinterpret_cast<cufftReal*>(_device.d_treePmDensity),
                     reinterpret_cast<cufftComplex*>(_device.d_treePmSpectrum)) != CUFFT_SUCCESS) {
        return abortCapture();
    }

    const int spectrumCells = grid.gridSize * grid.gridSize * (grid.gridSize / 2 + 1);
    const int spectrumBlocks =
        (spectrumCells + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    treepm::treePmApplyFftKernel<<<spectrumBlocks, Particle::kDefaultCudaBlockSize, 0, stream>>>(
        reinterpret_cast<const cufftComplex*>(_device.d_treePmSpectrum),
        reinterpret_cast<cufftComplex*>(_device.d_treePmSpectrumX), grid,
        std::max(_octreeSoftening, _physicsMinSoftening));
    if (cudaGetLastError() != cudaSuccess ||
        cufftExecC2R(inversePlan, reinterpret_cast<cufftComplex*>(_device.d_treePmSpectrumX),
                     reinterpret_cast<cufftReal*>(_device.d_treePmPotentialA)) != CUFFT_SUCCESS) {
        return abortCapture();
    }

    const int fieldBlocks =
        (grid.totalCells + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    treepm::treePmNormalizeFftFieldKernel<<<fieldBlocks, Particle::kDefaultCudaBlockSize, 0, stream>>>(
        _device.d_treePmPotentialA, grid.totalCells);
    treepm::treePmPotentialGradientKernel<<<fieldBlocks, Particle::kDefaultCudaBlockSize, 0, stream>>>(
        _device.d_treePmPotentialA, _device.d_treePmAccelX, _device.d_treePmAccelY,
        _device.d_treePmAccelZ, grid);
    const int updateBlocks =
        (numParticles + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    treepm::updateParticlesTreePmLocalGridKernel<<<updateBlocks, Particle::kDefaultCudaBlockSize, 0,
                                           stream>>>(
        currentView, nextView, numParticles, grid, nullptr, nullptr, nullptr, forceLaw, deltaTime,
        maxAcceleration, _device.d_treePmAccelX, _device.d_treePmAccelY, _device.d_treePmAccelZ,
        _device.d_treePmCellMask, cutoffSquared, 1, 0, nullptr, nullptr, nullptr, nullptr);
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
    if (_device._treePmGraphExec[slot] != nullptr) {
        cudaGraphExecDestroy(static_cast<cudaGraphExec_t>(_device._treePmGraphExec[slot]));
    }
    _device._treePmGraphExec[slot] = executable;
    _device._treePmGraphCaptured[slot] = true;
    return true;
}
bool ParticleSystem::launchTreePmGraph(int slot)
{
    if (slot < 0 || slot > 1 || !_device._treePmGraphCaptured[slot] ||
        _device._treePmGraphExec[slot] == nullptr) {
        return false;
    }
    return checkCudaStatus(
        cudaGraphLaunch(static_cast<cudaGraphExec_t>(_device._treePmGraphExec[slot]), nullptr),
        "cudaGraphLaunch(treepm)");
}

void ParticleSystem::releaseTreePmGraph()
{
    for (int slot = 0; slot < 2; ++slot) {
        if (_device._treePmGraphExec[slot] != nullptr) {
            cudaGraphExecDestroy(static_cast<cudaGraphExec_t>(_device._treePmGraphExec[slot]));
            _device._treePmGraphExec[slot] = nullptr;
        }
        _device._treePmGraphCaptured[slot] = false;
    }
    if (_device._treePmGraphStream != nullptr) {
        cudaStreamDestroy(static_cast<cudaStream_t>(_device._treePmGraphStream));
        _device._treePmGraphStream = nullptr;
    }
    _device._treePmGraphSlot = 0;
}
