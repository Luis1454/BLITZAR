/*
 * @file engine/physics/core/cuda/prelude/CudParticleKernels.inl
 * @project BLITZAR
 * @brief Shared CUDA system helper or kernel implementation fragment.
 */

__global__ void copyParticlesKernel(ParticleConstHandle src, ParticleHandle dst, int numParticles)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }
    dst[i] = src[i];
}

/*
 * @brief Documents the extract velocity kernel operation contract.
 * @param particles Input value used by this contract.
 * @param outVelocity Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void extractVelocityKernel(ParticleSoAView particles, Vector3Handle outVelocity,
                                      int numParticles)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }
    outVelocity[i] = getSoAVelocity(particles, i);
}

/*
 * @brief Documents the compute pairwise acceleration kernel operation contract.
 * @param state Input value used by this contract.
 * @param outAcceleration Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @param policy Input value used by this contract.
 * @param maxAcceleration Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void computePairwiseAccelerationKernel(ParticleSoAView state,
                                                  Vector3Handle outAcceleration, int numParticles,
                                                  ForceLawPolicy policy, float maxAcceleration)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }
    outAcceleration[i] =
        computePairwiseAcceleration(state, numParticles, i, policy, maxAcceleration);
}

/*
 * @brief Documents the build rk4 stage kernel operation contract.
 * @param base Input value used by this contract.
 * @param kPos Input value used by this contract.
 * @param kVel Input value used by this contract.
 * @param dtScale Input value used by this contract.
 * @param stage Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void buildRk4StageKernel(ParticleSoAView base, Vector3ConstHandle kPos,
                                    Vector3ConstHandle kVel, float dtScale, ParticleSoAView stage,
                                    int numParticles)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }

    const Vector3 nextPos = getSoAPosition(base, i) + kPos[i] * dtScale;
    const Vector3 nextVel = getSoAVelocity(base, i) + kVel[i] * dtScale;

    setSoAPosition(stage, i, nextPos);
    setSoAVelocity(stage, i, nextVel);
    stage.mass[i] = base.mass[i];
    stage.temp[i] = base.temp[i];
    stage.dens[i] = base.dens[i];
}

/*
 * @brief Documents the prime half velocity kernel operation contract.
 * @param state Input value used by this contract.
 * @param vHalf Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void primeHalfVelocityKernel(ParticleSoAView state, float3* vHalf, int numParticles)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }

    __shared__ float3 sVel[Particle::kDefaultCudaBlockSize];
    sVel[threadIdx.x] = make_float3(state.velX[i], state.velY[i], state.velZ[i]);
    __syncthreads();

    vHalf[i] = sVel[threadIdx.x];
}

/*
 * @brief Documents the apply kick half step kernel operation contract.
 * @param state Input value used by this contract.
 * @param acceleration Input value used by this contract.
 * @param deltaTime Input value used by this contract.
 * @param vHalf Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void applyKickHalfStepKernel(ParticleSoAView state, Vector3ConstHandle acceleration,
                                        float deltaTime, float3* vHalf, int numParticles)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }

    __shared__ float3 sVel[Particle::kDefaultCudaBlockSize];
    __shared__ float3 sAcc[Particle::kDefaultCudaBlockSize];

    sVel[threadIdx.x] = make_float3(state.velX[i], state.velY[i], state.velZ[i]);
    sAcc[threadIdx.x] = make_float3(acceleration[i].x, acceleration[i].y, acceleration[i].z);
    __syncthreads();

    const float halfDt = 0.5f * deltaTime;
    const float3 vel = sVel[threadIdx.x];
    const float3 acc = sAcc[threadIdx.x];
    vHalf[i] = make_float3(vel.x + acc.x * halfDt, vel.y + acc.y * halfDt, vel.z + acc.z * halfDt);
}

/*
 * @brief Documents the drift with half velocity kernel operation contract.
 * @param state Input value used by this contract.
 * @param vHalf Input value used by this contract.
 * @param deltaTime Input value used by this contract.
 * @param out Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void driftWithHalfVelocityKernel(ParticleSoAView state, const float3* vHalf,
                                            float deltaTime, ParticleSoAView out, int numParticles)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }

    __shared__ float3 sPos[Particle::kDefaultCudaBlockSize];
    __shared__ float3 sHalf[Particle::kDefaultCudaBlockSize];

    sPos[threadIdx.x] = make_float3(state.posX[i], state.posY[i], state.posZ[i]);
    sHalf[threadIdx.x] = vHalf[i];
    __syncthreads();

    const float3 pos = sPos[threadIdx.x];
    const float3 halfVel = sHalf[threadIdx.x];
    const Vector3 nextPos(pos.x + halfVel.x * deltaTime, pos.y + halfVel.y * deltaTime,
                          pos.z + halfVel.z * deltaTime);

    setSoAPosition(out, i, nextPos);
    setSoAVelocity(out, i, getSoAVelocity(state, i));
    out.mass[i] = state.mass[i];
    out.temp[i] = state.temp[i];
    out.dens[i] = state.dens[i];
}

/*
 * @brief Documents the finalize leapfrog kick kernel operation contract.
 * @param driftedState Input value used by this contract.
 * @param vHalf Input value used by this contract.
 * @param acceleration Input value used by this contract.
 * @param deltaTime Input value used by this contract.
 * @param out Input value used by this contract.
 * @param vHalfOut Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void finalizeLeapfrogKickKernel(ParticleSoAView driftedState, const float3* vHalf,
                                           Vector3ConstHandle acceleration, float deltaTime,
                                           ParticleSoAView out, float3* vHalfOut, int numParticles)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }

    __shared__ float3 sHalf[Particle::kDefaultCudaBlockSize];
    __shared__ float3 sAcc[Particle::kDefaultCudaBlockSize];

    sHalf[threadIdx.x] = vHalf[i];
    sAcc[threadIdx.x] = make_float3(acceleration[i].x, acceleration[i].y, acceleration[i].z);
    __syncthreads();

    const float3 halfVel = sHalf[threadIdx.x];
    const float3 acc = sAcc[threadIdx.x];
    const float halfDt = 0.5f * deltaTime;

    const Vector3 nextVel(halfVel.x + acc.x * halfDt, halfVel.y + acc.y * halfDt,
                          halfVel.z + acc.z * halfDt);

    setSoAPosition(out, i, getSoAPosition(driftedState, i));
    setSoAVelocity(out, i, nextVel);
    setSoAPressure(out, i, Vector3(acc.x, acc.y, acc.z) * 100.0f);
    out.mass[i] = driftedState.mass[i];
    out.temp[i] = driftedState.temp[i];
    out.dens[i] = driftedState.dens[i];

    vHalfOut[i] = make_float3(halfVel.x + acc.x * deltaTime, halfVel.y + acc.y * deltaTime,
                              halfVel.z + acc.z * deltaTime);
}
