/*
 * @file engine/src/cuda/fragments/system/prelude/SoaKernels.inl
 * @project BLITZAR
 * @brief Shared CUDA system helper or kernel implementation fragment.
 */

__global__ void packSoAKernel(ParticleConstHandle src, ParticleSoAView dst, int numParticles)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles)
        return;

    const Particle p = src[i];
    const Vector3 pos = p.getPosition();
    const Vector3 vel = p.getVelocity();
    const Vector3 press = p.getPressure();

    dst.posX[i] = pos.x;
    dst.posY[i] = pos.y;
    dst.posZ[i] = pos.z;
    dst.velX[i] = vel.x;
    dst.velY[i] = vel.y;
    dst.velZ[i] = vel.z;
    setSoAPressure(dst, i, press);
    dst.mass[i] = p.getMass();
    if (dst.temp != nullptr) {
        dst.temp[i] = p.getTemperature();
    }
    if (dst.dens != nullptr) {
        dst.dens[i] = p.getDensity();
    }
}

/*
 * @brief Documents the unpack so akernel operation contract.
 * @param src Input value used by this contract.
 * @param dst Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void unpackSoAKernel(ParticleSoAView src, ParticleHandle dst, int numParticles)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles)
        return;

    Particle p;
    p.setPosition(getSoAPosition(src, i));
    p.setVelocity(getSoAVelocity(src, i));
    p.setPressure(getSoAPressure(src, i));
    p.setMass(src.mass[i]);
    p.setTemperature(src.temp ? src.temp[i] : 0.0f);
    p.setDensity(src.dens ? src.dens[i] : 0.0f);
    dst[i] = p;
}

__global__ void finalizeRk4Kernel(ParticleSoAView base, Vector3ConstHandle k1x,
                                  Vector3ConstHandle k2x, Vector3ConstHandle k3x,
                                  Vector3ConstHandle k4x, Vector3ConstHandle k1v,
                                  Vector3ConstHandle k2v, Vector3ConstHandle k3v,
                                  Vector3ConstHandle k4v, float deltaTime, ParticleSoAView out,
                                  int numParticles)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }

    const Vector3 weightedVel = (k1v[i] + k2v[i] * 2.0f + k3v[i] * 2.0f + k4v[i]) / 6.0f;
    const Vector3 weightedPos = (k1x[i] + k2x[i] * 2.0f + k3x[i] * 2.0f + k4x[i]) / 6.0f;

    const Vector3 nextVel = getSoAVelocity(base, i) + weightedVel * deltaTime;
    const Vector3 nextPos = getSoAPosition(base, i) + weightedPos * deltaTime;

    setSoAVelocity(out, i, nextVel);
    setSoAPosition(out, i, nextPos);
    setSoAPressure(out, i, weightedVel * 100.0f);
    out.mass[i] = base.mass[i];
    out.temp[i] = base.temp[i];
    out.dens[i] = base.dens[i];
}

// Note: Device buffers and management functions moved to ParticleSystem class members/methods.
