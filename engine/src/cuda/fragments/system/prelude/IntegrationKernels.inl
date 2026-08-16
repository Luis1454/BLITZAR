/*
 * @file engine/src/cuda/fragments/system/prelude/IntegrationKernels.inl
 * @project BLITZAR
 * @brief Shared CUDA system helper or kernel implementation fragment.
 */

__global__ void updateParticles(ParticleSoAView last, ParticleSoAView current, int numParticles,
                                float deltaTime, ForceLawPolicy policy, float maxAcceleration)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < numParticles) {
        const Vector3 pos = getSoAPosition(last, i);
        const Vector3 vel = getSoAVelocity(last, i);
        const Vector3 force =
            computePairwiseAcceleration(last, numParticles, i, policy, maxAcceleration);

        const Vector3 nextVel = vel + force * deltaTime;
        const Vector3 nextPos = pos + nextVel * deltaTime;

        setSoAPressure(current, i, force * 100.0f);
        setSoAVelocity(current, i, nextVel);
        setSoAPosition(current, i, nextPos);
        current.mass[i] = last.mass[i];
        current.temp[i] = last.temp[i];
        current.dens[i] = last.dens[i];
    }
}

__global__ void updateParticlesWithAcceleration(ParticleSoAView last, ParticleSoAView current,
                                                const Vector3* acceleration, int numParticles,
                                                float deltaTime)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }
    const Vector3 pos = getSoAPosition(last, i);
    const Vector3 vel = getSoAVelocity(last, i);
    const Vector3 force = acceleration[i];
    const Vector3 nextVel = vel + force * deltaTime;
    const Vector3 nextPos = pos + nextVel * deltaTime;
    setSoAPressure(current, i, force * 100.0f);
    setSoAVelocity(current, i, nextVel);
    setSoAPosition(current, i, nextPos);
    current.mass[i] = last.mass[i];
    current.temp[i] = last.temp[i];
    current.dens[i] = last.dens[i];
}

__global__ void applyCosmologyExpansionKernel(ParticleSoAView particles, int numParticles,
                                              float scaleRatio, float previousHubble,
                                              float nextHubble)
{
    (void)previousHubble;
    (void)nextHubble;
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }
    const Vector3 position = getSoAPosition(particles, i);
    const Vector3 nextPosition = position * scaleRatio;
    const Vector3 nextVelocity = getSoAVelocity(particles, i) / max(scaleRatio, 1.0e-6f);
    setSoAPosition(particles, i, nextPosition);
    setSoAVelocity(particles, i, nextVelocity);
}

/*
 * @brief Documents the compute sph density pressure kernel operation contract.
 * @param particles Input value used by this contract.
 * @param outDensity Input value used by this contract.
 * @param outPressure Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @param smoothingLength Input value used by this contract.
 * @param restDensity Input value used by this contract.
 * @param gasConstant Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void computeSphDensityPressureKernel(ParticleSoAView particles, FloatHandle outDensity,
                                                FloatHandle outPressure, int numParticles,
                                                float smoothingLength, float restDensity,
                                                float gasConstant)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }

    if (particles.mass[i] > 0.1f) {
        outDensity[i] = restDensity;
        outPressure[i] = 0.0f;
        return;
    }

    const Vector3 pi = getSoAPosition(particles, i);
    float density = 0.0f;
    for (int j = 0; j < numParticles; ++j) {
        if (particles.mass[j] > 0.1f) {
            continue;
        }
        const Vector3 pj = getSoAPosition(particles, j);
        const Vector3 d = pi - pj;
        const float r2 = dot(d, d);
        density += particles.mass[j] * sphPoly6(r2, smoothingLength);
    }
    density = fmaxf(density, restDensity * 0.05f);
    outDensity[i] = density;
    const float pressure = gasConstant * fmaxf(density - restDensity, 0.0f);
    outPressure[i] = fminf(pressure, gasConstant * restDensity * 20.0f);
}

__global__ void integrateSphKernel(ParticleSoAView inParticles, ParticleSoAView outParticles,
                                   ConstFloatHandle density, ConstFloatHandle pressure,
                                   int numParticles, float smoothingLength, float viscosity,
                                   float deltaTime, float correctionScale,
                                   IndexConstHandle cellHash, IndexConstHandle sortedIndex,
                                   IndexConstHandle cellStart, IndexConstHandle cellEnd,
                                   SphGridParams grid, float maxAcceleration, float maxSpeed)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }

    const float selfMass = inParticles.mass[i];
    if (selfMass > 0.1f) {
        outParticles.posX[i] = inParticles.posX[i];
        outParticles.posY[i] = inParticles.posY[i];
        outParticles.posZ[i] = inParticles.posZ[i];
        outParticles.velX[i] = inParticles.velX[i];
        outParticles.velY[i] = inParticles.velY[i];
        outParticles.velZ[i] = inParticles.velZ[i];
        outParticles.dens[i] = inParticles.dens[i];
        setSoAPressure(outParticles, i, getSoAPressure(inParticles, i));
        outParticles.mass[i] = inParticles.mass[i];
        return;
    }

    const Vector3 pi = getSoAPosition(inParticles, i);
    const Vector3 vi = getSoAVelocity(inParticles, i);
    const float rhoI = fmaxf(density[i], 1e-6f);
    const float pI = pressure[i];

    Vector3 pressureForce(0.0f, 0.0f, 0.0f);
    Vector3 viscosityForce(0.0f, 0.0f, 0.0f);

    for (int j = 0; j < numParticles; ++j) {
        if (j == i) {
            continue;
        }
        if (inParticles.mass[j] > 0.1f) {
            continue;
        }
        const Vector3 pj = getSoAPosition(inParticles, j);
        const Vector3 rij = pi - pj;
        const float r = sqrtf(dot(rij, rij));
        if (r >= smoothingLength || r <= 1e-6f) {
            continue;
        }

        const float rhoJ = fmaxf(density[j], 1e-6f);
        const float pJ = pressure[j];
        const Vector3 gradDir = rij / r;
        const float grad = sphSpikyGrad(r, smoothingLength);
        pressureForce += gradDir * (-inParticles.mass[j] * (pI + pJ) * 0.5f / rhoJ * grad);

        const float lap = sphViscosityLaplacian(r, smoothingLength);
        viscosityForce +=
            (getSoAVelocity(inParticles, j) - vi) * (viscosity * inParticles.mass[j] / rhoJ * lap);
    }

    const Vector3 totalForce = pressureForce + viscosityForce;
    Vector3 acceleration = totalForce / rhoI;

    const float accelNorm = acceleration.norm();
    if (accelNorm > maxAcceleration) {
        acceleration = acceleration * (maxAcceleration / accelNorm);
    }

    Vector3 velocity = vi + acceleration * (deltaTime * correctionScale);
    const float speed = velocity.norm();
    if (speed > maxSpeed) {
        velocity = velocity * (maxSpeed / speed);
    }
    Vector3 position = pi + velocity * (deltaTime * correctionScale);

    outParticles.velX[i] = velocity.x;
    outParticles.velY[i] = velocity.y;
    outParticles.velZ[i] = velocity.z;
    outParticles.posX[i] = position.x;
    outParticles.posY[i] = position.y;
    outParticles.posZ[i] = position.z;
    outParticles.dens[i] = rhoI;
    setSoAPressure(outParticles, i, (pressureForce + viscosityForce) * 2.0f);
    outParticles.mass[i] = selfMass;
}

/*
 * @brief Documents the copy particles kernel operation contract.
 * @param src Input value used by this contract.
 * @param dst Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
