/*
 * @file engine/server/simulation/runtime/SrvPhysics.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Simulation server runtime physics operations.
 */

#include "simulation/SrvInternal.hpp"

/*
 * @brief Documents the set octree parameters operation contract.
 * @param theta Input value used by this contract.
 * @param softening Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::setOctreeParameters(float theta, float softening)
{
    std::lock_guard<std::mutex> lock(_commandMutex);
    if (theta > 0.01f)
        _configState._octreeTheta = theta;
    const float clampedMin = clampThetaBound(_configState._octreeThetaAutoMin);
    const float clampedMax = std::max(clampedMin, clampThetaBound(_configState._octreeThetaAutoMax));
    _configState._octreeEffectiveTheta = std::clamp(theta, clampedMin, clampedMax);
    _configState._runtimeConfigMirror.octreeTheta = _configState._octreeTheta;
    if (softening > 1e-6f)
        _configState._octreeSoftening = softening;
    _configState._runtimeConfigMirror.octreeSoftening = _configState._octreeSoftening;
}

/*
 * @brief Documents the set sph enabled operation contract.
 * @param enabled Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::setSphEnabled(bool enabled)
{
    bool changed = false;
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        if (_configState._sphEnabled != enabled) {
            _configState._sphEnabled = enabled;
            changed = true;
        }
    }
    if (changed && _running.load(std::memory_order_relaxed)) {
        requestReset();
    }
}

/*
 * @brief Documents the set sph parameters operation contract.
 * @param smoothingLength Input value used by this contract.
 * @param restDensity Input value used by this contract.
 * @param gasConstant Input value used by this contract.
 * @param viscosity Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::setSphParameters(float smoothingLength, float restDensity, float gasConstant,
                                        float viscosity)
{
    std::lock_guard<std::mutex> lock(_commandMutex);
    if (smoothingLength > 0.05f)
        _configState._sphSmoothingLength = smoothingLength;
    if (restDensity > 0.01f)
        _configState._sphRestDensity = restDensity;
    if (gasConstant > 0.01f)
        _configState._sphGasConstant = gasConstant;
    if (viscosity >= 0.0f)
        _configState._sphViscosity = viscosity;
}

void SimulationServer::setDeterministicMode(bool enabled)
{
    bool changed = false;
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        if (_configState._deterministicMode != enabled) {
            _configState._deterministicMode = enabled;
            _configState._runtimeConfigMirror.deterministicMode = enabled;
            changed = true;
        }
    }
    if (changed && _running.load(std::memory_order_relaxed)) {
        requestReset();
    }
}

/*
 * @brief Documents the set substep policy operation contract.
 * @param targetDt Input value used by this contract.
 * @param maxSubsteps Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::setSubstepPolicy(float targetDt, std::uint32_t maxSubsteps)
{
    const float safeTargetDt = std::max(0.0f, targetDt);
    const std::uint32_t safeMaxSubsteps = std::max<std::uint32_t>(1u, maxSubsteps);
    _configuredSubstepTargetDt.store(safeTargetDt, std::memory_order_relaxed);
    _configuredMaxSubsteps.store(safeMaxSubsteps, std::memory_order_relaxed);
    std::lock_guard<std::mutex> lock(_commandMutex);
    _configState._runtimeConfigMirror.substepTargetDt = safeTargetDt;
    _configState._runtimeConfigMirror.maxSubsteps = safeMaxSubsteps;
}

/*
 * @brief Documents the set snapshot publish period ms operation contract.
 * @param periodMs Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::setSnapshotPublishPeriodMs(std::uint32_t periodMs)
{
    const std::uint32_t safePeriodMs = std::max<std::uint32_t>(1u, periodMs);
    _snapshotPublishPeriodMs.store(safePeriodMs, std::memory_order_relaxed);
    std::lock_guard<std::mutex> lock(_commandMutex);
    _configState._runtimeConfigMirror.snapshotPublishPeriodMs = safePeriodMs;
}

/*
 * @brief Documents the set snapshot transfer cap operation contract.
 * @param maxPoints Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::setSnapshotTransferCap(std::uint32_t maxPoints)
{
    const std::uint32_t safeMaxPoints = bltzr_protocol::clampSnapshotPoints(maxPoints);
    _snapshotTransferCap.store(resolvePublishedSnapshotCap(safeMaxPoints),
                               std::memory_order_relaxed);
    _configState._runtimeConfigMirror.clientParticleCap = safeMaxPoints;
}

/*
 * @brief Documents the set energy measurement config operation contract.
 * @param everySteps Input value used by this contract.
 * @param sampleLimit Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::setEnergyMeasurementConfig(std::uint32_t everySteps,
                                                  std::uint32_t sampleLimit)
{
    const std::uint32_t safeEverySteps = std::max<std::uint32_t>(1u, everySteps);
    const std::uint32_t safeSampleLimit = std::max<std::uint32_t>(64u, sampleLimit);
    _energyMeasureEverySteps.store(safeEverySteps, std::memory_order_relaxed);
    _energySampleLimit.store(safeSampleLimit, std::memory_order_relaxed);
    std::lock_guard<std::mutex> lock(_commandMutex);
    _configState._runtimeConfigMirror.energyMeasureEverySteps = safeEverySteps;
    _configState._runtimeConfigMirror.energySampleLimit = safeSampleLimit;
}

/*
 * @brief Documents the set gpu telemetry enabled operation contract.
 * @param enabled Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::setGpuTelemetryEnabled(bool enabled)
{
    _gpuTelemetryEnabled.store(enabled, std::memory_order_relaxed);
    if (!enabled) {
        clearGpuTelemetry();
    }
}
