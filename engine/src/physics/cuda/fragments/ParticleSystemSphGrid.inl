/*
 * @file engine/src/physics/cuda/fragments/ParticleSystemSphGrid.inl
 * @author Luis1454
 * @project BLITZAR
 * @brief Physics and CUDA implementation for the deterministic simulation core.
 */

/*
 * Module: physics/cuda
 * Responsibility: Build and maintain the SPH spatial hash grid.
 */

// Spatial hash grid for SPH neighbor search acceleration.
// Grid cell size = smoothingLength; particles hashed into 3D uniform grid.
// Neighbor queries iterate 27 adjacent cells instead of all particles.

// Definition moved to ParticleSystemPrelude.inl

/*
 * @brief Documents the sph grid cell index operation contract.
 * @param px Input value used by this contract.
 * @param py Input value used by this contract.
 * @param pz Input value used by this contract.
 * @param grid Input value used by this contract.
 * @return int value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__device__ int sphGridCellIndex(float px, float py, float pz, const SphGridParams& grid)
{
    int cx = static_cast<int>(floorf((px - grid.originX) / grid.cellSize));
    int cy = static_cast<int>(floorf((py - grid.originY) / grid.cellSize));
    int cz = static_cast<int>(floorf((pz - grid.originZ) / grid.cellSize));
    cx = max(0, min(cx, grid.gridSize - 1));
    cy = max(0, min(cy, grid.gridSize - 1));
    cz = max(0, min(cz, grid.gridSize - 1));
    return cx + cy * grid.gridSize + cz * grid.gridSize * grid.gridSize;
}

/*
 * @brief Documents the compute sph cell hash kernel operation contract.
 * @param particles Input value used by this contract.
 * @param outCellHash Input value used by this contract.
 * @param outParticleIndex Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @param grid Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void computeSphCellHashKernel(ParticleSoAView particles, IndexHandle outCellHash,
                                         IndexHandle outParticleIndex, int numParticles,
                                         SphGridParams grid)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }
    const Vector3 pos = getSoAPosition(particles, i);
    outCellHash[i] = sphGridCellIndex(pos.x, pos.y, pos.z, grid);
    outParticleIndex[i] = i;
}

/*
 * @brief Documents the reset cell bounds kernel operation contract.
 * @param cellStart Input value used by this contract.
 * @param cellEnd Input value used by this contract.
 * @param totalCells Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void resetCellBoundsKernel(IndexHandle cellStart, IndexHandle cellEnd, int totalCells)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= totalCells) {
        return;
    }
    cellStart[i] = -1;
    cellEnd[i] = -1;
}

/*
 * @brief Documents the find cell bounds kernel operation contract.
 * @param sortedHash Input value used by this contract.
 * @param cellStart Input value used by this contract.
 * @param cellEnd Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void findCellBoundsKernel(IndexConstHandle sortedHash, IndexHandle cellStart,
                                     IndexHandle cellEnd, int numParticles)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }
    const int hash = sortedHash[i];
    if (i == 0 || hash != sortedHash[i - 1]) {
        cellStart[hash] = i;
    }
    if (i == numParticles - 1 || hash != sortedHash[i + 1]) {
        cellEnd[hash] = i + 1;
    }
}

// Sort logic and grid building methods moved below.

// CPU-side comparison sort for cell hashes (simple insertion sort for GPU-built arrays).
// Used host-side after downloading hash array — avoids needing thrust.
/*
 * @brief Documents the sort particles by hash operation contract.
 * @param cellHash Input value used by this contract.
 * @param particleIndex Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static void sortParticlesByHash(int* cellHash, int* particleIndex, int numParticles)
{
    // Simple O(n log n) sort using index array.
    std::vector<int> order(static_cast<std::size_t>(numParticles));
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        return cellHash[a] < cellHash[b];
    });
    std::vector<int> tempHash(static_cast<std::size_t>(numParticles));
    std::vector<int> tempIdx(static_cast<std::size_t>(numParticles));
    for (int i = 0; i < numParticles; ++i) {
        tempHash[static_cast<std::size_t>(i)] = cellHash[order[static_cast<std::size_t>(i)]];
        tempIdx[static_cast<std::size_t>(i)] = particleIndex[order[static_cast<std::size_t>(i)]];
    }
    std::memcpy(cellHash, tempHash.data(), static_cast<std::size_t>(numParticles) * sizeof(int));
    std::memcpy(particleIndex, tempIdx.data(),
                static_cast<std::size_t>(numParticles) * sizeof(int));
}

/*
 * @brief Documents the build sph grid operation contract.
 * @param numParticles Input value used by this contract.
 * @return bool ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool ParticleSystem::buildSphGrid(int numParticles)
{
    if (numParticles <= 0 || !d_soaPosX || !d_sphCellHash || !d_sphSortedIndex) {
        return false;
    }

    // Compute bounding box on host particles (already synced).
    float minX = _particles[0].getPosition().x;
    float minY = _particles[0].getPosition().y;
    float minZ = _particles[0].getPosition().z;
    float maxX = minX;
    float maxY = minY;
    float maxZ = minZ;
    for (int i = 1; i < numParticles; ++i) {
        const Vector3 p = _particles[static_cast<std::size_t>(i)].getPosition();
        if (p.x < minX) {
            minX = p.x;
        }
        if (p.y < minY) {
            minY = p.y;
        }
        if (p.z < minZ) {
            minZ = p.z;
        }
        if (p.x > maxX) {
            maxX = p.x;
        }
        if (p.y > maxY) {
            maxY = p.y;
        }
        if (p.z > maxZ) {
            maxZ = p.z;
        }
    }
    const float cellSize = std::max(0.01f, _sphSmoothingLength);
    const float extent = std::max({maxX - minX, maxY - minY, maxZ - minZ, cellSize});
    const int gridSize =
        std::min(256, std::max(1, static_cast<int>(std::ceil(extent / cellSize)) + 2));
    const int totalCells = gridSize * gridSize * gridSize;
    _sphGridSize = gridSize;
    _sphGridTotalCells = totalCells;

    SphGridParams grid;
    grid.gridSize = gridSize;
    grid.totalCells = totalCells;
    grid.cellSize = cellSize;
    grid.originX = minX - cellSize;
    grid.originY = minY - cellSize;
    grid.originZ = minZ - cellSize;

    // (Re)allocate cell start/end arrays.
    const std::size_t cellBytes = static_cast<std::size_t>(totalCells) * sizeof(int);
    if (d_sphCellStart) {
        bltzr_x::CudaMemoryPool::deallocate(d_sphCellStart);
        d_sphCellStart = nullptr;
    }
    if (d_sphCellEnd) {
        bltzr_x::CudaMemoryPool::deallocate(d_sphCellEnd);
        d_sphCellEnd = nullptr;
    }
    d_sphCellStart = static_cast<int*>(bltzr_x::CudaMemoryPool::allocate(cellBytes));
    d_sphCellEnd = static_cast<int*>(bltzr_x::CudaMemoryPool::allocate(cellBytes));

    if (!d_sphCellStart || !d_sphCellEnd) {
        return false;
    }

    // 1) Hash particles into cells on GPU.
    const int numBlocks =
        (numParticles + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    computeSphCellHashKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
        getSoAView(false), d_sphCellHash, d_sphSortedIndex, numParticles, grid);
    if (!checkCudaStatus(cudaDeviceSynchronize(), "sph hash kernel sync")) {
        return false;
    }

    // 2) Download, sort on CPU, upload (avoids thrust).
    const std::size_t pBytes = static_cast<std::size_t>(numParticles) * sizeof(int);
    if (!checkCudaStatus(
            cudaMemcpy(_hostCellHash.data(), d_sphCellHash, pBytes, cudaMemcpyDeviceToHost),
            "sph hash D2H")) {
        return false;
    }
    if (!checkCudaStatus(
            cudaMemcpy(_hostSortedIndex.data(), d_sphSortedIndex, pBytes, cudaMemcpyDeviceToHost),
            "sph idx D2H")) {
        return false;
    }
    sortParticlesByHash(_hostCellHash.data(), _hostSortedIndex.data(), numParticles);
    if (!checkCudaStatus(
            cudaMemcpy(d_sphCellHash, _hostCellHash.data(), pBytes, cudaMemcpyHostToDevice),
            "sph hash H2D")) {
        return false;
    }
    if (!checkCudaStatus(
            cudaMemcpy(d_sphSortedIndex, _hostSortedIndex.data(), pBytes, cudaMemcpyHostToDevice),
            "sph idx H2D")) {
        return false;
    }

    // 3) Reset and build cell boundaries on GPU.
    const int cellBlocks = (totalCells + 255) / 256;
    resetCellBoundsKernel<<<cellBlocks, 256>>>(d_sphCellStart, d_sphCellEnd, totalCells);
    findCellBoundsKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
        d_sphCellHash, d_sphCellStart, d_sphCellEnd, numParticles);
    if (!checkCudaStatus(cudaDeviceSynchronize(), "sph grid build sync")) {
        return false;
    }
    return true;
}
