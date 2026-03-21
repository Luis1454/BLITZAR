/*
 * Module: physics/cuda
 * Responsibility: Manage particle-system buffer allocation and release paths.
 */

void ParticleSystem::initializeRuntimeState(std::size_t particleCapacity)
{
    grav_x::CudaMemoryPool::initialize();
    _solverMode = solverModeFromEnv();
    _integratorMode = integratorModeFromEnv();
    _octreeTheta = parseFloatEnv("GRAVITY_OCTREE_THETA", 1.2f);
    _octreeSoftening = parseFloatEnv("GRAVITY_OCTREE_SOFTENING", 2.5f);
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
    d_stage = nullptr;
    d_k1x = nullptr; d_k2x = nullptr; d_k3x = nullptr; d_k4x = nullptr;
    d_k1v = nullptr; d_k2v = nullptr; d_k3v = nullptr; d_k4v = nullptr;
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
    ok &= ((d_soaPosX = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaPosY = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaPosZ = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaVelX = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaVelY = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaVelZ = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaPressX = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaPressY = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaPressZ = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaMass = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaTemp = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaDens = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaNextPosX = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaNextPosY = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaNextPosZ = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaNextVelX = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaNextVelY = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((d_soaNextVelZ = static_cast<float*>(grav_x::CudaMemoryPool::allocate(bytesTotal))) != nullptr);
    
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
    
    grav_x::CudaMemoryPool::deallocate(d_soaPosX); d_soaPosX = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaPosY); d_soaPosY = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaPosZ); d_soaPosZ = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaVelX); d_soaVelX = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaVelY); d_soaVelY = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaVelZ); d_soaVelZ = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaPressX); d_soaPressX = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaPressY); d_soaPressY = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaPressZ); d_soaPressZ = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaMass); d_soaMass = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaTemp); d_soaTemp = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaDens); d_soaDens = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaNextPosX); d_soaNextPosX = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaNextPosY); d_soaNextPosY = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaNextPosZ); d_soaNextPosZ = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaNextVelX); d_soaNextVelX = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaNextVelY); d_soaNextVelY = nullptr;
    grav_x::CudaMemoryPool::deallocate(d_soaNextVelZ); d_soaNextVelZ = nullptr;

    if (g_dOctreeNodes) {
        grav_x::CudaMemoryPool::deallocate(g_dOctreeNodes);
        g_dOctreeNodes = nullptr;
    }
    if (g_dOctreeLeafIndices) {
        grav_x::CudaMemoryPool::deallocate(g_dOctreeLeafIndices);
        g_dOctreeLeafIndices = nullptr;
    }
    _deviceParticleCapacity = 0;
    g_dOctreeNodeCapacity = 0;
    g_dOctreeLeafCapacity = 0;
}

bool ParticleSystem::allocateRk4Buffers(int numParticles)
{
    releaseRk4Buffers();
    d_stage = static_cast<Particle*>(grav_x::CudaMemoryPool::allocate(numParticles * sizeof(Particle)));
    d_k1x = static_cast<Vector3*>(grav_x::CudaMemoryPool::allocate(numParticles * sizeof(Vector3)));
    d_k2x = static_cast<Vector3*>(grav_x::CudaMemoryPool::allocate(numParticles * sizeof(Vector3)));
    d_k3x = static_cast<Vector3*>(grav_x::CudaMemoryPool::allocate(numParticles * sizeof(Vector3)));
    d_k4x = static_cast<Vector3*>(grav_x::CudaMemoryPool::allocate(numParticles * sizeof(Vector3)));
    d_k1v = static_cast<Vector3*>(grav_x::CudaMemoryPool::allocate(numParticles * sizeof(Vector3)));
    d_k2v = static_cast<Vector3*>(grav_x::CudaMemoryPool::allocate(numParticles * sizeof(Vector3)));
    d_k3v = static_cast<Vector3*>(grav_x::CudaMemoryPool::allocate(numParticles * sizeof(Vector3)));
    d_k4v = static_cast<Vector3*>(grav_x::CudaMemoryPool::allocate(numParticles * sizeof(Vector3)));

    if (!d_stage || !d_k1x || !d_k2x || !d_k3x || !d_k4x || !d_k1v || !d_k2v || !d_k3v || !d_k4v) {
        releaseRk4Buffers();
        return false;
    }
    return true;
}

void ParticleSystem::releaseRk4Buffers()
{
    if (d_stage) { grav_x::CudaMemoryPool::deallocate(d_stage); d_stage = nullptr; }
    if (d_k1x) { grav_x::CudaMemoryPool::deallocate(d_k1x); d_k1x = nullptr; }
    if (d_k2x) { grav_x::CudaMemoryPool::deallocate(d_k2x); d_k2x = nullptr; }
    if (d_k3x) { grav_x::CudaMemoryPool::deallocate(d_k3x); d_k3x = nullptr; }
    if (d_k4x) { grav_x::CudaMemoryPool::deallocate(d_k4x); d_k4x = nullptr; }
    if (d_k1v) { grav_x::CudaMemoryPool::deallocate(d_k1v); d_k1v = nullptr; }
    if (d_k2v) { grav_x::CudaMemoryPool::deallocate(d_k2v); d_k2v = nullptr; }
    if (d_k3v) { grav_x::CudaMemoryPool::deallocate(d_k3v); d_k3v = nullptr; }
    if (d_k4v) { grav_x::CudaMemoryPool::deallocate(d_k4v); d_k4v = nullptr; }
}

bool ParticleSystem::allocateSphBuffers(int numParticles)
{
    releaseSphBuffers();
    d_sphDensity = static_cast<float*>(grav_x::CudaMemoryPool::allocate(numParticles * sizeof(float)));
    d_sphPressure = static_cast<float*>(grav_x::CudaMemoryPool::allocate(numParticles * sizeof(float)));

    if (!d_sphDensity || !d_sphPressure) {
        releaseSphBuffers();
        return false;
    }
    return true;
}

void ParticleSystem::releaseSphBuffers()
{
    if (d_sphDensity) { grav_x::CudaMemoryPool::deallocate(d_sphDensity); d_sphDensity = nullptr; }
    if (d_sphPressure) { grav_x::CudaMemoryPool::deallocate(d_sphPressure); d_sphPressure = nullptr; }
}

bool ParticleSystem::allocateSphGridBuffers(int numParticles)
{
    releaseSphGridBuffers();
    const std::size_t particleBytes = static_cast<std::size_t>(numParticles) * sizeof(int);
    d_sphCellHash = static_cast<int*>(grav_x::CudaMemoryPool::allocate(particleBytes));
    d_sphSortedIndex = static_cast<int*>(grav_x::CudaMemoryPool::allocate(particleBytes));

    if (!d_sphCellHash || !d_sphSortedIndex) {
        releaseSphGridBuffers();
        return false;
    }
    _hostCellHash.resize(numParticles);
    _hostSortedIndex.resize(numParticles);
    return true;
}

void ParticleSystem::releaseSphGridBuffers()
{
    if (d_sphCellHash) { grav_x::CudaMemoryPool::deallocate(d_sphCellHash); d_sphCellHash = nullptr; }
    if (d_sphSortedIndex) { grav_x::CudaMemoryPool::deallocate(d_sphSortedIndex); d_sphSortedIndex = nullptr; }
    if (d_sphCellStart) { grav_x::CudaMemoryPool::deallocate(d_sphCellStart); d_sphCellStart = nullptr; }
    if (d_sphCellEnd) { grav_x::CudaMemoryPool::deallocate(d_sphCellEnd); d_sphCellEnd = nullptr; }
    _hostCellHash.clear();
    _hostSortedIndex.clear();
}

bool ParticleSystem::seedDeviceState()
{
    if (_particles.empty()) return true;
    syncDeviceState();
    return true;
}
