/*
 * @file engine/server/simulation/telemetry/SrvSnapshotAndEnergy.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Source artifact for the BLITZAR simulation project.
 */

#include "server/simulation/runtime/SrvInternal.hpp"

namespace blitzar_simulation_snapshot_energy {
constexpr int kDensityGridSide = 24;
constexpr std::size_t kDensityCellCount = static_cast<std::size_t>(kDensityGridSide) *
                                          static_cast<std::size_t>(kDensityGridSide) *
                                          static_cast<std::size_t>(kDensityGridSide);

struct DensityGrid {
    std::array<float, kDensityCellCount> mass{};
    double totalMass = 0.0;
    float minX = 0.0f;
    float minY = 0.0f;
    float minZ = 0.0f;
    float scaleX = 0.0f;
    float scaleY = 0.0f;
    float scaleZ = 0.0f;
    float meanCellMass = 0.0f;
    bool valid = false;
};

int densityCell(float value, float minimum, float scale)
{
    return std::clamp(static_cast<int>((value - minimum) * scale), 0, kDensityGridSide - 1);
}

std::size_t densityIndex(int x, int y, int z)
{
    return static_cast<std::size_t>(x + kDensityGridSide * (y + kDensityGridSide * z));
}

DensityGrid buildDensityGrid(const std::vector<Particle>& particles)
{
    DensityGrid grid;
    if (particles.empty()) {
        return grid;
    }
    grid.minX = std::numeric_limits<float>::max();
    grid.minY = std::numeric_limits<float>::max();
    grid.minZ = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    float maxZ = std::numeric_limits<float>::lowest();
    double totalMass = 0.0;
    for (const Particle& particle : particles) {
        const Vector3 position = particle.getPosition();
        grid.minX = std::min(grid.minX, position.x);
        grid.minY = std::min(grid.minY, position.y);
        grid.minZ = std::min(grid.minZ, position.z);
        maxX = std::max(maxX, position.x);
        maxY = std::max(maxY, position.y);
        maxZ = std::max(maxZ, position.z);
        totalMass += std::max(0.0f, particle.getMass());
    }
    const float rangeX = maxX - grid.minX;
    const float rangeY = maxY - grid.minY;
    const float rangeZ = maxZ - grid.minZ;
    if (totalMass <= 0.0f || rangeX <= 1.0e-6f || rangeY <= 1.0e-6f || rangeZ <= 1.0e-6f) {
        return grid;
    }
    grid.scaleX = static_cast<float>(kDensityGridSide) / rangeX;
    grid.scaleY = static_cast<float>(kDensityGridSide) / rangeY;
    grid.scaleZ = static_cast<float>(kDensityGridSide) / rangeZ;
    grid.totalMass = totalMass;
    grid.meanCellMass = static_cast<float>(totalMass / static_cast<double>(kDensityCellCount));
    for (const Particle& particle : particles) {
        const Vector3 position = particle.getPosition();
        const int x = densityCell(position.x, grid.minX, grid.scaleX);
        const int y = densityCell(position.y, grid.minY, grid.scaleY);
        const int z = densityCell(position.z, grid.minZ, grid.scaleZ);
        grid.mass[densityIndex(x, y, z)] += std::max(0.0f, particle.getMass());
    }
    grid.valid = true;
    return grid;
}

float densityNormAt(const DensityGrid& grid, const Vector3& position)
{
    if (!grid.valid || grid.meanCellMass <= 0.0f) {
        return 0.0f;
    }
    const int centerX = densityCell(position.x, grid.minX, grid.scaleX);
    const int centerY = densityCell(position.y, grid.minY, grid.scaleY);
    const int centerZ = densityCell(position.z, grid.minZ, grid.scaleZ);
    float localMass = 0.0f;
    for (int z = centerZ - 1; z <= centerZ + 1; ++z) {
        for (int y = centerY - 1; y <= centerY + 1; ++y) {
            for (int x = centerX - 1; x <= centerX + 1; ++x) {
                if (x >= 0 && x < kDensityGridSide && y >= 0 && y < kDensityGridSide && z >= 0 &&
                    z < kDensityGridSide) {
                    localMass += grid.mass[densityIndex(x, y, z)];
                }
            }
        }
    }
    const float ratio = std::max(localMass / (27.0f * grid.meanCellMass), 1.0e-6f);
    return std::clamp(0.5f + 0.2f * std::log2(ratio), 0.0f, 1.0f);
}
} // namespace blitzar_simulation_snapshot_energy

/*
 * @brief Documents the publish snapshot operation contract.
 * @param None This contract does not take explicit parameters.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
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
    bool cosmologyEnabled = false;
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        cosmologyEnabled = _configState._initialStateConfig.cosmology.enabled;
    }
    const blitzar_simulation_snapshot_energy::DensityGrid densityGrid =
        cosmologyEnabled ? blitzar_simulation_snapshot_energy::buildDensityGrid(particles)
                         : blitzar_simulation_snapshot_energy::DensityGrid{};
    double totalMass = densityGrid.totalMass;
    if (!cosmologyEnabled) {
        for (const Particle& particle : particles) {
            totalMass += static_cast<double>(std::max(0.0f, particle.getMass()));
        }
    }
    _totalMass.store(static_cast<float>(totalMass), std::memory_order_relaxed);
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
            RenderParticle{particles[i].getPosition().x,
                           particles[i].getPosition().y,
                           particles[i].getPosition().z,
                           particles[i].getMass(),
                           particles[i].getPressure().norm(),
                           particles[i].getTemperature(),
                           blitzar_simulation_snapshot_energy::densityNormAt(
                               densityGrid, particles[i].getPosition())};
        outIndex += 1u;
    }
    std::lock_guard<std::mutex> lock(_snapshotMutex);
    _publishedSnapshot.swap(_scratchSnapshot);
}

/*
 * @brief Documents the clear published snapshot cache operation contract.
 * @param None This contract does not take explicit parameters.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::clearPublishedSnapshotCache()
{
    std::lock_guard<std::mutex> lock(_snapshotMutex);
    _publishedSnapshot.clear();
    _scratchSnapshot.clear();
}

/*
 * @brief Documents the compute energy values operation contract.
 * @param None This contract does not take explicit parameters.
 * @return SimulationServer::EnergyValues SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
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
        energySoftening = _configState._octreeSoftening;
        energyMinSoftening = _configState._physicsMinSoftening;
        energyMinDistance2 = _configState._physicsMinDistance2;
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

/*
 * @brief Documents the maybe update energy operation contract.
 * @param currentStep Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
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

    if (!_configState._hasEnergyBaseline) {
        _configState._energyBaseline = values.total;
        _configState._hasEnergyBaseline = true;
        _energyDriftPct.store(0.0f, std::memory_order_relaxed);
        return;
    }

    const float denom = std::max(std::fabs(_configState._energyBaseline), 1e-6f);
    const float drift = ((values.total - _configState._energyBaseline) / denom) * 100.0f;

    _energyDriftPct.store(drift, std::memory_order_relaxed);
}
