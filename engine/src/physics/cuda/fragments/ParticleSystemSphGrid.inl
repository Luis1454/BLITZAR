// Spatial hash grid for SPH neighbor search acceleration.
// Grid cell size = smoothingLength; particles hashed into 3D uniform grid.
// Neighbor queries iterate 27 adjacent cells instead of all particles.

struct SphGridParams {
    int gridSize;
    int totalCells;
    float cellSize;
    float originX;
    float originY;
    float originZ;
};

__device__ int sphGridCellIndex(
    float px, float py, float pz,
    const SphGridParams &grid)
{
    int cx = static_cast<int>(floorf((px - grid.originX) / grid.cellSize));
    int cy = static_cast<int>(floorf((py - grid.originY) / grid.cellSize));
    int cz = static_cast<int>(floorf((pz - grid.originZ) / grid.cellSize));
    cx = max(0, min(cx, grid.gridSize - 1));
    cy = max(0, min(cy, grid.gridSize - 1));
    cz = max(0, min(cz, grid.gridSize - 1));
    return cx + cy * grid.gridSize + cz * grid.gridSize * grid.gridSize;
}

__global__ void computeSphCellHashKernel(
    ParticleConstHandle particles,
    IndexHandle outCellHash,
    IndexHandle outParticleIndex,
    int numParticles,
    SphGridParams grid)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }
    const Vector3 pos = particles[i].getPosition();
    outCellHash[i] = sphGridCellIndex(pos.x, pos.y, pos.z, grid);
    outParticleIndex[i] = i;
}

__global__ void resetCellBoundsKernel(
    IndexHandle cellStart,
    IndexHandle cellEnd,
    int totalCells)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= totalCells) {
        return;
    }
    cellStart[i] = -1;
    cellEnd[i] = -1;
}

__global__ void findCellBoundsKernel(
    IndexConstHandle sortedHash,
    IndexHandle cellStart,
    IndexHandle cellEnd,
    int numParticles)
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

// Grid-accelerated SPH density/pressure kernel — iterates 27 neighbor cells.
__global__ void computeSphDensityPressureGridKernel(
    ParticleConstHandle particles,
    FloatHandle outDensity,
    FloatHandle outPressure,
    int numParticles,
    float smoothingLength,
    float restDensity,
    float gasConstant,
    IndexConstHandle sortedHash,
    IndexConstHandle sortedIndex,
    IndexConstHandle cellStart,
    IndexConstHandle cellEnd,
    SphGridParams grid)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }

    if (particles[i].getMass() > 0.1f) {
        outDensity[i] = restDensity;
        outPressure[i] = 0.0f;
        return;
    }

    const Vector3 pi = particles[i].getPosition();
    int cx = static_cast<int>(floorf((pi.x - grid.originX) / grid.cellSize));
    int cy = static_cast<int>(floorf((pi.y - grid.originY) / grid.cellSize));
    int cz = static_cast<int>(floorf((pi.z - grid.originZ) / grid.cellSize));
    cx = max(0, min(cx, grid.gridSize - 1));
    cy = max(0, min(cy, grid.gridSize - 1));
    cz = max(0, min(cz, grid.gridSize - 1));

    float density = 0.0f;
    for (int dz = -1; dz <= 1; ++dz) {
        int nz = cz + dz;
        if (nz < 0 || nz >= grid.gridSize) {
            continue;
        }
        for (int dy = -1; dy <= 1; ++dy) {
            int ny = cy + dy;
            if (ny < 0 || ny >= grid.gridSize) {
                continue;
            }
            for (int dx = -1; dx <= 1; ++dx) {
                int nx = cx + dx;
                if (nx < 0 || nx >= grid.gridSize) {
                    continue;
                }
                int cellIdx = nx + ny * grid.gridSize
                    + nz * grid.gridSize * grid.gridSize;
                int start = cellStart[cellIdx];
                if (start < 0) {
                    continue;
                }
                int end = cellEnd[cellIdx];
                for (int s = start; s < end; ++s) {
                    int j = sortedIndex[s];
                    const Particle pj = particles[j];
                    if (pj.getMass() > 0.1f) {
                        continue;
                    }
                    const Vector3 d = pi - pj.getPosition();
                    const float r2 = dot(d, d);
                    density += pj.getMass()
                        * sphPoly6(r2, smoothingLength);
                }
            }
        }
    }
    density = fmaxf(density, restDensity * 0.05f);
    outDensity[i] = density;
    const float pVal = gasConstant
        * fmaxf(density - restDensity, 0.0f);
    outPressure[i] = fminf(pVal,
        gasConstant * restDensity * 20.0f);
}

// Grid-accelerated SPH integration kernel — iterates 27 neighbor cells.
__global__ void integrateSphGridKernel(
    ParticleHandle particles,
    ConstFloatHandle density,
    ConstFloatHandle pressure,
    int numParticles,
    float smoothingLength,
    float viscosity,
    float deltaTime,
    float correctionScale,
    IndexConstHandle sortedHash,
    IndexConstHandle sortedIndex,
    IndexConstHandle cellStart,
    IndexConstHandle cellEnd,
    SphGridParams grid)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }

    const Particle self = particles[i];
    const Vector3 pi = self.getPosition();
    const Vector3 vi = self.getVelocity();
    const float selfMass = self.getMass();
    if (selfMass > 0.1f) {
        return;
    }

    const float rhoI = fmaxf(density[i], 1e-6f);
    const float pI = pressure[i];

    int cx = static_cast<int>(floorf(
        (pi.x - grid.originX) / grid.cellSize));
    int cy = static_cast<int>(floorf(
        (pi.y - grid.originY) / grid.cellSize));
    int cz = static_cast<int>(floorf(
        (pi.z - grid.originZ) / grid.cellSize));
    cx = max(0, min(cx, grid.gridSize - 1));
    cy = max(0, min(cy, grid.gridSize - 1));
    cz = max(0, min(cz, grid.gridSize - 1));

    Vector3 pressureForce(0.0f, 0.0f, 0.0f);
    Vector3 viscosityForce(0.0f, 0.0f, 0.0f);

    for (int dz = -1; dz <= 1; ++dz) {
        int nz = cz + dz;
        if (nz < 0 || nz >= grid.gridSize) {
            continue;
        }
        for (int dy = -1; dy <= 1; ++dy) {
            int ny = cy + dy;
            if (ny < 0 || ny >= grid.gridSize) {
                continue;
            }
            for (int dx = -1; dx <= 1; ++dx) {
                int nx = cx + dx;
                if (nx < 0 || nx >= grid.gridSize) {
                    continue;
                }
                int cellIdx = nx + ny * grid.gridSize
                    + nz * grid.gridSize * grid.gridSize;
                int start = cellStart[cellIdx];
                if (start < 0) {
                    continue;
                }
                int end = cellEnd[cellIdx];
                for (int s = start; s < end; ++s) {
                    int j = sortedIndex[s];
                    if (j == i) {
                        continue;
                    }
                    const Particle other = particles[j];
                    if (other.getMass() > 0.1f) {
                        continue;
                    }
                    const Vector3 rij = pi - other.getPosition();
                    const float r = sqrtf(dot(rij, rij));
                    if (r >= smoothingLength || r <= 1e-6f) {
                        continue;
                    }
                    const float rhoJ = fmaxf(
                        density[j], 1e-6f);
                    const float pJ = pressure[j];
                    const Vector3 gradDir = rij / r;
                    const float grad = sphSpikyGrad(
                        r, smoothingLength);
                    pressureForce += gradDir
                        * (-other.getMass() * (pI + pJ)
                           * 0.5f / rhoJ * grad);
                    const float lap = sphViscosityLaplacian(
                        r, smoothingLength);
                    viscosityForce += (other.getVelocity() - vi)
                        * (viscosity * other.getMass()
                           / rhoJ * lap);
                }
            }
        }
    }

    const Vector3 totalForce = pressureForce + viscosityForce;
    Vector3 acceleration = totalForce / rhoI;

    const float accelNorm = acceleration.norm();
    const float maxAccel = 40.0f;
    if (accelNorm > maxAccel) {
        acceleration = acceleration * (maxAccel / accelNorm);
    }

    Particle updated = self;
    Vector3 velocity = vi
        + acceleration * (deltaTime * correctionScale);
    const float speed = velocity.norm();
    const float maxSpeed = 120.0f;
    if (speed > maxSpeed) {
        velocity = velocity * (maxSpeed / speed);
    }
    Vector3 position = pi
        + velocity * (deltaTime * correctionScale);

    updated.setVelocity(velocity);
    updated.setPosition(position);
    updated.setDensity(rhoI);
    updated.setPressure(
        (pressureForce + viscosityForce) * 2.0f);
    particles[i] = updated;
}

// CPU-side comparison sort for cell hashes (simple insertion sort for GPU-built arrays).
// Used host-side after downloading hash array — avoids needing thrust.
static void sortParticlesByHash(
    int *cellHash, int *particleIndex, int numParticles)
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
        tempHash[static_cast<std::size_t>(i)] =
            cellHash[order[static_cast<std::size_t>(i)]];
        tempIdx[static_cast<std::size_t>(i)] =
            particleIndex[order[static_cast<std::size_t>(i)]];
    }
    std::memcpy(cellHash, tempHash.data(),
        static_cast<std::size_t>(numParticles) * sizeof(int));
    std::memcpy(particleIndex, tempIdx.data(),
        static_cast<std::size_t>(numParticles) * sizeof(int));
}

bool ParticleSystem::buildSphGrid(int numParticles)
{
    if (numParticles <= 0 || !d_particles || !d_sphCellHash || !d_sphSortedIndex) {
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
        if (p.x < minX) { minX = p.x; }
        if (p.y < minY) { minY = p.y; }
        if (p.z < minZ) { minZ = p.z; }
        if (p.x > maxX) { maxX = p.x; }
        if (p.y > maxY) { maxY = p.y; }
        if (p.z > maxZ) { maxZ = p.z; }
    }
    const float cellSize = std::max(0.01f, _sphSmoothingLength);
    const float extent = std::max(
        {maxX - minX, maxY - minY, maxZ - minZ, cellSize});
    const int gridSize = std::min(256,
        std::max(1, static_cast<int>(std::ceil(extent / cellSize)) + 2));
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
    const std::size_t cellBytes =
        static_cast<std::size_t>(totalCells) * sizeof(int);
    if (d_sphCellStart) {
        cudaFree(d_sphCellStart);
        d_sphCellStart = nullptr;
    }
    if (d_sphCellEnd) {
        cudaFree(d_sphCellEnd);
        d_sphCellEnd = nullptr;
    }
    if (!checkCudaStatus(cudaMalloc(&d_sphCellStart, cellBytes),
            "cudaMalloc(d_sphCellStart)")) {
        return false;
    }
    if (!checkCudaStatus(cudaMalloc(&d_sphCellEnd, cellBytes),
            "cudaMalloc(d_sphCellEnd)")) {
        return false;
    }

    // 1) Hash particles into cells on GPU.
    const int numBlocks =
        (numParticles + Particle::kDefaultCudaBlockSize - 1)
        / Particle::kDefaultCudaBlockSize;
    computeSphCellHashKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
        d_particles, d_sphCellHash, d_sphSortedIndex,
        numParticles, grid);
    if (!checkCudaStatus(cudaDeviceSynchronize(),
            "sph hash kernel sync")) {
        return false;
    }

    // 2) Download, sort on CPU, upload (avoids thrust).
    const std::size_t pBytes =
        static_cast<std::size_t>(numParticles) * sizeof(int);
    if (!checkCudaStatus(cudaMemcpy(_hostCellHash.data(),
            d_sphCellHash, pBytes, cudaMemcpyDeviceToHost),
            "sph hash D2H")) {
        return false;
    }
    if (!checkCudaStatus(cudaMemcpy(_hostSortedIndex.data(),
            d_sphSortedIndex, pBytes, cudaMemcpyDeviceToHost),
            "sph idx D2H")) {
        return false;
    }
    sortParticlesByHash(
        _hostCellHash.data(), _hostSortedIndex.data(), numParticles);
    if (!checkCudaStatus(cudaMemcpy(d_sphCellHash,
            _hostCellHash.data(), pBytes, cudaMemcpyHostToDevice),
            "sph hash H2D")) {
        return false;
    }
    if (!checkCudaStatus(cudaMemcpy(d_sphSortedIndex,
            _hostSortedIndex.data(), pBytes, cudaMemcpyHostToDevice),
            "sph idx H2D")) {
        return false;
    }

    // 3) Reset and build cell boundaries on GPU.
    const int cellBlocks = (totalCells + 255) / 256;
    resetCellBoundsKernel<<<cellBlocks, 256>>>(
        d_sphCellStart, d_sphCellEnd, totalCells);
    findCellBoundsKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
        d_sphCellHash, d_sphCellStart, d_sphCellEnd, numParticles);
    if (!checkCudaStatus(cudaDeviceSynchronize(),
            "sph grid build sync")) {
        return false;
    }
    return true;
}
