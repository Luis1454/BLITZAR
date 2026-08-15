/*
 * @file engine/src/cuda/fragments/integration/Update.inl
 * @author Luis1454
 * @project BLITZAR
 * @brief Physics and CUDA implementation for the deterministic simulation core.
 */

/*
 * Module: cuda
 * Responsibility: Advance the particle system for one deterministic update step.
 */

#include <algorithm>
#include <chrono>
#include <cstdlib>

struct CudaEndToEndProfiler final {
    cudaEvent_t start = nullptr;
    cudaEvent_t stop = nullptr;
    double* totalMilliseconds = nullptr;
    std::uint64_t* sampleCount = nullptr;
    std::uint32_t interval = 1000u;
    bool active = false;

    CudaEndToEndProfiler(bool enabled, double* total, std::uint64_t* samples)
        : totalMilliseconds(total), sampleCount(samples)
    {
        if (!enabled || totalMilliseconds == nullptr || sampleCount == nullptr ||
            cudaEventCreate(&start) != cudaSuccess || cudaEventCreate(&stop) != cudaSuccess ||
            cudaEventRecord(start) != cudaSuccess) {
            if (start != nullptr) {
                cudaEventDestroy(start);
                start = nullptr;
            }
            if (stop != nullptr) {
                cudaEventDestroy(stop);
                stop = nullptr;
            }
            return;
        }
        if (const char* rawInterval = std::getenv("BLITZAR_CUDA_E2E_PROFILE_INTERVAL");
            rawInterval != nullptr && rawInterval[0] != '\0') {
            const unsigned long parsed = std::strtoul(rawInterval, nullptr, 10);
            interval =
                static_cast<std::uint32_t>(std::clamp<unsigned long>(parsed, 1ul, 1000000ul));
        }
        active = true;
    }

    ~CudaEndToEndProfiler()
    {
        if (!active) {
            return;
        }
        if (cudaEventRecord(stop) == cudaSuccess && cudaEventSynchronize(stop) == cudaSuccess) {
            float elapsedMilliseconds = 0.0f;
            if (cudaEventElapsedTime(&elapsedMilliseconds, start, stop) == cudaSuccess) {
                *totalMilliseconds += static_cast<double>(elapsedMilliseconds);
                *sampleCount += 1u;
                if ((*sampleCount % interval) == 0u) {
                    fprintf(stderr,
                            "[cuda-e2e] samples=%llu last_ms=%.4f avg_ms=%.4f interval=%u\n",
                            static_cast<unsigned long long>(*sampleCount), elapsedMilliseconds,
                            *totalMilliseconds / static_cast<double>(*sampleCount), interval);
                }
            }
        }
        cudaEventDestroy(start);
        cudaEventDestroy(stop);
    }
};

/*
 * @brief Documents the update operation contract.
 * @param deltaTime Input value used by this contract.
 * @return bool ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool ParticleSystem::update(float deltaTime)
{
    const ForceLawPolicy forceLaw =
        resolveForceLawPolicy(_octreeTheta, _octreeSoftening, _physicsMinSoftening,
                              _physicsMinDistance2, _physicsMinTheta);
    const bool thermalActive = (_thermalHeatingCoeff > 0.0f || _thermalRadiationCoeff > 0.0f);
    const bool e2eProfileEnabled =
        _device._cudaRuntimeAvailable && _solverMode != SolverMode::OctreeCpu &&
        _solverMode != SolverMode::FmmCpu && parseBoolEnv("BLITZAR_CUDA_E2E_PROFILE", false);
    CudaEndToEndProfiler e2eProfiler(e2eProfileEnabled, &_device._cudaE2eTotalMs,
                                     &_device._cudaE2eSamples);
    if (_adaptiveTimeStepsEnabled && _adaptiveTimeStepCostGuard &&
        !_adaptiveTimeStepMarkerPrinted) {
        fprintf(stderr, "[adaptive] backend=fixed_equivalent scheduler=dyadic reason=cost_guard "
                        "force_with_--adaptive-cost-guard=false\n");
        _adaptiveTimeStepMarkerPrinted = true;
    }
    auto syncParticlesFromDevice = [&]() -> bool {
        return syncHostState();
    };
    auto applySphCorrection = [&](bool uploadHostState) -> bool {
        if (!_sphEnabled) {
            return true;
        }
        if (!_device._cudaRuntimeAvailable) {
            return true;
        }
        if (!_device.d_soaPosX || !_device.d_sphDensity || !_device.d_sphPressure) {
            return false;
        }
        const int numParticles = static_cast<int>(_particles.size());
        if (numParticles < 2) {
            return true;
        }
        const int numBlocks =
            (numParticles + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;

        if (uploadHostState) {
            syncDeviceState();
        }

        // Build spatial hash grid for neighbor lookup.
        if (!buildSphGrid(numParticles)) {
            return false;
        }

        SphGridParams grid;
        grid.gridSize = _device._sphGridSize;
        grid.totalCells = _device._sphGridTotalCells;
        grid.cellSize = std::max(0.01f, _sphSmoothingLength);

        ParticleSoAView currentView = getSoAView(false);
        ParticleSoAView nextView = getSoAView(true);

        computeSphDensityPressureGridKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView, _device.d_sphDensity, _device.d_sphPressure, numParticles,
            _sphSmoothingLength, _sphRestDensity, _sphGasConstant, _device.d_sphCellHash,
            _device.d_sphSortedIndex, _device.d_sphCellStart, _device.d_sphCellEnd, grid);
        if (!checkCudaStatus(cudaGetLastError(), "computeSphDensityPressureGrid kernel launch")) {
            return false;
        }

        constexpr float kSphCorrectionScale = 0.22f;
        integrateSphGridKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView, nextView, _device.d_sphDensity, _device.d_sphPressure, numParticles,
            _sphSmoothingLength, _sphViscosity, deltaTime, kSphCorrectionScale,
            _device.d_sphCellHash, _device.d_sphSortedIndex, _device.d_sphCellStart,
            _device.d_sphCellEnd, grid, _sphMaxAcceleration, _sphMaxSpeed);
        if (!checkCudaStatus(cudaGetLastError(), "integrateSphGrid kernel launch")) {
            return false;
        }
        if (!checkCudaStatus(cudaDeviceSynchronize(), "sph grid kernels sync")) {
            return false;
        }

        std::swap(_device.d_soaPosX, _device.d_soaNextPosX);
        std::swap(_device.d_soaPosY, _device.d_soaNextPosY);
        std::swap(_device.d_soaPosZ, _device.d_soaNextPosZ);
        std::swap(_device.d_soaVelX, _device.d_soaNextVelX);
        std::swap(_device.d_soaVelY, _device.d_soaNextVelY);
        std::swap(_device.d_soaVelZ, _device.d_soaNextVelZ);

        _device._hostStateDirty = true;
        return true;
    };

    auto applyHostCosmology = [&]() {
        float scaleRatio = 1.0f;
        float previousHubble = 0.0f;
        float nextHubble = 0.0f;
        if (prepareCosmologyStep(deltaTime, scaleRatio, previousHubble, nextHubble)) {
            applyCosmologyExpansionHost(scaleRatio, previousHubble, nextHubble);
        }
    };

    auto computeCpuAcceleration = [&](const std::vector<Particle>& state,
                                      std::vector<Vector3>& output) -> bool {
        if (_solverMode == SolverMode::FmmCpu) {
            if (!_fmmWorkspace) {
                _fmmWorkspace = std::make_unique<bltzr_fmm::FmmWorkspace>();
            }
            bltzr_fmm::configure(*_fmmWorkspace, _fmmLeafCapacity, _octreeTheta);
            return bltzr_fmm::computeForces(state, forceLaw, *_fmmWorkspace, output);
        }
        const bool cpuFp64Reference =
            _treePmEnabled && _treePmModel == "exact_tree" && _treePmPrecision == "fp64";
        const bool cpuTreePm = _treePmEnabled && _treePmModel != "exact_tree";
        if (cpuFp64Reference) {
            if (!computeCpuFp64PairwiseForces(state, forceLaw, output)) {
                return false;
            }
            if (!_device._treePmMarkerPrinted) {
                fprintf(
                    stderr,
                    "[treepm] enabled solver=cpu_fp64_pairwise model=exact_tree precision=fp64\n");
                _device._treePmMarkerPrinted = true;
            }
            return true;
        }
        else if (cpuTreePm) {
            CpuTreePmParameters parameters;
            parameters.model = _treePmModel;
            parameters.localGrid = _treePmLocalGrid;
            parameters.gridSize = _treePmGridSize;
            parameters.cutoffFactor = _treePmCutoffFactor;
            parameters.maxLocalNeighbors = _treePmMaxLocalNeighbors;
            parameters.particleLimit = _treePmParticleLimit;
            parameters.precision = _treePmPrecision;
            parameters.assignment = _treePmAssignment;
            bool computed = false;
            if (_treePmPrecision == "fp64") {
                if (!_cpuTreePmFp64Workspace) {
                    _cpuTreePmFp64Workspace = std::make_unique<CpuTreePmFp64Workspace>();
                }
                computed = computeCpuTreePmForcesFp64(state, forceLaw, parameters,
                                                      *_cpuTreePmFp64Workspace, _octree,
                                                      _octreeOpeningCriterion, output);
            }
            else {
                if (!_cpuTreePmWorkspace) {
                    _cpuTreePmWorkspace = std::make_unique<CpuTreePmWorkspace>();
                }
                computed = computeCpuTreePmForces(state, forceLaw, parameters, *_cpuTreePmWorkspace,
                                                  _octree, _octreeOpeningCriterion, output);
            }
            if (!computed) {
                return false;
            }
            if (!_device._treePmMarkerPrinted) {
                fprintf(stderr,
                        "[treepm] enabled solver=cpu_fft_%s model=%s assignment=%s grid=%d "
                        "local_grid=%d neighbors=%d\n",
                        _treePmPrecision.c_str(), _treePmModel.c_str(), _treePmAssignment.c_str(),
                        std::clamp(_treePmGridSize, 32, 128), _treePmLocalGrid ? 1 : 0,
                        std::clamp(_treePmMaxLocalNeighbors, 0, 256));
                _device._treePmMarkerPrinted = true;
            }
            return true;
        }

        _octree.build(state);
        output.resize(state.size());
#pragma omp parallel for schedule(static) if (!_deterministicMode)
        for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(state.size()); ++i) {
            output[static_cast<std::size_t>(i)] = _octree.computeForceOn(
                state[static_cast<std::size_t>(i)], static_cast<std::size_t>(i), forceLaw,
                _octreeOpeningCriterion);
            output[static_cast<std::size_t>(i)] =
                clampAcceleration(output[static_cast<std::size_t>(i)], _physicsMaxAcceleration);
        }
        return true;
    };

    // The CUDA-native path below handles pairwise and octree GPU modes. Keep
    // this reference implementation only for the explicit CPU octree solver.
    if (_adaptiveTimeStepsEnabled && !_adaptiveTimeStepCostGuard &&
        (_solverMode == SolverMode::OctreeCpu || _solverMode == SolverMode::FmmCpu)) {
        if (!syncParticlesFromDevice()) {
            return false;
        }
        if (!_adaptiveTimeStepMarkerPrinted) {
            fprintf(stderr,
                    "[adaptive] backend=cpu_reference scheduler=dyadic max_level=%u eta=%.4f\n",
                    _adaptiveTimeStepMaxLevel, _adaptiveTimeStepEta);
            _adaptiveTimeStepMarkerPrinted = true;
        }
        const std::size_t count = _particles.size();
        const std::uint32_t levelCount = std::min<std::uint32_t>(_adaptiveTimeStepMaxLevel, 12u);
        const std::uint32_t sliceCount = 1u << levelCount;
        const float quantum = deltaTime / static_cast<float>(sliceCount);
        if (quantum <= 0.0f) {
            return false;
        }
        const bool stateChanged = _adaptiveTimeStepLevels.size() != count ||
                                  _adaptiveTimeStepAccelerations.size() != count ||
                                  _adaptiveTimeStepLastForceTicks.size() != count ||
                                  std::abs(_adaptiveTimeStepQuantum - quantum) > 1e-12f;
        auto clampVelocityLocal = [&](Vector3 velocity) {
            const float limit = _sphMaxSpeed;
            const float magnitude = velocity.norm();
            return limit > 0.0f && magnitude > limit ? velocity * (limit / magnitude) : velocity;
        };
        auto chooseLevel = [&](Vector3 acceleration, Vector3 velocity) -> std::uint8_t {
            const float accelerationMagnitude = acceleration.norm();
            const float velocityMagnitude = velocity.norm();
            const float softening = std::max(_octreeSoftening, _physicsMinSoftening);
            const float accelerationDt =
                accelerationMagnitude > 1e-6f
                    ? _adaptiveTimeStepEta * std::sqrt(softening / accelerationMagnitude)
                    : deltaTime;
            const float velocityDt = velocityMagnitude > 1e-6f
                                         ? _adaptiveTimeStepEta * softening / velocityMagnitude
                                         : deltaTime;
            const float stableDt = std::min(deltaTime, std::min(accelerationDt, velocityDt));
            std::uint8_t selected = static_cast<std::uint8_t>(levelCount);
            for (std::uint32_t level = 0u; level <= levelCount; ++level) {
                if (deltaTime / static_cast<float>(1u << level) <= stableDt) {
                    selected = static_cast<std::uint8_t>(level);
                    break;
                }
            }
            return selected;
        };

        auto computeCpuAccelerationForIndices = [&](const std::vector<Particle>& state,
                                                    const std::vector<int>& activeIndices,
                                                    std::vector<Vector3>& output) -> bool {
            if (activeIndices.empty()) {
                return true;
            }
            const bool cpuFp64Reference =
                _treePmEnabled && _treePmModel == "exact_tree" && _treePmPrecision == "fp64";
            const bool cpuTreePm = _treePmEnabled && _treePmModel != "exact_tree";
            if (cpuTreePm) {
                CpuTreePmParameters parameters;
                parameters.model = _treePmModel;
                parameters.localGrid = _treePmLocalGrid;
                parameters.gridSize = _treePmGridSize;
                parameters.cutoffFactor = _treePmCutoffFactor;
                parameters.maxLocalNeighbors = _treePmMaxLocalNeighbors;
                parameters.particleLimit = _treePmParticleLimit;
                parameters.precision = _treePmPrecision;
                parameters.assignment = _treePmAssignment;
                if (_treePmPrecision == "fp64") {
                    if (!_cpuTreePmFp64Workspace) {
                        _cpuTreePmFp64Workspace = std::make_unique<CpuTreePmFp64Workspace>();
                    }
                    return computeCpuTreePmForcesSelectiveFp64(
                        state, activeIndices, forceLaw, parameters, *_cpuTreePmFp64Workspace,
                        _octree, _octreeOpeningCriterion, output);
                }
                if (!_cpuTreePmWorkspace) {
                    _cpuTreePmWorkspace = std::make_unique<CpuTreePmWorkspace>();
                }
                return computeCpuTreePmForcesSelective(state, activeIndices, forceLaw, parameters,
                                                       *_cpuTreePmWorkspace, _octree,
                                                       _octreeOpeningCriterion, output);
            }

            if (cpuFp64Reference) {
#pragma omp parallel for schedule(static) if (!_deterministicMode)
                for (std::ptrdiff_t active = 0;
                     active < static_cast<std::ptrdiff_t>(activeIndices.size()); ++active) {
                    const std::size_t target =
                        static_cast<std::size_t>(activeIndices[static_cast<std::size_t>(active)]);
                    const Vector3 position = state[target].getPosition();
                    Vector3 acceleration;
                    for (std::size_t source = 0u; source < state.size(); ++source) {
                        if (source != target) {
                            const Vector3 delta = state[source].getPosition() - position;
                            const float distanceSquared = delta.x * delta.x + delta.y * delta.y +
                                                          delta.z * delta.z +
                                                          forceLaw.softening * forceLaw.softening;
                            if (distanceSquared > forceLaw.minDistance2) {
                                const float inverseDistance = 1.0f / std::sqrt(distanceSquared);
                                const float scale = state[source].getMass() * inverseDistance *
                                                    inverseDistance * inverseDistance;
                                acceleration += delta * scale;
                            }
                        }
                    }
                    output[target] = clampAcceleration(acceleration, _physicsMaxAcceleration);
                }
                return true;
            }

            // The CUDA translation unit does not expose the host Octree force
            // symbol. Keep the explicit CPU-octree reference path unchanged;
            // TreePM and FP64 pairwise paths still use the selective kernels.
            return computeCpuAcceleration(state, output);
        };

        // Rebuild the global representation once per outer step. The selective
        // force path below is valid only while the nested micro-ticks share it.
        std::vector<Vector3> refreshedAccelerations(count, Vector3());
        if (!computeCpuAcceleration(_particles, refreshedAccelerations)) {
            return false;
        }
        _adaptiveTimeStepAccelerations = std::move(refreshedAccelerations);
        _adaptiveTimeStepQuantum = quantum;
        _adaptiveTimeStepLevels.resize(count);
        if (stateChanged || _adaptiveTimeStepTick == 0u) {
            _adaptiveTimeStepLastForceTicks.assign(count, 0u);
        }
        for (std::size_t i = 0u; i < count; ++i) {
            _adaptiveTimeStepLevels[i] =
                chooseLevel(_adaptiveTimeStepAccelerations[i], _particles[i].getVelocity());
        }

        std::vector<Vector3> nextAccelerations(count, Vector3());
        std::vector<int> activeIndices;
        activeIndices.reserve(count);
        for (std::uint32_t slice = 0u; slice < sliceCount; ++slice) {
            for (std::size_t i = 0; i < count; ++i) {
                const Vector3 velocity = _particles[i].getVelocity();
                const Vector3 acceleration = _adaptiveTimeStepAccelerations[i];
                _particles[i].setPosition(_particles[i].getPosition() + velocity * quantum +
                                          acceleration * (0.5f * quantum * quantum));
                _particles[i].setVelocity(clampVelocityLocal(velocity + acceleration * quantum));
            }
            activeIndices.clear();
            const std::uint64_t targetTick = _adaptiveTimeStepTick + 1u;
            for (std::size_t i = 0u; i < count; ++i) {
                const std::uint32_t cadence = 1u << (levelCount - _adaptiveTimeStepLevels[i]);
                if ((targetTick % cadence) == 0u) {
                    activeIndices.push_back(static_cast<int>(i));
                }
            }
            if (!computeCpuAccelerationForIndices(_particles, activeIndices, nextAccelerations)) {
                return false;
            }
            for (const int activeIndex : activeIndices) {
                const std::size_t i = static_cast<std::size_t>(activeIndex);
                const float localDt =
                    static_cast<float>(targetTick - _adaptiveTimeStepLastForceTicks[i]) * quantum;
                const Vector3 correction = nextAccelerations[i] - _adaptiveTimeStepAccelerations[i];
                _particles[i].setPosition(_particles[i].getPosition() +
                                          correction * (0.5f * localDt * localDt));
                _particles[i].setVelocity(
                    clampVelocityLocal(_particles[i].getVelocity() + correction * localDt));
                _adaptiveTimeStepAccelerations[i] = nextAccelerations[i];
                _adaptiveTimeStepLastForceTicks[i] = targetTick;
                _adaptiveTimeStepLevels[i] =
                    chooseLevel(nextAccelerations[i], _particles[i].getVelocity());
                _particles[i].setPressure(nextAccelerations[i] * 100.0f);
            }
            _adaptiveTimeStepTick = targetTick;
        }
        if (thermalActive) {
            applyThermalModel(deltaTime);
        }
        applyHostCosmology();
        syncDeviceState();
        _device._hostStateDirty = false;
        return true;
    }

    if (_solverMode == SolverMode::OctreeCpu || _solverMode == SolverMode::FmmCpu) {
        if (!syncParticlesFromDevice()) {
            return false;
        }
        if (isComovingCosmology(_cosmology)) {
            if (!updateComovingCosmology(deltaTime)) {
                return false;
            }
            syncDeviceState();
            _device._hostStateDirty = false;
            return true;
        }
        if (_integratorMode == IntegratorMode::Euler) {
            if (!computeCpuAcceleration(_particles, _octreeForces)) {
                return false;
            }
            for (std::size_t i = 0; i < _particles.size(); ++i) {
                _octreeForces[i] = clampAcceleration(_octreeForces[i], _physicsMaxAcceleration);
                _particles[i].setPressure(_octreeForces[i] * 100.0f);
            }
#pragma omp parallel for schedule(static) if (!_deterministicMode)
            for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(_particles.size()); ++i) {
                _particles[i].setVelocity(_particles[i].getVelocity() +
                                          _octreeForces[i] * deltaTime);
                _particles[i].setPosition(_particles[i].getPosition() +
                                          _particles[i].getVelocity() * deltaTime);
            }
            if (!applySphCorrection(true)) {
                return false;
            }
            if (_sphEnabled && !syncParticlesFromDevice()) {
                return false;
            }
            if (thermalActive) {
                applyThermalModel(deltaTime);
            }
            applyHostCosmology();
            return true;
        }

        if (_integratorMode == IntegratorMode::Leapfrog) {
            const size_t n = _particles.size();
            std::vector<Vector3> accStart(n);
            std::vector<Vector3> accEnd(n);
            std::vector<Particle> stage = _particles;

            if (!computeCpuAcceleration(_particles, accStart)) {
                return false;
            }

#pragma omp parallel for schedule(static) if (!_deterministicMode)
            for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(n); ++i) {
                const Vector3 velHalf =
                    _particles[i].getVelocity() + accStart[i] * (0.5f * deltaTime);
                stage[i].setVelocity(velHalf);
                stage[i].setPosition(_particles[i].getPosition() + velHalf * deltaTime);
            }

            if (!computeCpuAcceleration(stage, accEnd)) {
                return false;
            }

#pragma omp parallel for schedule(static) if (!_deterministicMode)
            for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(n); ++i) {
                const Vector3 velHalf =
                    _particles[i].getVelocity() + accStart[i] * (0.5f * deltaTime);
                const Vector3 nextVel = velHalf + accEnd[i] * (0.5f * deltaTime);
                _particles[i].setPosition(stage[i].getPosition());
                _particles[i].setVelocity(nextVel);
                _particles[i].setPressure(accEnd[i] * 100.0f);
            }
            if (!applySphCorrection(true)) {
                return false;
            }
            if (_sphEnabled && !syncParticlesFromDevice()) {
                return false;
            }
            if (thermalActive) {
                applyThermalModel(deltaTime);
            }
            applyHostCosmology();
            return true;
        }

        const size_t n = _particles.size();
        std::vector<Vector3> k1x(n), k2x(n), k3x(n), k4x(n);
        std::vector<Vector3> k1v(n), k2v(n), k3v(n), k4v(n);
        std::vector<Particle> stage(n);
        auto resetStage = [&]() {
#pragma omp parallel for schedule(static) if (!_deterministicMode)
            for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(n); ++i) {
                stage[i] = _particles[i];
            }
        };

        auto computeOctreeAcceleration = [&](const std::vector<Particle>& state,
                                             std::vector<Vector3>& outAcc) {
            if (!computeCpuAcceleration(state, outAcc)) {
                outAcc.assign(state.size(), Vector3());
            }
        };

#pragma omp parallel for schedule(static) if (!_deterministicMode)
        for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(n); ++i) {
            k1x[i] = _particles[i].getVelocity();
        }
        computeOctreeAcceleration(_particles, k1v);

        resetStage();
#pragma omp parallel for schedule(static) if (!_deterministicMode)
        for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(n); ++i) {
            stage[i].setPosition(_particles[i].getPosition() + k1x[i] * (0.5f * deltaTime));
            stage[i].setVelocity(_particles[i].getVelocity() + k1v[i] * (0.5f * deltaTime));
            k2x[i] = stage[i].getVelocity();
        }
        computeOctreeAcceleration(stage, k2v);

        resetStage();
#pragma omp parallel for schedule(static) if (!_deterministicMode)
        for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(n); ++i) {
            stage[i].setPosition(_particles[i].getPosition() + k2x[i] * (0.5f * deltaTime));
            stage[i].setVelocity(_particles[i].getVelocity() + k2v[i] * (0.5f * deltaTime));
            k3x[i] = stage[i].getVelocity();
        }
        computeOctreeAcceleration(stage, k3v);

        resetStage();
#pragma omp parallel for schedule(static) if (!_deterministicMode)
        for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(n); ++i) {
            stage[i].setPosition(_particles[i].getPosition() + k3x[i] * deltaTime);
            stage[i].setVelocity(_particles[i].getVelocity() + k3v[i] * deltaTime);
            k4x[i] = stage[i].getVelocity();
        }
        computeOctreeAcceleration(stage, k4v);

#pragma omp parallel for schedule(static) if (!_deterministicMode)
        for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(n); ++i) {
            const Vector3 weightedVel = (k1v[i] + k2v[i] * 2.0f + k3v[i] * 2.0f + k4v[i]) / 6.0f;
            const Vector3 weightedPos = (k1x[i] + k2x[i] * 2.0f + k3x[i] * 2.0f + k4x[i]) / 6.0f;
            _particles[i].setVelocity(_particles[i].getVelocity() + weightedVel * deltaTime);
            _particles[i].setPosition(_particles[i].getPosition() + weightedPos * deltaTime);
            _particles[i].setPressure(weightedVel * 100.0f);
        }
        if (!applySphCorrection(true)) {
            return false;
        }
        if (_sphEnabled && !syncParticlesFromDevice()) {
            return false;
        }
        if (thermalActive) {
            applyThermalModel(deltaTime);
        }
        applyHostCosmology();
        return true;
    }

    if (_solverMode == SolverMode::OctreeGpu) {
        const bool profileFlashMode = parseBoolEnv("BLITZAR_OCTREE_PROFILE_FLASH", false);
        const bool treePmEnabled = _treePmEnabled && _treePmModel != "exact_tree";
        const bool treePmLegacyModel = _treePmModel == "auto" || _treePmModel.empty();
        const bool treePmHybrid =
            treePmEnabled && _integratorMode == IntegratorMode::Euler && _treePmModel == "hybrid";
        const bool treePmPmOnly = treePmEnabled && _treePmModel == "pm_only";
        const bool treePmLocalGrid =
            treePmEnabled && _integratorMode == IntegratorMode::Euler &&
            (treePmHybrid || treePmPmOnly || (_treePmModel == "local_grid" && _treePmLocalGrid) ||
             (treePmLegacyModel && _treePmLocalGrid));
        const int treePmMaxLocalNeighbors =
            treePmLocalGrid && !treePmPmOnly ? std::clamp(_treePmMaxLocalNeighbors, 0, 256) : 0;
        const bool treePmNeighborGrid =
            treePmLocalGrid && (treePmHybrid || treePmMaxLocalNeighbors > 0);
        bool treePmGather = treePmNeighborGrid && treePmGatherEnabled();
        bool treePmMorton = treePmNeighborGrid && treePmMortonEnabled();
        const bool treePmGraphRequested = parseBoolEnv("BLITZAR_TREEPM_GRAPH", false) &&
                                          treePmPmOnly && !_adaptiveTimeStepsEnabled &&
                                          !thermalActive && !_cosmology.enabled;
        if (_integratorMode == IntegratorMode::Rk4) {
            fprintf(stderr, "[integrator] rk4 is not supported with octree_gpu\n");
            return false;
        }
        if (!_device.d_soaPosX) {
            return false;
        }
        ParticleSoAView currentView = getSoAView(false);
        ParticleSoAView nextView = getSoAView(true);

        const int numParticles = static_cast<int>(_particles.size());
        const int adaptiveNumBlocks =
            (numParticles + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;

        if (isComovingCosmology(_cosmology)) {
            if (numParticles < 2) {
                return false;
            }
            if (!_device.d_k1v || !_device.d_k2v) {
                if (!allocateRk4Buffers(numParticles)) {
                    return false;
                }
            }
            if (!_device.d_vHalf) {
                _device.d_vHalf = static_cast<GpuHalfVelocity*>(bltzr_x::MemoryPool::allocate(
                    static_cast<std::size_t>(numParticles) * sizeof(GpuHalfVelocity)));
                if (!_device.d_vHalf) {
                    return false;
                }
            }
            const auto hubbleRate = [this](float scaleFactor) {
                const float a = std::max(scaleFactor, 1.0e-6f);
                const float density = _cosmology.omegaRadiation / std::pow(a, 4.0f) +
                                      _cosmology.omegaMatter / std::pow(a, 3.0f) +
                                      _cosmology.omegaLambda;
                return _cosmology.hubbleH0 * std::sqrt(std::max(0.0f, density));
            };
            const float a0 = std::max(_cosmologyScaleFactor, 1.0e-6f);
            const float midpointPredict =
                std::max(a0 + 0.5f * a0 * hubbleRate(a0) * deltaTime, 1.0e-6f);
            const float a1 =
                std::max(a0, a0 + midpointPredict * hubbleRate(midpointPredict) * deltaTime);
            const float amid = 0.5f * (a0 + a1);
            const auto driftIntegrand = [&hubbleRate](float a) {
                return 1.0f / (a * a * a * std::max(hubbleRate(a), 1.0e-12f));
            };
            const float drift =
                (a1 - a0) *
                (driftIntegrand(a0) + 4.0f * driftIntegrand(amid) + driftIntegrand(a1)) / 6.0f;
            if (a1 <= a0 || drift <= 0.0f) {
                return false;
            }
            TreePmGridParams grid{};
            float unusedCutoff = 0.0f;
            _cosmologyScaleFactor = amid;
            if (!buildTreePmGrid(currentView, numParticles, &grid, &unusedCutoff)) {
                return false;
            }
            computeTreePmPmOnlyAccelerationKernel<<<adaptiveNumBlocks,
                                                    Particle::kDefaultCudaBlockSize>>>(
                currentView, _device.d_k1v, numParticles, grid, _device.d_treePmAccelX,
                _device.d_treePmAccelY, _device.d_treePmAccelZ);
            auto* momentumHalf = reinterpret_cast<float3*>(_device.d_vHalf);
            applyKickHalfStepKernel<<<adaptiveNumBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, _device.d_k1v, deltaTime, momentumHalf, numParticles);
            cosmologyDriftKernel<<<adaptiveNumBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, momentumHalf, drift, 2.0f * _cosmology.boxHalfExtent, nextView,
                numParticles);
            if (!checkCudaStatus(cudaGetLastError(), "cosmology first KDK launch") ||
                !checkCudaStatus(cudaDeviceSynchronize(), "cosmology first KDK sync")) {
                return false;
            }
            std::swap(_device.d_soaPosX, _device.d_soaNextPosX);
            std::swap(_device.d_soaPosY, _device.d_soaNextPosY);
            std::swap(_device.d_soaPosZ, _device.d_soaNextPosZ);
            std::swap(_device.d_soaVelX, _device.d_soaNextVelX);
            std::swap(_device.d_soaVelY, _device.d_soaNextVelY);
            std::swap(_device.d_soaVelZ, _device.d_soaNextVelZ);
            currentView = getSoAView(false);
            nextView = getSoAView(true);
            _cosmologyScaleFactor = a1;
            if (!buildTreePmGrid(currentView, numParticles, &grid, &unusedCutoff)) {
                return false;
            }
            computeTreePmPmOnlyAccelerationKernel<<<adaptiveNumBlocks,
                                                    Particle::kDefaultCudaBlockSize>>>(
                currentView, _device.d_k2v, numParticles, grid, _device.d_treePmAccelX,
                _device.d_treePmAccelY, _device.d_treePmAccelZ);
            finalizeLeapfrogKickKernel<<<adaptiveNumBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, momentumHalf, _device.d_k2v, deltaTime, nextView, momentumHalf,
                numParticles);
            if (!checkCudaStatus(cudaGetLastError(), "cosmology final KDK launch") ||
                !checkCudaStatus(cudaDeviceSynchronize(), "cosmology final KDK sync")) {
                return false;
            }
            std::swap(_device.d_soaPosX, _device.d_soaNextPosX);
            std::swap(_device.d_soaPosY, _device.d_soaNextPosY);
            std::swap(_device.d_soaPosZ, _device.d_soaNextPosZ);
            std::swap(_device.d_soaVelX, _device.d_soaNextVelX);
            std::swap(_device.d_soaVelY, _device.d_soaNextVelY);
            std::swap(_device.d_soaVelZ, _device.d_soaNextVelZ);
            _cosmologyTime += deltaTime;
            _device._hostStateDirty = true;
            if (!_cosmologyMarkerPrinted) {
                fprintf(
                    stderr,
                    "[cosmology] mode=comoving backend=cuda_pm assignment=tsc box=%.6g a0=%.6g\n",
                    2.0f * _cosmology.boxHalfExtent, a0);
                _cosmologyMarkerPrinted = true;
            }
            publishMappedMetrics(deltaTime);
            return true;
        }

        if (treePmGraphRequested && _device._treePmGraphCaptured[_device._treePmGraphSlot]) {
            if (!launchTreePmGraph(_device._treePmGraphSlot)) {
                return false;
            }
            std::swap(_device.d_soaPosX, _device.d_soaNextPosX);
            std::swap(_device.d_soaPosY, _device.d_soaNextPosY);
            std::swap(_device.d_soaPosZ, _device.d_soaNextPosZ);
            std::swap(_device.d_soaVelX, _device.d_soaNextVelX);
            std::swap(_device.d_soaVelY, _device.d_soaNextVelY);
            std::swap(_device.d_soaVelZ, _device.d_soaNextVelZ);
            _device._treePmGraphSlot ^= 1;
            _device._hostStateDirty = true;
            if (!_device._treePmGraphMarkerPrinted) {
                fprintf(stderr,
                        "[treepm] cuda_graph=active model=pm_only static_mesh_pipeline=1\n");
                _device._treePmGraphMarkerPrinted = true;
            }
            publishMappedMetrics(deltaTime);
            return true;
        }

        const cudaFuncCache cachePreference =
            _cudaCachePreference == "shared"    ? cudaFuncCachePreferShared
            : _cudaCachePreference == "default" ? cudaFuncCachePreferNone
                                                : cudaFuncCachePreferL1;
        if (!checkCudaStatus(
                cudaFuncSetCacheConfig(computeOctreeAccelerationKernel, cachePreference),
                "computeOctreeAccelerationKernel cache config")) {
            return false;
        }
        if (!checkCudaStatus(cudaFuncSetCacheConfig(updateParticlesOctree, cachePreference),
                             "updateParticlesOctree cache config")) {
            return false;
        }

        if ((!treePmLocalGrid || treePmHybrid) &&
            !buildLinearOctreeGpu(currentView, numParticles)) {
            return false;
        }
        const int rootIndex = _device._gpuOctreeRootIndex;
        TreePmGridParams treePmGrid{};
        float treePmCutoffSquared = 0.0f;
        if (treePmEnabled) {
            if (!buildTreePmGrid(currentView, numParticles, &treePmGrid, &treePmCutoffSquared)) {
                return false;
            }
            treePmGather = treePmNeighborGrid && treePmGatherEnabled();
            treePmMorton = treePmNeighborGrid && treePmMortonEnabled();
            if (!_device._treePmMarkerPrinted) {
                fprintf(stderr,
                        "[treepm] enabled solver=%s precision=fp32 requested_precision=%s model=%s "
                        "assignment=%s grid=%d jacobi=%d local_grid=%d neighbors=%d "
                        "pm_particles=%d dense_threshold=%d cutoff2=%.6f cache=%s gather=%d "
                        "morton=%d\n",
                        _device._treePmFftActive ? "fft" : "red_black", _treePmPrecision.c_str(),
                        _treePmModel.c_str(), _treePmAssignment.c_str(), treePmGrid.gridSize,
                        _treePmJacobiIterations, treePmLocalGrid ? 1 : 0, treePmMaxLocalNeighbors,
                        numParticles, _treePmDenseCellThreshold, treePmCutoffSquared,
                        _cudaCachePreference.c_str(), treePmGather ? 1 : 0, treePmMorton ? 1 : 0);
                _device._treePmMarkerPrinted = true;
            }
            if (treePmGraphRequested && !_device._treePmGraphCaptured[_device._treePmGraphSlot]) {
                if (!captureTreePmGraph(
                        _device._treePmGraphSlot, currentView, nextView, numParticles,
                        _treePmParticleLimit <= 0 ? numParticles
                                                  : std::min(_treePmParticleLimit, numParticles),
                        treePmGrid, treePmCutoffSquared, forceLaw, deltaTime,
                        _physicsMaxAcceleration)) {
                    fprintf(stderr, "[treepm] cuda_graph=capture_failed fallback=regular\n");
                }
            }
            if (treePmNeighborGrid &&
                !buildTreePmNeighborGrid(currentView, numParticles, treePmGrid)) {
                return false;
            }
        }

        if (_adaptiveTimeStepsEnabled && !_adaptiveTimeStepCostGuard) {
            const std::uint32_t levelCount =
                std::min<std::uint32_t>(_adaptiveTimeStepMaxLevel, 12u);
            const std::uint32_t sliceCount = 1u << levelCount;
            const float quantum = deltaTime / static_cast<float>(sliceCount);
            if (quantum <= 0.0f || !ensureAdaptiveCudaScratchCapacity(numParticles)) {
                fprintf(stderr, "[adaptive] CUDA scratch allocation failed\n");
                return false;
            }

            AdaptiveGpuForceContext forceContext{};
            forceContext.mode = treePmHybrid ? 3 : treePmLocalGrid ? 1 : treePmEnabled ? 2 : 0;
            forceContext.nodeHot = _device.d_octreeNodeHot;
            forceContext.nodeNav = _device.d_octreeNodeNav;
            forceContext.nodeFirstChild = _device.d_octreeFirstChild;
            forceContext.leafStarts = _device.d_octreeLeafStarts;
            forceContext.leafCounts = _device.d_octreeLeafCounts;
            forceContext.rootIndex = rootIndex;
            forceContext.leafIndices = _device.g_dOctreeLeafIndices;
            forceContext.forceLaw = forceLaw;
            forceContext.maxAcceleration = _physicsMaxAcceleration;
            forceContext.openingCriterion =
                _octreeOpeningCriterion == OctreeOpeningCriterion::Bounds ? 1 : 0;
            forceContext.grid = treePmGrid;
            forceContext.sortedIndex = _device.d_sphSortedIndex;
            forceContext.cellStart = _device.d_sphCellStart;
            forceContext.cellEnd = _device.d_sphCellEnd;
            forceContext.pmAccelX = _device.d_treePmAccelX;
            forceContext.pmAccelY = _device.d_treePmAccelY;
            forceContext.pmAccelZ = _device.d_treePmAccelZ;
            forceContext.cellMask = _device.d_treePmCellMask;
            forceContext.cutoffSquared = treePmCutoffSquared;
            forceContext.cellRadius =
                std::clamp(static_cast<int>(
                               std::ceil(std::sqrt(treePmCutoffSquared) * treePmGrid.invCellSize)),
                           1, 2);
            forceContext.maxLocalNeighbors = treePmMaxLocalNeighbors;
            forceContext.sortedPosX = treePmGather ? _device.d_treePmSortedPosX : nullptr;
            forceContext.sortedPosY = treePmGather ? _device.d_treePmSortedPosY : nullptr;
            forceContext.sortedPosZ = treePmGather ? _device.d_treePmSortedPosZ : nullptr;
            forceContext.sortedMass = treePmGather ? _device.d_treePmSortedMass : nullptr;
            forceContext.denseCellThreshold = std::max(_treePmDenseCellThreshold, 1);

            const bool resetSchedule = _adaptiveTimeStepTick == 0u ||
                                       std::abs(_adaptiveTimeStepQuantum - quantum) > 1.0e-12f;
            if (resetSchedule) {
                computeAdaptiveForceKernel<<<adaptiveNumBlocks, Particle::kDefaultCudaBlockSize>>>(
                    currentView, _device.d_adaptiveAcceleration, numParticles, forceContext);
                if (!checkCudaStatus(cudaGetLastError(), "adaptive octree initial force launch")) {
                    return false;
                }
                initializeAdaptiveScheduleKernel<<<adaptiveNumBlocks,
                                                   Particle::kDefaultCudaBlockSize>>>(
                    currentView, _device.d_adaptiveAcceleration, _device.d_adaptiveLevels,
                    _device.d_adaptiveLastForceTicks, numParticles, static_cast<int>(levelCount),
                    _adaptiveTimeStepEta, std::max(_octreeSoftening, _physicsMinSoftening),
                    deltaTime);
                if (!checkCudaStatus(cudaGetLastError(), "adaptive octree schedule launch")) {
                    return false;
                }
                _adaptiveTimeStepQuantum = quantum;
            }

            for (std::uint32_t slice = 0u; slice < sliceCount; ++slice) {
                const unsigned long long targetTick =
                    static_cast<unsigned long long>(_adaptiveTimeStepTick + slice + 1u);
                adaptiveDriftKernel<<<adaptiveNumBlocks, Particle::kDefaultCudaBlockSize>>>(
                    currentView, nextView, _device.d_adaptiveAcceleration, numParticles, quantum,
                    _sphMaxSpeed);
                if (!checkCudaStatus(cudaGetLastError(), "adaptive octree drift launch")) {
                    return false;
                }
                adaptiveOctreeCorrectKernel<<<adaptiveNumBlocks, Particle::kDefaultCudaBlockSize>>>(
                    nextView, _device.d_adaptiveAcceleration, _device.d_adaptiveLevels,
                    _device.d_adaptiveLastForceTicks, numParticles, forceContext, quantum,
                    static_cast<int>(levelCount), _adaptiveTimeStepEta,
                    std::max(_octreeSoftening, _physicsMinSoftening), deltaTime, targetTick,
                    _sphMaxSpeed);
                if (!checkCudaStatus(cudaGetLastError(), "adaptive octree correction launch")) {
                    return false;
                }
                std::swap(_device.d_soaPosX, _device.d_soaNextPosX);
                std::swap(_device.d_soaPosY, _device.d_soaNextPosY);
                std::swap(_device.d_soaPosZ, _device.d_soaNextPosZ);
                std::swap(_device.d_soaVelX, _device.d_soaNextVelX);
                std::swap(_device.d_soaVelY, _device.d_soaNextVelY);
                std::swap(_device.d_soaVelZ, _device.d_soaNextVelZ);
                currentView = getSoAView(false);
                nextView = getSoAView(true);
            }
            if (!checkCudaStatus(cudaDeviceSynchronize(), "adaptive octree sync")) {
                return false;
            }
            _adaptiveTimeStepTick += sliceCount;
            _device._leapfrogPrimed = false;
            _device._hostStateDirty = true;
            float scaleRatio = 1.0f;
            float previousHubble = 0.0f;
            float nextHubble = 0.0f;
            if (prepareCosmologyStep(deltaTime, scaleRatio, previousHubble, nextHubble)) {
                applyCosmologyExpansionKernel<<<adaptiveNumBlocks,
                                                Particle::kDefaultCudaBlockSize>>>(
                    getSoAView(false), numParticles, scaleRatio, previousHubble, nextHubble);
                if (!checkCudaStatus(cudaGetLastError(), "cosmology expansion kernel launch") ||
                    !checkCudaStatus(cudaDeviceSynchronize(), "cosmology expansion kernel sync")) {
                    return false;
                }
            }
            if (!applySphCorrection(false)) {
                return false;
            }
            if (thermalActive) {
                if (!syncParticlesFromDevice()) {
                    return false;
                }
                applyThermalModel(deltaTime);
                syncDeviceState();
            }
            if (!_adaptiveTimeStepMarkerPrinted) {
                fprintf(stderr,
                        "[adaptive] backend=cuda_native solver=octree_gpu scheduler=dyadic "
                        "max_level=%u eta=%.4f tree_rebuild=global_step mode=%d\n",
                        levelCount, _adaptiveTimeStepEta, forceContext.mode);
                _adaptiveTimeStepMarkerPrinted = true;
            }
            publishMappedMetrics(deltaTime);
            return true;
        }

        if (_integratorMode == IntegratorMode::Leapfrog) {
            if (!_device.d_k1v || !_device.d_k2v) {
                if (!allocateRk4Buffers(static_cast<int>(_particles.size()))) {
                    fprintf(stderr, "[integrator] leapfrog buffers missing\n");
                    return false;
                }
            }
            if (!_device.d_vHalf) {
                fprintf(stderr, "[integrator] leapfrog v_half buffer missing\n");
                return false;
            }
        }

        if (_integratorMode == IntegratorMode::Leapfrog) {
            const int openingCriterion =
                _octreeOpeningCriterion == OctreeOpeningCriterion::Bounds ? 1 : 0;
            const int numBlocks = (numParticles + Particle::kDefaultCudaBlockSize - 1) /
                                  Particle::kDefaultCudaBlockSize;
            auto* halfVelocity = reinterpret_cast<float3*>(_device.d_vHalf);

            bool treePmLeapfrogCompleted = false;
            if (!_device._leapfrogPrimed && treePmEnabled) {
                    computeTreePmAccelerationKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                        currentView, _device.d_k1v, numParticles, _device.d_octreeNodeHot,
                        _device.d_octreeNodeNav, _device.d_octreeFirstChild,
                        _device.d_octreeLeafStarts, _device.d_octreeLeafCounts, rootIndex,
                        _device.g_dOctreeLeafIndices, forceLaw, _physicsMaxAcceleration,
                        openingCriterion, treePmGrid, _device.d_treePmAccelX,
                        _device.d_treePmAccelY, _device.d_treePmAccelZ, treePmCutoffSquared);
                    if (!checkCudaStatus(cudaGetLastError(),
                                         "computeTreePmAcceleration kick1 launch")) {
                        return false;
                    }
                    applyKickHalfStepKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                        currentView, _device.d_k1v, deltaTime, halfVelocity, numParticles);
                    if (!checkCudaStatus(cudaGetLastError(), "applyKickHalfStepKernel launch")) {
                        return false;
                    }
                    driftWithHalfVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                        currentView, halfVelocity, deltaTime, nextView, numParticles);
                    if (!checkCudaStatus(cudaGetLastError(),
                                         "driftWithHalfVelocityKernel launch")) {
                        return false;
                    }
                    if (!checkCudaStatus(cudaDeviceSynchronize(), "treepm leapfrog drift sync")) {
                        return false;
                    }
                    std::swap(_device.d_soaPosX, _device.d_soaNextPosX);
                    std::swap(_device.d_soaPosY, _device.d_soaNextPosY);
                    std::swap(_device.d_soaPosZ, _device.d_soaNextPosZ);
                    std::swap(_device.d_soaVelX, _device.d_soaNextVelX);
                    std::swap(_device.d_soaVelY, _device.d_soaNextVelY);
                    std::swap(_device.d_soaVelZ, _device.d_soaNextVelZ);

                    currentView = getSoAView(false);
                    nextView = getSoAView(true);

                    if (!buildLinearOctreeGpu(currentView, numParticles)) {
                        return false;
                    }
                    if (treePmEnabled && !buildTreePmGrid(currentView, numParticles, &treePmGrid,
                                                          &treePmCutoffSquared)) {
                        return false;
                    }

                    computeTreePmAccelerationKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                        currentView, _device.d_k2v, numParticles, _device.d_octreeNodeHot,
                        _device.d_octreeNodeNav, _device.d_octreeFirstChild,
                        _device.d_octreeLeafStarts, _device.d_octreeLeafCounts,
                        _device._gpuOctreeRootIndex, _device.g_dOctreeLeafIndices, forceLaw,
                        _physicsMaxAcceleration, openingCriterion, treePmGrid,
                        _device.d_treePmAccelX, _device.d_treePmAccelY, _device.d_treePmAccelZ,
                        treePmCutoffSquared);
                    if (!checkCudaStatus(cudaGetLastError(),
                                         "computeTreePmAcceleration kick2 launch")) {
                        return false;
                    }

                    finalizeLeapfrogKickKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                        currentView, halfVelocity, _device.d_k2v, deltaTime, nextView, halfVelocity,
                        numParticles);
                    if (!checkCudaStatus(cudaGetLastError(), "finalizeLeapfrogKickKernel launch")) {
                        return false;
                    }
                    if (!checkCudaStatus(cudaDeviceSynchronize(),
                                         "treepm leapfrog finalize sync")) {
                        return false;
                    }

                    std::swap(_device.d_soaPosX, _device.d_soaNextPosX);
                    std::swap(_device.d_soaPosY, _device.d_soaNextPosY);
                    std::swap(_device.d_soaPosZ, _device.d_soaNextPosZ);
                    std::swap(_device.d_soaVelX, _device.d_soaNextVelX);
                    std::swap(_device.d_soaVelY, _device.d_soaNextVelY);
                    std::swap(_device.d_soaVelZ, _device.d_soaNextVelZ);
                treePmLeapfrogCompleted = true;
            }
            if (!treePmLeapfrogCompleted) {
                if (!_device._leapfrogPrimed) {
                primeHalfVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                    currentView, halfVelocity, numParticles);
                if (!checkCudaStatus(cudaGetLastError(), "primeHalfVelocityKernel launch")) {
                    return false;
                }
                _device._leapfrogPrimed = true;
                }

            computeOctreeAccelerationKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, _device.d_k1v, numParticles, _device.d_octreeNodeHot,
                _device.d_octreeNodeNav, _device.d_octreeFirstChild, _device.d_octreeLeafStarts,
                _device.d_octreeLeafCounts, rootIndex, _device.g_dOctreeLeafIndices, forceLaw,
                _physicsMaxAcceleration, openingCriterion, 0.0f);
            if (!checkCudaStatus(cudaGetLastError(), "computeOctreeAcceleration kick1 launch")) {
                return false;
            }

            applyKickHalfStepKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, _device.d_k1v, deltaTime, halfVelocity, numParticles);
            if (!checkCudaStatus(cudaGetLastError(), "applyKickHalfStepKernel launch")) {
                return false;
            }

            driftWithHalfVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, halfVelocity, deltaTime, nextView, numParticles);
            if (!checkCudaStatus(cudaGetLastError(), "driftWithHalfVelocityKernel launch")) {
                return false;
            }
            if (!checkCudaStatus(cudaDeviceSynchronize(), "leapfrog drift sync")) {
                return false;
            }

            std::swap(_device.d_soaPosX, _device.d_soaNextPosX);
            std::swap(_device.d_soaPosY, _device.d_soaNextPosY);
            std::swap(_device.d_soaPosZ, _device.d_soaNextPosZ);
            std::swap(_device.d_soaVelX, _device.d_soaNextVelX);
            std::swap(_device.d_soaVelY, _device.d_soaNextVelY);
            std::swap(_device.d_soaVelZ, _device.d_soaNextVelZ);

            currentView = getSoAView(false);
            nextView = getSoAView(true);

            if (!buildLinearOctreeGpu(currentView, numParticles)) {
                return false;
            }
            const int nextRootIndex = _device._gpuOctreeRootIndex;

            computeOctreeAccelerationKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, _device.d_k2v, numParticles, _device.d_octreeNodeHot,
                _device.d_octreeNodeNav, _device.d_octreeFirstChild, _device.d_octreeLeafStarts,
                _device.d_octreeLeafCounts, nextRootIndex, _device.g_dOctreeLeafIndices, forceLaw,
                _physicsMaxAcceleration, openingCriterion, 0.0f);
            if (!checkCudaStatus(cudaGetLastError(), "computeOctreeAcceleration kick2 launch")) {
                return false;
            }

            finalizeLeapfrogKickKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, halfVelocity, _device.d_k2v, deltaTime, nextView, halfVelocity,
                numParticles);
            if (!checkCudaStatus(cudaGetLastError(), "finalizeLeapfrogKickKernel launch")) {
                return false;
            }
            if (!checkCudaStatus(cudaDeviceSynchronize(), "leapfrog finalize sync")) {
                return false;
            }

            std::swap(_device.d_soaPosX, _device.d_soaNextPosX);
            std::swap(_device.d_soaPosY, _device.d_soaNextPosY);
            std::swap(_device.d_soaPosZ, _device.d_soaNextPosZ);
            std::swap(_device.d_soaVelX, _device.d_soaNextVelX);
            std::swap(_device.d_soaVelY, _device.d_soaNextVelY);
            std::swap(_device.d_soaVelZ, _device.d_soaNextVelZ);
        }
        }
        else if (_integratorMode == IntegratorMode::Euler) {
            const auto forceStartTime = std::chrono::high_resolution_clock::now();
            const int numBlocks = (numParticles + Particle::kDefaultCudaBlockSize - 1) /
                                  Particle::kDefaultCudaBlockSize;
            if (treePmHybrid) {
                const int treePmCellRadius =
                    std::clamp(static_cast<int>(std::ceil(std::sqrt(treePmCutoffSquared) *
                                                          treePmGrid.invCellSize)),
                               1, 2);
                updateParticlesTreePmHybridKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                    currentView, nextView, numParticles, _device.d_octreeNodeHot,
                    _device.d_octreeNodeNav, _device.d_octreeFirstChild, _device.d_octreeLeafStarts,
                    _device.d_octreeLeafCounts, rootIndex, _device.g_dOctreeLeafIndices, forceLaw,
                    deltaTime, _physicsMaxAcceleration,
                    _octreeOpeningCriterion == OctreeOpeningCriterion::Bounds ? 1 : 0, treePmGrid,
                    _device.d_sphSortedIndex, _device.d_sphCellStart, _device.d_sphCellEnd,
                    _device.d_treePmAccelX, _device.d_treePmAccelY, _device.d_treePmAccelZ,
                    _device.d_treePmCellMask, treePmCutoffSquared, treePmCellRadius,
                    treePmMaxLocalNeighbors, std::max(_treePmDenseCellThreshold, 1),
                    treePmGather ? _device.d_treePmSortedPosX : nullptr,
                    treePmGather ? _device.d_treePmSortedPosY : nullptr,
                    treePmGather ? _device.d_treePmSortedPosZ : nullptr,
                    treePmGather ? _device.d_treePmSortedMass : nullptr);
                if (!checkCudaStatus(cudaGetLastError(),
                                     "updateParticlesTreePmHybrid kernel launch")) {
                    return false;
                }
            }
            else if (treePmLocalGrid) {
                const int treePmCellRadius =
                    std::clamp(static_cast<int>(std::ceil(std::sqrt(treePmCutoffSquared) *
                                                          treePmGrid.invCellSize)),
                               1, 2);
                updateParticlesTreePmLocalGridKernel<<<numBlocks,
                                                       Particle::kDefaultCudaBlockSize>>>(
                    currentView, nextView, numParticles, treePmGrid, _device.d_sphSortedIndex,
                    _device.d_sphCellStart, _device.d_sphCellEnd, forceLaw, deltaTime,
                    _physicsMaxAcceleration, _device.d_treePmAccelX, _device.d_treePmAccelY,
                    _device.d_treePmAccelZ, _device.d_treePmCellMask, treePmCutoffSquared,
                    treePmCellRadius, treePmMaxLocalNeighbors,
                    treePmGather ? _device.d_treePmSortedPosX : nullptr,
                    treePmGather ? _device.d_treePmSortedPosY : nullptr,
                    treePmGather ? _device.d_treePmSortedPosZ : nullptr,
                    treePmGather ? _device.d_treePmSortedMass : nullptr);
                if (!checkCudaStatus(cudaGetLastError(),
                                     "updateParticlesTreePmLocalGrid kernel launch")) {
                    return false;
                }
            }
            else if (treePmEnabled) {
                updateParticlesTreePmKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                    currentView, nextView, numParticles, _device.d_octreeNodeHot,
                    _device.d_octreeNodeNav, _device.d_octreeFirstChild, _device.d_octreeLeafStarts,
                    _device.d_octreeLeafCounts, rootIndex, _device.g_dOctreeLeafIndices, forceLaw,
                    deltaTime, _physicsMaxAcceleration,
                    _octreeOpeningCriterion == OctreeOpeningCriterion::Bounds ? 1 : 0, treePmGrid,
                    _device.d_treePmAccelX, _device.d_treePmAccelY, _device.d_treePmAccelZ,
                    treePmCutoffSquared);
                if (!checkCudaStatus(cudaGetLastError(), "updateParticlesTreePm kernel launch")) {
                    return false;
                }
            }
            else {
                updateParticlesOctree<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                    currentView, nextView, numParticles, _device.d_octreeNodeHot,
                    _device.d_octreeNodeNav, _device.d_octreeFirstChild, _device.d_octreeLeafStarts,
                    _device.d_octreeLeafCounts, rootIndex, _device.g_dOctreeLeafIndices, forceLaw,
                    deltaTime, _physicsMaxAcceleration,
                    _octreeOpeningCriterion == OctreeOpeningCriterion::Bounds ? 1 : 0, 0.0f);
                if (!checkCudaStatus(cudaGetLastError(), "updateParticlesOctree kernel launch")) {
                    return false;
                }
            }
            if (!checkCudaStatus(cudaDeviceSynchronize(), "updateParticlesOctree kernel sync")) {
                return false;
            }
            if (profileFlashMode) {
                const auto forceStopTime = std::chrono::high_resolution_clock::now();
                const double forceMs =
                    std::chrono::duration<double, std::milli>(forceStopTime - forceStartTime)
                        .count();
                fprintf(stderr, "[octree-profile] computeBarnesHutForce_ms=%.3f\n", forceMs);
            }

            // Swap buffers
            std::swap(_device.d_soaPosX, _device.d_soaNextPosX);
            std::swap(_device.d_soaPosY, _device.d_soaNextPosY);
            std::swap(_device.d_soaPosZ, _device.d_soaNextPosZ);
            std::swap(_device.d_soaVelX, _device.d_soaNextVelX);
            std::swap(_device.d_soaVelY, _device.d_soaNextVelY);
            std::swap(_device.d_soaVelZ, _device.d_soaNextVelZ);
            _device._leapfrogPrimed = false;
            if (treePmGraphRequested && _device._treePmGraphCaptured[_device._treePmGraphSlot]) {
                _device._treePmGraphSlot ^= 1;
            }
        }

        float scaleRatio = 1.0f;
        float previousHubble = 0.0f;
        float nextHubble = 0.0f;
        if (prepareCosmologyStep(deltaTime, scaleRatio, previousHubble, nextHubble)) {
            applyCosmologyExpansionKernel<<<adaptiveNumBlocks, Particle::kDefaultCudaBlockSize>>>(
                getSoAView(false), numParticles, scaleRatio, previousHubble, nextHubble);
            if (!checkCudaStatus(cudaGetLastError(), "cosmology expansion kernel launch") ||
                !checkCudaStatus(cudaDeviceSynchronize(), "cosmology expansion kernel sync")) {
                return false;
            }
        }
        if (!applySphCorrection(false)) {
            return false;
        }
        _device._hostStateDirty = true;
        if (thermalActive) {
            if (!syncParticlesFromDevice()) {
                return false;
            }
            applyThermalModel(deltaTime);
            syncDeviceState();
        }
        publishMappedMetrics(deltaTime);
        return true;
    }

    constexpr bool kProfileLogsEnabled = BLITZAR_PROFILE_LOGS != 0;
    cudaEvent_t start = nullptr;
    cudaEvent_t stop = nullptr;
    if constexpr (kProfileLogsEnabled) {
        cudaEventCreate(&start);
        cudaEventCreate(&stop);
        cudaEventRecord(start);
    }

    if (!_device._cudaRuntimeAvailable || !_device.d_soaPosX) {
        return false;
    }

    const int numParticles = static_cast<int>(_particles.size());
    const int numBlocks =
        (numParticles + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    constexpr int kPairwiseCudaBlockSize = 128;
    constexpr std::size_t kPairwiseSharedBytes = 4u * kPairwiseCudaBlockSize * sizeof(float);
    const int pairwiseBlocks = (numParticles + kPairwiseCudaBlockSize - 1) / kPairwiseCudaBlockSize;

    ParticleSoAView currentView = getSoAView(false);
    ParticleSoAView nextView = getSoAView(true);

    const auto launchPairwiseAcceleration = [&](ParticleSoAView view, Vector3* output) {
        CudaJitRequest jitRequest;
        jitRequest.family = CudaJitFamily::ForceTile;
        jitRequest.blockSize = kPairwiseCudaBlockSize;
        jitRequest.tileSize = kPairwiseCudaBlockSize;
        jitRequest.softeningMode = forceLaw.softening > 0.0f ? 1 : 0;
        jitRequest.softening = forceLaw.softening;
        CudaJitMetrics jitMetrics;
        const bool usedJit = _device._cudaJit != nullptr &&
                             _device._cudaJit->launchForceTile(
                                 view.posX, view.posY, view.posZ, view.mass, output, numParticles,
                                 forceLaw.softening, forceLaw.minDistance2, _physicsMaxAcceleration,
                                 jitRequest, &jitMetrics);
        if (_device._cudaJit != nullptr && !_device._cudaJitForceMarkerPrinted) {
            fprintf(stderr,
                    "[cuda-jit] family=force_tile backend=%s cache=%s accepted=%u registers=%u "
                    "shared_bytes=%u active_blocks_sm=%u occupancy=%.3f "
                    "divergent_warp_fraction=%.6f warmup_static_ms=%.4f "
                    "warmup_jit_ms=%.4f compile_ms=%.3f\n",
                    usedJit ? "jit" : "static-fallback",
                    jitMetrics.cacheSource.empty() ? "unknown" : jitMetrics.cacheSource.c_str(),
                    jitMetrics.warmupAccepted ? 1u : 0u, jitMetrics.registersPerThread,
                    jitMetrics.staticSharedBytes, jitMetrics.activeBlocksPerSm,
                    jitMetrics.occupancy, jitMetrics.divergentWarpFraction, jitMetrics.staticMs,
                    jitMetrics.jitMs, jitMetrics.compileMs);
            _device._cudaJitForceMarkerPrinted = true;
        }
        if (usedJit) {
            return true;
        }
        computePairwiseAccelerationKernelTiled<<<pairwiseBlocks, kPairwiseCudaBlockSize,
                                                 kPairwiseSharedBytes>>>(
            view, output, numParticles, forceLaw, _physicsMaxAcceleration);
        return checkCudaStatus(cudaGetLastError(), "computePairwiseAccelerationKernelTiled launch");
    };

    if (_adaptiveTimeStepsEnabled && !_adaptiveTimeStepCostGuard &&
        _solverMode == SolverMode::PairwiseCuda) {
        const std::uint32_t levelCount = std::min<std::uint32_t>(_adaptiveTimeStepMaxLevel, 12u);
        const std::uint32_t sliceCount = 1u << levelCount;
        const float quantum = deltaTime / static_cast<float>(sliceCount);
        if (quantum <= 0.0f || !ensureAdaptiveCudaScratchCapacity(numParticles)) {
            fprintf(stderr, "[adaptive] CUDA scratch allocation failed\n");
            return false;
        }
        const bool resetSchedule =
            _adaptiveTimeStepTick == 0u || std::abs(_adaptiveTimeStepQuantum - quantum) > 1.0e-12f;
        if (resetSchedule) {
            if (!launchPairwiseAcceleration(currentView, _device.d_adaptiveAcceleration)) {
                return false;
            }
            initializeAdaptiveScheduleKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, _device.d_adaptiveAcceleration, _device.d_adaptiveLevels,
                _device.d_adaptiveLastForceTicks, numParticles, static_cast<int>(levelCount),
                _adaptiveTimeStepEta, std::max(_octreeSoftening, _physicsMinSoftening), deltaTime);
            if (!checkCudaStatus(cudaGetLastError(), "adaptive pairwise schedule launch")) {
                return false;
            }
            _adaptiveTimeStepQuantum = quantum;
        }

        for (std::uint32_t slice = 0u; slice < sliceCount; ++slice) {
            const unsigned long long targetTick =
                static_cast<unsigned long long>(_adaptiveTimeStepTick + slice + 1u);
            adaptiveDriftKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, nextView, _device.d_adaptiveAcceleration, numParticles, quantum,
                _sphMaxSpeed);
            if (!checkCudaStatus(cudaGetLastError(), "adaptive pairwise drift launch")) {
                return false;
            }
            adaptivePairwiseCorrectKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                nextView, _device.d_adaptiveAcceleration, _device.d_adaptiveLevels,
                _device.d_adaptiveLastForceTicks, numParticles, forceLaw, _physicsMaxAcceleration,
                quantum, static_cast<int>(levelCount), _adaptiveTimeStepEta,
                std::max(_octreeSoftening, _physicsMinSoftening), deltaTime, targetTick,
                _sphMaxSpeed);
            if (!checkCudaStatus(cudaGetLastError(), "adaptive pairwise correction launch")) {
                return false;
            }
            std::swap(_device.d_soaPosX, _device.d_soaNextPosX);
            std::swap(_device.d_soaPosY, _device.d_soaNextPosY);
            std::swap(_device.d_soaPosZ, _device.d_soaNextPosZ);
            std::swap(_device.d_soaVelX, _device.d_soaNextVelX);
            std::swap(_device.d_soaVelY, _device.d_soaNextVelY);
            std::swap(_device.d_soaVelZ, _device.d_soaNextVelZ);
            currentView = getSoAView(false);
            nextView = getSoAView(true);
        }
        if (!checkCudaStatus(cudaDeviceSynchronize(), "adaptive pairwise sync")) {
            return false;
        }
        _adaptiveTimeStepTick += sliceCount;
        _device._hostStateDirty = true;
        if (!applySphCorrection(false)) {
            return false;
        }
        if (thermalActive) {
            if (!syncParticlesFromDevice()) {
                return false;
            }
            applyThermalModel(deltaTime);
            syncDeviceState();
        }
        if (!_adaptiveTimeStepMarkerPrinted) {
            fprintf(stderr,
                    "[adaptive] backend=cuda_native solver=pairwise_cuda scheduler=dyadic "
                    "max_level=%u eta=%.4f\n",
                    levelCount, _adaptiveTimeStepEta);
            _adaptiveTimeStepMarkerPrinted = true;
        }
        publishMappedMetrics(deltaTime);
        return true;
    }

    if (_integratorMode == IntegratorMode::Rk4 || _integratorMode == IntegratorMode::Leapfrog) {
        if (!_device.d_stage || !_device.d_k1x || !_device.d_k2x || !_device.d_k3x ||
            !_device.d_k4x || !_device.d_k1v || !_device.d_k2v || !_device.d_k3v ||
            !_device.d_k4v) {
            if (!allocateRk4Buffers(numParticles)) {
                fprintf(stderr, "[integrator] advanced integrator buffers missing\n");
                return false;
            }
        }
        if (_integratorMode == IntegratorMode::Leapfrog && !_device.d_vHalf) {
            if (!allocateRk4Buffers(numParticles)) {
                fprintf(stderr, "[integrator] leapfrog v_half buffer missing\n");
                return false;
            }
        }
    }

    if (_integratorMode == IntegratorMode::Rk4) {
        extractVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView, _device.d_k1x, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "extractVelocity k1 launch")) {
            return false;
        }
        if (!launchPairwiseAcceleration(currentView, _device.d_k1v)) {
            return false;
        }

        buildRk4StageKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView, _device.d_k1x, _device.d_k1v, 0.5f * deltaTime, nextView, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "buildStage k2 launch")) {
            return false;
        }
        extractVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            nextView, _device.d_k2x, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "extractVelocity k2 launch")) {
            return false;
        }
        if (!launchPairwiseAcceleration(nextView, _device.d_k2v)) {
            return false;
        }

        buildRk4StageKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView, _device.d_k2x, _device.d_k2v, 0.5f * deltaTime, nextView, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "buildStage k3 launch")) {
            return false;
        }
        extractVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            nextView, _device.d_k3x, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "extractVelocity k3 launch")) {
            return false;
        }
        if (!launchPairwiseAcceleration(nextView, _device.d_k3v)) {
            return false;
        }

        buildRk4StageKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView, _device.d_k3x, _device.d_k3v, deltaTime, nextView, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "buildStage k4 launch")) {
            return false;
        }
        extractVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            nextView, _device.d_k4x, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "extractVelocity k4 launch")) {
            return false;
        }
        if (!launchPairwiseAcceleration(nextView, _device.d_k4v)) {
            return false;
        }

        finalizeRk4Kernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView, _device.d_k1x, _device.d_k2x, _device.d_k3x, _device.d_k4x, _device.d_k1v,
            _device.d_k2v, _device.d_k3v, _device.d_k4v, deltaTime, nextView, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "finalizeRk4 launch")) {
            return false;
        }
        if (!checkCudaStatus(cudaDeviceSynchronize(), "rk4 kernel sync")) {
            return false;
        }
    }
    else if (_integratorMode == IntegratorMode::Leapfrog) {
        auto* halfVelocity = reinterpret_cast<float3*>(_device.d_vHalf);
        if (!_device._leapfrogPrimed) {
            primeHalfVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, halfVelocity, numParticles);
            if (!checkCudaStatus(cudaGetLastError(), "pairwise primeHalfVelocityKernel launch")) {
                return false;
            }
            _device._leapfrogPrimed = true;
        }

        if (!launchPairwiseAcceleration(currentView, _device.d_k1v)) {
            return false;
        }

        applyKickHalfStepKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView, _device.d_k1v, deltaTime, halfVelocity, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "pairwise applyKickHalfStepKernel launch")) {
            return false;
        }

        driftWithHalfVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView, halfVelocity, deltaTime, nextView, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "pairwise driftWithHalfVelocityKernel launch")) {
            return false;
        }
        if (!checkCudaStatus(cudaDeviceSynchronize(), "pairwise leapfrog drift sync")) {
            return false;
        }

        std::swap(_device.d_soaPosX, _device.d_soaNextPosX);
        std::swap(_device.d_soaPosY, _device.d_soaNextPosY);
        std::swap(_device.d_soaPosZ, _device.d_soaNextPosZ);
        std::swap(_device.d_soaVelX, _device.d_soaNextVelX);
        std::swap(_device.d_soaVelY, _device.d_soaNextVelY);
        std::swap(_device.d_soaVelZ, _device.d_soaNextVelZ);

        currentView = getSoAView(false);
        nextView = getSoAView(true);

        if (!launchPairwiseAcceleration(currentView, _device.d_k2v)) {
            return false;
        }

        finalizeLeapfrogKickKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView, halfVelocity, _device.d_k2v, deltaTime, nextView, halfVelocity,
            numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "pairwise finalizeLeapfrogKickKernel launch")) {
            return false;
        }
        if (!checkCudaStatus(cudaDeviceSynchronize(), "pairwise leapfrog finalize sync")) {
            return false;
        }
    }
    else {
        _device._leapfrogPrimed = false;
        if (_solverMode == SolverMode::PairwiseCuda) {
            if (!_device.d_k1v && !allocateRk4Buffers(numParticles)) {
                fprintf(stderr, "[pairwise] acceleration scratch allocation failed\n");
                return false;
            }
            if (!launchPairwiseAcceleration(currentView, _device.d_k1v)) {
                return false;
            }
            updateParticlesWithAcceleration<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, nextView, _device.d_k1v, numParticles, deltaTime);
        }
        else {
            updateParticles<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, nextView, numParticles, deltaTime, forceLaw, _physicsMaxAcceleration);
        }
        if (!checkCudaStatus(cudaGetLastError(), "updateParticles kernel launch")) {
            return false;
        }
        if (!checkCudaStatus(cudaDeviceSynchronize(), "updateParticles kernel sync")) {
            return false;
        }
    }

    // Swap buffers
    std::swap(_device.d_soaPosX, _device.d_soaNextPosX);
    std::swap(_device.d_soaPosY, _device.d_soaNextPosY);
    std::swap(_device.d_soaPosZ, _device.d_soaNextPosZ);
    std::swap(_device.d_soaVelX, _device.d_soaNextVelX);
    std::swap(_device.d_soaVelY, _device.d_soaNextVelY);
    std::swap(_device.d_soaVelZ, _device.d_soaNextVelZ);

    float scaleRatio = 1.0f;
    float previousHubble = 0.0f;
    float nextHubble = 0.0f;
    if (prepareCosmologyStep(deltaTime, scaleRatio, previousHubble, nextHubble)) {
        applyCosmologyExpansionKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            getSoAView(false), numParticles, scaleRatio, previousHubble, nextHubble);
        if (!checkCudaStatus(cudaGetLastError(), "cosmology expansion kernel launch") ||
            !checkCudaStatus(cudaDeviceSynchronize(), "cosmology expansion kernel sync")) {
            return false;
        }
    }

    if (!applySphCorrection(false)) {
        return false;
    }
    _device._hostStateDirty = true;
    if (thermalActive) {
        if (!syncParticlesFromDevice()) {
            return false;
        }
        applyThermalModel(deltaTime);
        syncDeviceState();
    }

    if constexpr (kProfileLogsEnabled) {
        cudaEventRecord(stop);
        cudaEventSynchronize(stop);
        float milliseconds = 0;
        cudaEventElapsedTime(&milliseconds, start, stop);
        printf("Time elapsed: %f ms (%f fps) for computing %zu particles\n", milliseconds,
               1000.0f / milliseconds, _particles.size());
    }
    publishMappedMetrics(deltaTime);
    return true;
}

// Note: destroyParticles logic moved to ParticleSystem destructor and releaseParticleBuffers.
