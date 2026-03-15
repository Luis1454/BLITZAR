// SPH simulation kernels using spatial hash grid acceleration.
// These kernels iterate over neighboring cells instead of the full particle list.

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
    SphGridParams grid,
    float maxAcceleration,
    float maxSpeed)
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
    if (accelNorm > maxAcceleration) {
        acceleration = acceleration * (maxAcceleration / accelNorm);
    }

    Particle updated = self;
    Vector3 velocity = vi
        + acceleration * (deltaTime * correctionScale);
    const float speed = velocity.norm();
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
