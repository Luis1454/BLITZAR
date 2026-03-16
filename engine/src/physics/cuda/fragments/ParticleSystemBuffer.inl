void ParticleSystem::initializeRuntimeState(std::size_t particleCapacity)
{
    _solverMode = solverModeFromEnv();
    _integratorMode = integratorModeFromEnv();
    _octreeTheta = parseFloatEnv("GRAVITY_OCTREE_THETA", 1.2f);
    _octreeSoftening = parseFloatEnv("GRAVITY_OCTREE_SOFTENING", 0.05f);
    _sphEnabled = parseBoolEnv("GRAVITY_SPH_ENABLED", false);
    _sphSmoothingLength = parseFloatEnv("GRAVITY_SPH_H", 1.25f);
    _sphRestDensity = parseFloatEnv("GRAVITY_SPH_REST_DENSITY", 1.0f);
    _sphGasConstant = parseFloatEnv("GRAVITY_SPH_GAS_CONSTANT", 4.0f);
    _sphViscosity = parseFloatEnv("GRAVITY_SPH_VISCOSITY", 0.08f);
    _thermalAmbientTemperature = parseFloatEnv("GRAVITY_THERMAL_AMBIENT", 0.0f);
    _thermalSpecificHeat = parseFloatEnv("GRAVITY_THERMAL_SPECIFIC_HEAT", 1.0f);
    _thermalHeatingCoeff = parseFloatEnv("GRAVITY_THERMAL_HEATING", 0.0002f);
    _thermalRadiationCoeff = parseFloatEnv("GRAVITY_THERMAL_RADIATION", 0.00000001f);
    _physicsMaxAcceleration = 64.0f;
    _physicsMinSoftening = 1e-4f;
    _physicsMinDistance2 = 1e-12f;
    _physicsMinTheta = 0.05f;
    _sphMaxAcceleration = 40.0f;
    _sphMaxSpeed = 120.0f;
    _cumulativeRadiatedEnergy = 0.0f;
    _sphGridSize = 0;
    _sphGridTotalCells = 0;
    _deviceParticleCapacity = particleCapacity;
    _hostStateDirty = false;
    d_soaPosX = nullptr;
    d_soaPosY = nullptr;
    d_soaPosZ = nullptr;
    d_soaVelX = nullptr;
    d_soaVelY = nullptr;
    d_soaVelZ = nullptr;
    d_soaPressX = nullptr;
    d_soaPressY = nullptr;
    d_soaPressZ = nullptr;
    d_soaMass = nullptr;
    d_soaTemp = nullptr;
    d_soaDens = nullptr;
    d_soaNextPosX = nullptr;
    d_soaNextPosY = nullptr;
    d_soaNextPosZ = nullptr;
    d_soaNextVelX = nullptr;
    d_soaNextVelY = nullptr;
    d_soaNextVelZ = nullptr;
    d_sphDensity = nullptr;
    d_sphPressure = nullptr;
    d_sphCellHash = nullptr;
    d_sphSortedIndex = nullptr;
    d_sphCellStart = nullptr;
    d_sphCellEnd = nullptr;
    g_dOctreeNodes = nullptr;
    g_dOctreeLeafIndices = nullptr;
    g_dOctreeNodeCapacity = 0;
    g_dOctreeLeafCapacity = 0;
}

bool ParticleSystem::allocateParticleBuffers(std::size_t particleCapacity)
{
    const std::size_t bytesTotal = particleCapacity * sizeof(float);
    bool ok = true;
    ok &= checkCudaStatus(cudaMalloc(&d_soaPosX, bytesTotal), "malloc(posX)");
    ok &= checkCudaStatus(cudaMalloc(&d_soaPosY, bytesTotal), "malloc(posY)");
    ok &= checkCudaStatus(cudaMalloc(&d_soaPosZ, bytesTotal), "malloc(posZ)");
    ok &= checkCudaStatus(cudaMalloc(&d_soaVelX, bytesTotal), "malloc(velX)");
    ok &= checkCudaStatus(cudaMalloc(&d_soaVelY, bytesTotal), "malloc(velY)");
    ok &= checkCudaStatus(cudaMalloc(&d_soaVelZ, bytesTotal), "malloc(velZ)");
    ok &= checkCudaStatus(cudaMalloc(&d_soaPressX, bytesTotal), "malloc(pressX)");
    ok &= checkCudaStatus(cudaMalloc(&d_soaPressY, bytesTotal), "malloc(pressY)");
    ok &= checkCudaStatus(cudaMalloc(&d_soaPressZ, bytesTotal), "malloc(pressZ)");
    ok &= checkCudaStatus(cudaMalloc(&d_soaMass, bytesTotal), "malloc(mass)");
    ok &= checkCudaStatus(cudaMalloc(&d_soaTemp, bytesTotal), "malloc(temp)");
    ok &= checkCudaStatus(cudaMalloc(&d_soaDens, bytesTotal), "malloc(dens)");
    ok &= checkCudaStatus(cudaMalloc(&d_soaNextPosX, bytesTotal), "malloc(nextPosX)");
    ok &= checkCudaStatus(cudaMalloc(&d_soaNextPosY, bytesTotal), "malloc(nextPosY)");
    ok &= checkCudaStatus(cudaMalloc(&d_soaNextPosZ, bytesTotal), "malloc(nextPosZ)");
    ok &= checkCudaStatus(cudaMalloc(&d_soaNextVelX, bytesTotal), "malloc(nextVelX)");
    ok &= checkCudaStatus(cudaMalloc(&d_soaNextVelY, bytesTotal), "malloc(nextVelY)");
    ok &= checkCudaStatus(cudaMalloc(&d_soaNextVelZ, bytesTotal), "malloc(nextVelZ)");
    
    if (!ok) {
        releaseParticleBuffers();
        return false;
    }
    return true;
}

void ParticleSystem::releaseParticleBuffers()
{
    releaseRk4Buffers();
    releaseSphBuffers();
    releaseSphGridBuffers();
    
    cudaFree(d_soaPosX); d_soaPosX = nullptr;
    cudaFree(d_soaPosY); d_soaPosY = nullptr;
    cudaFree(d_soaPosZ); d_soaPosZ = nullptr;
    cudaFree(d_soaVelX); d_soaVelX = nullptr;
    cudaFree(d_soaVelY); d_soaVelY = nullptr;
    cudaFree(d_soaVelZ); d_soaVelZ = nullptr;
    cudaFree(d_soaPressX); d_soaPressX = nullptr;
    cudaFree(d_soaPressY); d_soaPressY = nullptr;
    cudaFree(d_soaPressZ); d_soaPressZ = nullptr;
    cudaFree(d_soaMass); d_soaMass = nullptr;
    cudaFree(d_soaTemp); d_soaTemp = nullptr;
    cudaFree(d_soaDens); d_soaDens = nullptr;
    cudaFree(d_soaNextPosX); d_soaNextPosX = nullptr;
    cudaFree(d_soaNextPosY); d_soaNextPosY = nullptr;
    cudaFree(d_soaNextPosZ); d_soaNextPosZ = nullptr;
    cudaFree(d_soaNextVelX); d_soaNextVelX = nullptr;
    cudaFree(d_soaNextVelY); d_soaNextVelY = nullptr;
    cudaFree(d_soaNextVelZ); d_soaNextVelZ = nullptr;

    if (g_dOctreeNodes) {
        cudaFree(g_dOctreeNodes);
        g_dOctreeNodes = nullptr;
    }
    if (g_dOctreeLeafIndices) {
        cudaFree(g_dOctreeLeafIndices);
        g_dOctreeLeafIndices = nullptr;
    }
    _deviceParticleCapacity = 0;
    g_dOctreeNodeCapacity = 0;
    g_dOctreeLeafCapacity = 0;
}

bool ParticleSystem::allocateRk4Buffers(int numParticles)
{
    releaseRk4Buffers();
    if (!checkCudaStatus(cudaMalloc(&d_stage, numParticles * sizeof(Particle)), "cudaMalloc(d_stage)")) {
        releaseRk4Buffers();
        return false;
    }
    if (!checkCudaStatus(cudaMalloc(&d_k1x, numParticles * sizeof(Vector3)), "cudaMalloc(d_k1x)")) {
        releaseRk4Buffers();
        return false;
    }
    if (!checkCudaStatus(cudaMalloc(&d_k2x, numParticles * sizeof(Vector3)), "cudaMalloc(d_k2x)")) {
        releaseRk4Buffers();
        return false;
    }
    if (!checkCudaStatus(cudaMalloc(&d_k3x, numParticles * sizeof(Vector3)), "cudaMalloc(d_k3x)")) {
        releaseRk4Buffers();
        return false;
    }
    if (!checkCudaStatus(cudaMalloc(&d_k4x, numParticles * sizeof(Vector3)), "cudaMalloc(d_k4x)")) {
        releaseRk4Buffers();
        return false;
    }
    if (!checkCudaStatus(cudaMalloc(&d_k1v, numParticles * sizeof(Vector3)), "cudaMalloc(d_k1v)")) {
        releaseRk4Buffers();
        return false;
    }
    if (!checkCudaStatus(cudaMalloc(&d_k2v, numParticles * sizeof(Vector3)), "cudaMalloc(d_k2v)")) {
        releaseRk4Buffers();
        return false;
    }
    if (!checkCudaStatus(cudaMalloc(&d_k3v, numParticles * sizeof(Vector3)), "cudaMalloc(d_k3v)")) {
        releaseRk4Buffers();
        return false;
    }
    if (!checkCudaStatus(cudaMalloc(&d_k4v, numParticles * sizeof(Vector3)), "cudaMalloc(d_k4v)")) {
        releaseRk4Buffers();
        return false;
    }
    return true;
}

void ParticleSystem::releaseRk4Buffers()
{
    if (d_stage) { cudaFree(d_stage); d_stage = nullptr; }
    if (d_k1x) { cudaFree(d_k1x); d_k1x = nullptr; }
    if (d_k2x) { cudaFree(d_k2x); d_k2x = nullptr; }
    if (d_k3x) { cudaFree(d_k3x); d_k3x = nullptr; }
    if (d_k4x) { cudaFree(d_k4x); d_k4x = nullptr; }
    if (d_k1v) { cudaFree(d_k1v); d_k1v = nullptr; }
    if (d_k2v) { cudaFree(d_k2v); d_k2v = nullptr; }
    if (d_k3v) { cudaFree(d_k3v); d_k3v = nullptr; }
    if (d_k4v) { cudaFree(d_k4v); d_k4v = nullptr; }
}

bool ParticleSystem::allocateSphBuffers(int numParticles)
{
    releaseSphBuffers();
    if (!checkCudaStatus(cudaMalloc(&d_sphDensity, numParticles * sizeof(float)), "cudaMalloc(d_sphDensity)")) {
        releaseSphBuffers();
        return false;
    }
    if (!checkCudaStatus(cudaMalloc(&d_sphPressure, numParticles * sizeof(float)), "cudaMalloc(d_sphPressure)")) {
        releaseSphBuffers();
        return false;
    }
    return true;
}

void ParticleSystem::releaseSphBuffers()
{
    if (d_sphDensity) { cudaFree(d_sphDensity); d_sphDensity = nullptr; }
    if (d_sphPressure) { cudaFree(d_sphPressure); d_sphPressure = nullptr; }
}

bool ParticleSystem::allocateSphGridBuffers(int numParticles)
{
    releaseSphGridBuffers();
    const std::size_t particleBytes = static_cast<std::size_t>(numParticles) * sizeof(int);
    if (!checkCudaStatus(cudaMalloc(&d_sphCellHash, particleBytes), "cudaMalloc(d_sphCellHash)")) {
        releaseSphGridBuffers();
        return false;
    }
    if (!checkCudaStatus(cudaMalloc(&d_sphSortedIndex, particleBytes), "cudaMalloc(d_sphSortedIndex)")) {
        releaseSphGridBuffers();
        return false;
    }
    _hostCellHash.resize(numParticles);
    _hostSortedIndex.resize(numParticles);
    return true;
}

void ParticleSystem::releaseSphGridBuffers()
{
    if (d_sphCellHash) { cudaFree(d_sphCellHash); d_sphCellHash = nullptr; }
    if (d_sphSortedIndex) { cudaFree(d_sphSortedIndex); d_sphSortedIndex = nullptr; }
    if (d_sphCellStart) { cudaFree(d_sphCellStart); d_sphCellStart = nullptr; }
    if (d_sphCellEnd) { cudaFree(d_sphCellEnd); d_sphCellEnd = nullptr; }
    _hostCellHash.clear();
    _hostSortedIndex.clear();
}

bool ParticleSystem::seedDeviceState()
{
    if (_particles.empty()) return true;
    syncDeviceState();
    return true;
}
