/*
 * Module: physics/cuda
 * Responsibility: Compute energy diagnostics from device buffers without host particle copies.
 */

__global__ void computeKineticThermalBlockSumsKernel(
    ParticleSoAView state,
    int numParticles,
    float specificHeat,
    float* kineticBlocks,
    float* thermalBlocks)
{
    __shared__ float sharedKinetic[Particle::kDefaultCudaBlockSize];
    __shared__ float sharedThermal[Particle::kDefaultCudaBlockSize];

    const int globalIndex = blockIdx.x * blockDim.x + threadIdx.x;
    float kinetic = 0.0f;
    float thermal = 0.0f;
    if (globalIndex < numParticles) {
        const float vx = state.velX[globalIndex];
        const float vy = state.velY[globalIndex];
        const float vz = state.velZ[globalIndex];
        const float mass = state.mass[globalIndex];
        const float temp = fmaxf(0.0f, state.temp[globalIndex]);
        const float speed2 = vx * vx + vy * vy + vz * vz;
        kinetic = 0.5f * mass * speed2;
        thermal = mass * specificHeat * temp;
    }

    sharedKinetic[threadIdx.x] = kinetic;
    sharedThermal[threadIdx.x] = thermal;
    /// Description: Executes the __syncthreads operation.
    __syncthreads();

    for (int stride = blockDim.x / 2; stride > 0; stride >>= 1) {
        if (threadIdx.x < stride) {
            sharedKinetic[threadIdx.x] += sharedKinetic[threadIdx.x + stride];
            sharedThermal[threadIdx.x] += sharedThermal[threadIdx.x + stride];
        }
        /// Description: Executes the __syncthreads operation.
        __syncthreads();
    }

    if (threadIdx.x == 0) {
        kineticBlocks[blockIdx.x] = sharedKinetic[0];
        thermalBlocks[blockIdx.x] = sharedThermal[0];
    }
}

__global__ void computeSamplePotentialPartialsKernel(
    ParticleSoAView state,
    int numParticles,
    int sampleCount,
    int sampleStride,
    float softening,
    float minDistance2,
    double* potentialPartials)
{
    const int sampleA = blockIdx.x * blockDim.x + threadIdx.x;
    if (sampleA >= sampleCount) {
        return;
    }

    const int idxA = min(sampleA * sampleStride, numParticles - 1);
    const Vector3 pa = getSoAPosition(state, idxA);
    const float massA = state.mass[idxA];
    const float soft2 = softening * softening;

    double partial = 0.0;
    for (int sampleB = sampleA + 1; sampleB < sampleCount; ++sampleB) {
        const int idxB = min(sampleB * sampleStride, numParticles - 1);
        const Vector3 pb = getSoAPosition(state, idxB);
        const float dx = pa.x - pb.x;
        const float dy = pa.y - pb.y;
        const float dz = pa.z - pb.z;
        const float dist2 = dx * dx + dy * dy + dz * dz + soft2;
        if (dist2 <= minDistance2) {
            continue;
        }
        const float dist = sqrtf(dist2);
        if (dist <= 1.0e-6f) {
            continue;
        }
        partial -= static_cast<double>(massA) * static_cast<double>(state.mass[idxB])
            / static_cast<double>(dist);
    }

    potentialPartials[sampleA] = partial;
}

bool ParticleSystem::computeEnergyEstimateGpu(
    std::size_t sampleLimit,
    float softening,
    float minDistance2,
    float specificHeat,
    float &kinetic,
    float &potential,
    float &thermal,
    bool &estimated)
{
    kinetic = 0.0f;
    potential = 0.0f;
    thermal = 0.0f;
    estimated = false;

    const int numParticles = static_cast<int>(_particles.size());
    if (numParticles < 2 || !d_soaPosX || !d_soaVelX || !d_soaMass || !d_soaTemp) {
        return numParticles < 2;
    }

    const int safeSampleLimit = static_cast<int>(std::max<std::size_t>(64u, sampleLimit));
    const bool sampled = static_cast<std::size_t>(numParticles) > sampleLimit;
    const int sampleCountTarget = sampled ? safeSampleLimit : numParticles;
    const int sampleStride = sampled
        ? std::max(1, numParticles / std::max(1, sampleCountTarget))
        : 1;
    const int sampleCount = std::max(2, std::min(numParticles,
        (numParticles + sampleStride - 1) / sampleStride));

    if (!ensureEnergyScratchCapacity(numParticles, sampleCount)) {
        return false;
    }

    const int threads = Particle::kDefaultCudaBlockSize;
    const int blockCount = (numParticles + threads - 1) / threads;
    const int sampleBlocks = (sampleCount + threads - 1) / threads;
    ParticleSoAView currentView = getSoAView(false);

    computeKineticThermalBlockSumsKernel<<<blockCount, threads>>>(
        currentView,
        numParticles,
        specificHeat,
        d_energyKineticBlocks,
        d_energyThermalBlocks);
    if (!checkCudaStatus(cudaGetLastError(), "computeKineticThermalBlockSums kernel launch")) {
        return false;
    }

    computeSamplePotentialPartialsKernel<<<sampleBlocks, threads>>>(
        currentView,
        numParticles,
        sampleCount,
        sampleStride,
        softening,
        minDistance2,
        d_energyPotentialPartials);
    if (!checkCudaStatus(cudaGetLastError(), "computeSamplePotentialPartials kernel launch")) {
        return false;
    }

    /// Description: Executes the kineticBlocks operation.
    std::vector<float> kineticBlocks(static_cast<std::size_t>(blockCount), 0.0f);
    /// Description: Executes the thermalBlocks operation.
    std::vector<float> thermalBlocks(static_cast<std::size_t>(blockCount), 0.0f);
    /// Description: Executes the potentialPartials operation.
    std::vector<double> potentialPartials(static_cast<std::size_t>(sampleCount), 0.0);

    if (!checkCudaStatus(cudaMemcpy(kineticBlocks.data(), d_energyKineticBlocks,
                                    static_cast<std::size_t>(blockCount) * sizeof(float),
                                    cudaMemcpyDeviceToHost),
                         "memcpy(DtoH kinetic blocks)")) {
        return false;
    }
    if (!checkCudaStatus(cudaMemcpy(thermalBlocks.data(), d_energyThermalBlocks,
                                    static_cast<std::size_t>(blockCount) * sizeof(float),
                                    cudaMemcpyDeviceToHost),
                         "memcpy(DtoH thermal blocks)")) {
        return false;
    }
    if (!checkCudaStatus(cudaMemcpy(potentialPartials.data(), d_energyPotentialPartials,
                                    static_cast<std::size_t>(sampleCount) * sizeof(double),
                                    cudaMemcpyDeviceToHost),
                         "memcpy(DtoH potential partials)")) {
        return false;
    }

    double kineticSum = 0.0;
    double thermalSum = 0.0;
    for (float value : kineticBlocks) {
        kineticSum += static_cast<double>(value);
    }
    for (float value : thermalBlocks) {
        thermalSum += static_cast<double>(value);
    }

    double potentialSum = 0.0;
    for (double value : potentialPartials) {
        potentialSum += value;
    }

    const double kineticScale = sampled
        ? static_cast<double>(numParticles) / static_cast<double>(sampleCount)
        : 1.0;
    const double pairCountFull =
        static_cast<double>(numParticles) * static_cast<double>(numParticles - 1) * 0.5;
    const double pairCountSample =
        static_cast<double>(sampleCount) * static_cast<double>(sampleCount - 1) * 0.5;
    const double potentialScale = (sampled && pairCountSample > 0.0)
        ? (pairCountFull / pairCountSample)
        : 1.0;

    kinetic = static_cast<float>(kineticSum * kineticScale);
    thermal = static_cast<float>(thermalSum * kineticScale);
    potential = static_cast<float>(potentialSum * potentialScale);
    estimated = sampled;
    return true;
}
