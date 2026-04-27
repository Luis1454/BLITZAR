// File: engine/src/server/simulation_server/SnapshotAndEnergy.cpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#include "Internal.hpp"
void SimulationServer::publishSnapshot()
{
    if (!_system) {
        return;
    }
    const bool telemetryEnabled = _gpuTelemetryEnabled.load(std::memory_order_relaxed);
    const auto copyStart = std::chrono::steady_clock::now();
    if (!_system->syncHostState()) {
        if (telemetryEnabled) {
            _gpuTelemetryAvailable.store(false, std::memory_order_relaxed);
            _gpuCopyMs.store(0.0f, std::memory_order_relaxed);
        }
        return;
    }
    if (telemetryEnabled) {
        const std::chrono::duration<float, std::milli> copyElapsed =
            std::chrono::steady_clock::now() - copyStart;
        _gpuCopyMs.store(copyElapsed.count(), std::memory_order_relaxed);
    }
    const std::vector<Particle>& particles = _system->getParticles();
    const size_t count = particles.size();
    const std::size_t publishCap =
        static_cast<std::size_t>(_snapshotTransferCap.load(std::memory_order_relaxed));
    const std::size_t publishedCount =
        std::min<std::size_t>(count, std::max<std::size_t>(1u, publishCap));
    if (_scratchSnapshot.size() != publishedCount) {
        _scratchSnapshot.resize(publishedCount);
    }
    const std::size_t stride =
        std::max<std::size_t>(1u, (count + publishedCount - 1u) / publishedCount);
    std::size_t outIndex = 0u;
    for (size_t i = 0; i < count && outIndex < publishedCount; i += stride) {
        _scratchSnapshot[outIndex] =
            RenderParticle{particles[i].getPosition().x,      particles[i].getPosition().y,
                           particles[i].getPosition().z,      particles[i].getMass(),
                           particles[i].getPressure().norm(), particles[i].getTemperature()};
        outIndex += 1u;
    }
    std::lock_guard<std::mutex> lock(_snapshotMutex);
    _publishedSnapshot.swap(_scratchSnapshot);
}
void SimulationServer::clearPublishedSnapshotCache()
{
    std::lock_guard<std::mutex> lock(_snapshotMutex);
    _publishedSnapshot.clear();
    _scratchSnapshot.clear();
}
SimulationServer::EnergyValues SimulationServer::computeEnergyValues()
{
    EnergyValues values{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, false};
    if (!_system)
        return values;
    if (!_system->syncHostState()) {
        values.estimated = true;
        return values;
    }
    const std::vector<Particle>& particles = _system->getParticles();
    const std::size_t n = particles.size();
    if (n < 2)
        return values;
    const std::size_t sampleLimit =
        static_cast<std::size_t>(_energySampleLimit.load(std::memory_order_relaxed));
    const bool sampled = n > sampleLimit;
    const float specificHeat = _system ? std::max(1e-6f, _system->getThermalSpecificHeat()) : 1.0f;
    float energySoftening = 0.0f;
    float energyMinSoftening = 0.0f;
    float energyMinDistance2 = 0.0f;
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        energySoftening = _octreeSoftening;
        energyMinSoftening = _physicsMinSoftening;
        energyMinDistance2 = _physicsMinDistance2;
    }
    std::vector<std::size_t> indices;
    if (!sampled) {
        indices.resize(n);
        for (std::size_t i = 0; i < n; ++i) {
            indices[i] = i;
        }
    }
    else {
        const std::size_t sampleCount = std::max<std::size_t>(64u, sampleLimit);
        const std::size_t stride = std::max<std::size_t>(1, n / sampleCount);
        for (std::size_t i = 0; i < n; i += stride) {
            indices.push_back(i);
            if (indices.size() >= sampleCount) {
                break;
            }
        }
    }
    const double kineticScale =
        sampled ? static_cast<double>(n) / static_cast<double>(indices.size()) : 1.0;
    const double pairCountFull = static_cast<double>(n) * static_cast<double>(n - 1) * 0.5;
    const double pairCountSample =
        static_cast<double>(indices.size()) * static_cast<double>(indices.size() - 1) * 0.5;
    const double potentialScale =
        (sampled && pairCountSample > 0.0) ? (pairCountFull / pairCountSample) : 1.0;
    const float softening = std::max(energySoftening, energyMinSoftening);
    double kinetic = 0.0;
    double thermal = 0.0;
    for (std::size_t idx : indices) {
        const Particle& p = particles[idx];
        const Vector3 v = p.getVelocity();
        const double speed2 = static_cast<double>(v.x * v.x + v.y * v.y + v.z * v.z);
        kinetic += 0.5 * static_cast<double>(p.getMass()) * speed2;
        thermal += static_cast<double>(p.getMass()) * static_cast<double>(specificHeat) *
                   static_cast<double>(std::max(0.0f, p.getTemperature()));
    }
    kinetic *= kineticScale;
    thermal *= kineticScale;
    double potential = 0.0;
    for (std::size_t a = 0; a < indices.size(); ++a) {
        const Particle& p = particles[indices[a]];
        const Vector3 pp = p.getPosition();
        for (std::size_t b = a + 1; b < indices.size(); ++b) {
            const Particle& q = particles[indices[b]];
            const Vector3 qq = q.getPosition();
            const float dx = pp.x - qq.x;
            const float dy = pp.y - qq.y;
            const float dz = pp.z - qq.z;
            const float dist2 = dx * dx + dy * dy + dz * dz + softening * softening;
            if (dist2 <= energyMinDistance2)
                continue;
            const float dist = std::sqrt(dist2);
            if (dist <= 1e-6f)
                continue;
            potential -= static_cast<double>(p.getMass()) * static_cast<double>(q.getMass()) /
                         static_cast<double>(dist);
        }
    }
    potential *= potentialScale;
    values.kinetic = static_cast<float>(kinetic);
    values.potential = static_cast<float>(potential);
    values.thermal = static_cast<float>(thermal);
    values.radiated = _system ? _system->getCumulativeRadiatedEnergy() : 0.0f;
    values.total = values.kinetic + values.potential + values.thermal + values.radiated;
    values.estimated = sampled;
    return values;
}
void SimulationServer::maybeUpdateEnergy(std::uint64_t currentStep)
{
    const std::uint32_t every = _energyMeasureEverySteps.load(std::memory_order_relaxed);
    if (every == 0 || (currentStep % every) != 0) {
        return;
    }
    const EnergyValues values = computeEnergyValues();
    _kineticEnergy.store(values.kinetic, std::memory_order_relaxed);
    _potentialEnergy.store(values.potential, std::memory_order_relaxed);
    _thermalEnergy.store(values.thermal, std::memory_order_relaxed);
    _radiatedEnergy.store(values.radiated, std::memory_order_relaxed);
    _totalEnergy.store(values.total, std::memory_order_relaxed);
    _energyEstimated.store(values.estimated, std::memory_order_relaxed);
    if (!_hasEnergyBaseline) {
        _energyBaseline = values.total;
        _hasEnergyBaseline = true;
        _energyDriftPct.store(0.0f, std::memory_order_relaxed);
        return;
    }
    const float denom = std::max(std::fabs(_energyBaseline), 1e-6f);
    const float drift = ((values.total - _energyBaseline) / denom) * 100.0f;
    _energyDriftPct.store(drift, std::memory_order_relaxed);
}
