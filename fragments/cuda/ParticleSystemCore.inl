ParticleSystem::ParticleSystem(int numParticles, bool bootstrapInitialState) {
    const int clampedParticles = std::max(2, numParticles);
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
    _cumulativeRadiatedEnergy = 0.0f;
    g_dOctreeNodes = nullptr;
    g_dOctreeLeafIndices = nullptr;
    g_dOctreeNodeCapacity = 0;
    g_dOctreeLeafCapacity = 0;
    _deviceParticleCapacity = static_cast<std::size_t>(clampedParticles);
    d_particles = nullptr;
    last = nullptr;

    if (bootstrapInitialState) {
        Particle p;
        p.setVelocity(Vector3{0, 0, 0});
        float massTerre = 1.0f;
        float diskMass = 0.75f * massTerre;
        const int diskParticleCount = std::max(1, clampedParticles - 1);
        const float diskMassPerParticle = diskMass / static_cast<float>(diskParticleCount);
        const float radiusMin = 1.5f;
        const float radiusMax = 11.5f;
        const float radiusRange2 = std::max(1e-6f, radiusMax * radiusMax - radiusMin * radiusMin);
        p.setMass(massTerre);
        p.setPosition(Vector3{0, 0, 0});
        _particles.push_back(p);
        for (int i = 1; i < clampedParticles; ++i) {
            p.setPosition(Vector3{
                rand() / (float)RAND_MAX * 10.0f + 1.5f,
                rand() / (float)RAND_MAX * 10.0f + 1.5f,
                0.0f
            });
            const float angle = rand() / (float)RAND_MAX * 2.0f * PI;
            p.setPosition(Vector3{
                p.getPosition().x * cosf(angle) - p.getPosition().y * sinf(angle),
                p.getPosition().x * sinf(angle) + p.getPosition().y * cosf(angle),
                0.0f
            });
            p.setMass(diskMassPerParticle);
            const float radius = std::max(p.getPosition().norm(), 1e-4f);
            const float enclosedFraction = std::clamp((radius * radius - radiusMin * radiusMin) / radiusRange2, 0.0f, 1.0f);
            const float enclosedMass = massTerre + diskMass * enclosedFraction;
            const float orbitalSpeed = sqrtf(enclosedMass / radius);
            p.setVelocity(Vector3{
                -p.getPosition().y * orbitalSpeed / radius,
                p.getPosition().x * orbitalSpeed / radius,
                0.0f
            });
            _particles.push_back(p);
        }
    } else {
        _particles.assign(static_cast<std::size_t>(clampedParticles), Particle{});
    }
    // for (int i = -sqrt(numParticles) / 2; i < sqrt(numParticles) / 2; i++)
    //     for (int j = -sqrt(numParticles) / 2; j < sqrt(numParticles) / 2; j++) {
    //         p.setPosition((Vector3){
    //             i * 2.0f,
    //             j * 2.0f,
    //             0
    //         });
    //         p.setVelocity((Vector3){0, 0, 0});
    //         p.setMass(MASS);
    //         _particles.push_back(p);
    //     }
    if (_sphEnabled) {
        fprintf(stdout, "[sph] enabled h=%f restDensity=%f gas=%f viscosity=%f\n",
            _sphSmoothingLength, _sphRestDensity, _sphGasConstant, _sphViscosity);
    }

    if (!checkCudaStatus(cudaMalloc(&d_particles, clampedParticles * sizeof(Particle)), "cudaMalloc(d_particles)")) {
        d_particles = nullptr;
        return;
    }
    if (!checkCudaStatus(cudaMalloc(&last, clampedParticles * sizeof(Particle)), "cudaMalloc(last)")) {
        cudaFree(d_particles);
        d_particles = nullptr;
        last = nullptr;
        return;
    }
    if (bootstrapInitialState) {
        if (!checkCudaStatus(
                cudaMemcpy(d_particles, _particles.data(), clampedParticles * sizeof(Particle), cudaMemcpyHostToDevice),
                "cudaMemcpy(HtoD initial particles)")) {
            cudaFree(d_particles);
            cudaFree(last);
            d_particles = nullptr;
            last = nullptr;
            return;
        }
    }

    if (!allocateSphBuffers(clampedParticles)) {
        fprintf(stderr, "[sph] buffers allocation failed, SPH disabled\n");
        _sphEnabled = false;
    }

    if (_integratorMode == IntegratorMode::Rk4) {
        if (!allocateRk4Buffers(clampedParticles)) {
            fprintf(stderr, "[integrator] rk4 buffers allocation failed, falling back to euler\n");
            _integratorMode = IntegratorMode::Euler;
        }
    }
}

__device__ bool octreeNodeContains(const GpuOctreeNode &node, const Vector3 &pos)
{
    return fabsf(pos.x - node.centerX) <= node.halfSize
        && fabsf(pos.y - node.centerY) <= node.halfSize
        && fabsf(pos.z - node.centerZ) <= node.halfSize;
}

__global__ void updateParticlesOctree(
    ParticleConstHandle lastState,
    ParticleHandle particlesOut,
    int numParticles,
    OctreeNodeConstHandle nodes,
    int rootIndex,
    IndexConstHandle leafIndices,
    float theta,
    float softening,
    float deltaTime
) {
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles || rootIndex < 0) {
        return;
    }

    const Particle selfParticle = lastState[i];
    const Vector3 selfPos = selfParticle.getPosition();
    Vector3 force(0.0f, 0.0f, 0.0f);

    constexpr int kStackCapacity = 64;
    int stack[kStackCapacity];
    int top = 0;
    stack[top++] = rootIndex;

    while (top > 0) {
        const GpuOctreeNode node = nodes[stack[--top]];
        if (node.mass <= 0.0f) {
            continue;
        }

        if (node.childMask == 0) {
            for (int k = 0; k < node.leafCount; ++k) {
                const int otherIndex = leafIndices[node.leafStart + k];
                if (otherIndex == i) {
                    continue;
                }
                const Particle other = lastState[otherIndex];
                const Vector3 direction = other.getPosition() - selfPos;
                const float dist2 = dot(direction, direction) + softening * softening;
                if (dist2 <= 1e-12f) {
                    continue;
                }
                const float invDist = rsqrtf(dist2);
                const float invDist3 = invDist * invDist * invDist;
                force += direction * (other.getMass() * invDist3);
            }
            continue;
        }

        const Vector3 direction(
            node.comX - selfPos.x,
            node.comY - selfPos.y,
            node.comZ - selfPos.z
        );
        const float rawDist2 = dot(direction, direction);
        const float dist2 = rawDist2 + softening * softening;
        const float dist = sqrtf(dist2);
        const bool containsSelf = octreeNodeContains(node, selfPos);
        const float size = node.halfSize * 2.0f;

        if (!containsSelf
            && (size / dist) < theta) {
            const float invDist = rsqrtf(dist2);
            const float invDist3 = invDist * invDist * invDist;
            force += direction * (node.mass * invDist3);
            continue;
        }

        for (int child = 0; child < 8; ++child) {
            if ((node.childMask & (1u << child)) == 0) {
                continue;
            }
            const int childIndex = node.children[child];
            if (childIndex >= 0 && top < kStackCapacity) {
                stack[top++] = childIndex;
            }
        }
    }

    force = clampAcceleration(force, kGravityMaxAcceleration);

    Particle updated = selfParticle;
    updated.setPressure(force * 100.0f);
    updated.setVelocity(updated.getVelocity() + force * deltaTime);
    updated.setPosition(updated.getPosition() + updated.getVelocity() * deltaTime);
    particlesOut[i] = updated;
}

ParticleSystem::~ParticleSystem() {
    if (g_dOctreeNodes) {
        cudaFree(g_dOctreeNodes);
        g_dOctreeNodes = nullptr;
        g_dOctreeNodeCapacity = 0;
    }
    if (g_dOctreeLeafIndices) {
        cudaFree(g_dOctreeLeafIndices);
        g_dOctreeLeafIndices = nullptr;
        g_dOctreeLeafCapacity = 0;
    }
    if (d_particles) {
        cudaFree(d_particles);
        d_particles = nullptr;
    }
    if (last) {
        cudaFree(last);
        last = nullptr;
    }
    releaseRk4Buffers();
    releaseSphBuffers();
}

const std::vector<Particle> &ParticleSystem::getParticles() const {
    return _particles;
}

bool ParticleSystem::setParticles(std::vector<Particle> particles)
{
    if (particles.empty()) {
        return false;
    }
    if (particles.size() != _deviceParticleCapacity) {
        fprintf(stderr,
            "[particles] rejected setParticles size=%zu expected=%zu\n",
            particles.size(),
            _deviceParticleCapacity);
        return false;
    }
    _particles = std::move(particles);
    return true;
}

void ParticleSystem::syncDeviceState()
{
    if (!d_particles || !last || _particles.empty()) {
        return;
    }
    if (_particles.size() > _deviceParticleCapacity) {
        fprintf(stderr,
            "[particles] sync aborted size=%zu exceeds capacity=%zu\n",
            _particles.size(),
            _deviceParticleCapacity);
        return;
    }
    const std::size_t bytes = _particles.size() * sizeof(Particle);
    if (!checkCudaStatus(
            cudaMemcpy(d_particles, _particles.data(), bytes, cudaMemcpyHostToDevice),
            "cudaMemcpy(HtoD sync particles)")) {
        return;
    }
    checkCudaStatus(
        cudaMemcpy(last, _particles.data(), bytes, cudaMemcpyHostToDevice),
        "cudaMemcpy(HtoD sync last)");
}

void ParticleSystem::setUseOctree(bool enabled)
{
    _solverMode = enabled ? SolverMode::OctreeGpu : SolverMode::PairwiseCuda;
}

bool ParticleSystem::usesOctree() const
{
    return _solverMode != SolverMode::PairwiseCuda;
}

void ParticleSystem::setOctreeTheta(float theta)
{
    if (theta > 0.01f) {
        _octreeTheta = theta;
    }
}

void ParticleSystem::setOctreeSoftening(float softening)
{
    if (softening > 1e-5f) {
        _octreeSoftening = softening;
    }
}

void ParticleSystem::setSphEnabled(bool enabled)
{
    if (_sphEnabled != enabled) {
        fprintf(stdout, "[sph] %s\n", enabled ? "enabled" : "disabled");
    }
    _sphEnabled = enabled;
}

bool ParticleSystem::isSphEnabled() const
{
    return _sphEnabled;
}

void ParticleSystem::setSphParameters(float smoothingLength, float restDensity, float gasConstant, float viscosity)
{
    if (smoothingLength > 0.05f) {
        _sphSmoothingLength = smoothingLength;
    }
    if (restDensity > 0.01f) {
        _sphRestDensity = restDensity;
    }
    if (gasConstant > 0.01f) {
        _sphGasConstant = gasConstant;
    }
    if (viscosity >= 0.0f) {
        _sphViscosity = viscosity;
    }
}

void ParticleSystem::setThermalParameters(float ambientTemperature, float specificHeat, float heatingCoeff, float radiationCoeff)
{
    _thermalAmbientTemperature = std::max(0.0f, ambientTemperature);
    _thermalSpecificHeat = std::max(1e-6f, specificHeat);
    _thermalHeatingCoeff = std::max(0.0f, heatingCoeff);
    _thermalRadiationCoeff = std::max(0.0f, radiationCoeff);
}

float ParticleSystem::getCumulativeRadiatedEnergy() const
{
    return _cumulativeRadiatedEnergy;
}

float ParticleSystem::getThermalSpecificHeat() const
{
    return _thermalSpecificHeat;
}

float ParticleSystem::applyThermalModel(float deltaTime)
{
    if (deltaTime <= 0.0f) {
        return 0.0f;
    }
    // Radiative exchange is accumulated as a signed reservoir:
    // positive => net emitted to environment, negative => net absorbed.
    float radiativeExchangeStep = 0.0f;
    constexpr float kStefanBoltzmann = 5.670374419e-8f;
    constexpr float kParticleDensity = 1.0f; // Simulation-unit bulk density.
    const float ambient4 = _thermalAmbientTemperature * _thermalAmbientTemperature * _thermalAmbientTemperature * _thermalAmbientTemperature;

    for (Particle &p : _particles) {
        const float mass = std::max(1e-6f, p.getMass());
        const float heatCapacity = _thermalSpecificHeat * mass;
        float temperature = std::max(0.0f, p.getTemperature());
        Vector3 velocity = p.getVelocity();
        const float speed2 = dot(velocity, velocity);
        float kineticEnergy = 0.5f * mass * speed2;

        // Conservative heating: convert kinetic energy into thermal energy.
        if (_thermalHeatingCoeff > 0.0f && heatCapacity > 1e-9f && kineticEnergy > 1e-12f) {
            const float requestedHeating =
                _thermalHeatingCoeff * p.getPressure().norm() * mass * deltaTime;
            const float convertedEnergy = std::min(std::max(0.0f, requestedHeating), kineticEnergy);
            if (convertedEnergy > 0.0f) {
                const float nextKineticEnergy = std::max(0.0f, kineticEnergy - convertedEnergy);
                const float scale = std::sqrt(std::max(0.0f, nextKineticEnergy / kineticEnergy));
                velocity = velocity * scale;
                p.setVelocity(velocity);
                kineticEnergy = nextKineticEnergy;
                temperature += convertedEnergy / heatCapacity;
            }
        }

        if (_thermalRadiationCoeff > 0.0f && heatCapacity > 1e-9f) {
            // Effective sphere area from mass and a reference density.
            const float radius = cbrtf((3.0f * mass) / (4.0f * kPi * kParticleDensity));
            const float area = 4.0f * kPi * radius * radius;
            const float t2 = temperature * temperature;
            const float t4 = t2 * t2;
            const float emissivity = std::max(0.0f, _thermalRadiationCoeff);
            const float netRadiativePower =
                emissivity * kStefanBoltzmann * area * (t4 - ambient4);
            const float radiativeEnergy = netRadiativePower * deltaTime;

            // positive radiativeEnergy => emitted (thermal decreases)
            // negative radiativeEnergy => absorbed (thermal increases)
            const float nextTemperature = std::max(0.0f, temperature - (radiativeEnergy / heatCapacity));
            const float thermalDelta = (temperature - nextTemperature) * heatCapacity;
            radiativeExchangeStep += thermalDelta;
            temperature = nextTemperature;
        }

        p.setTemperature(temperature);
    }
    _cumulativeRadiatedEnergy += radiativeExchangeStep;
    return radiativeExchangeStep;
}

void ParticleSystem::setSolverMode(SolverMode mode)
{
    if (_solverMode != mode) {
        if (mode == SolverMode::OctreeCpu) {
            fprintf(stdout, "[solver] using octree_cpu (theta=%f, softening=%f)\n", _octreeTheta, _octreeSoftening);
        } else if (mode == SolverMode::OctreeGpu) {
            fprintf(stdout, "[solver] using octree_gpu (theta=%f, softening=%f)\n", _octreeTheta, _octreeSoftening);
        } else {
            fprintf(stdout, "[solver] using pairwise_cuda\n");
        }
    }
    _solverMode = mode;
}

ParticleSystem::SolverMode ParticleSystem::getSolverMode() const
{
    return _solverMode;
}

void ParticleSystem::setIntegratorMode(IntegratorMode mode)
{
    if (_integratorMode != mode) {
        fprintf(stdout, "[integrator] mode=%s\n", mode == IntegratorMode::Rk4 ? "rk4" : "euler");
    }
    _integratorMode = mode;
}

ParticleSystem::IntegratorMode ParticleSystem::getIntegratorMode() const
{
    return _integratorMode;
}

