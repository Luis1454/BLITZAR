#include "tests/support/physics_test_utils.hpp"
#include <chrono>
#include <functional>
#include <thread>
namespace testsupport {
static void applyPassiveThermalDefaults(InitialStateConfig& initState)
{
    initState.thermalAmbientTemperature = 0.0f;
    initState.thermalSpecificHeat = 1.0f;
    initState.thermalHeatingCoeff = 0.0f;
    initState.thermalRadiationCoeff = 0.0f;
}
static bool waitForSnapshot(const std::function<bool(std::vector<RenderParticle>&)>& snapshotReader,
                            std::vector<RenderParticle>& outSnapshot, int timeoutMs)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        if (snapshotReader(outSnapshot)) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return snapshotReader(outSnapshot);
}
void setScenarioTiming(ScenarioConfig& cfg, int snapshotTimeoutMs, int stepTimeoutMs)
{
    cfg.snapshotTimeoutMs = snapshotTimeoutMs;
    cfg.stepTimeoutMs = stepTimeoutMs;
}
void setScenarioEnergySampling(ScenarioConfig& cfg, std::uint32_t measureEverySteps,
                               std::uint32_t sampleLimit)
{
    cfg.energyMeasureEverySteps = measureEverySteps;
    cfg.energySampleLimit = sampleLimit;
}
bool buildTwoBodyFileScenario(ScenarioConfig& outConfig, std::uint32_t steps, float dt,
                              const std::string& solver, const std::string& integrator,
                              std::string& error)
{
    outConfig = ScenarioConfig{};
    if (!prepareTwoBodyScenario(outConfig, error)) {
        return false;
    }
    outConfig.particleCount = 2u;
    outConfig.steps = steps;
    outConfig.dt = dt;
    outConfig.solver = solver;
    outConfig.integrator = integrator;
    return true;
}
ScenarioConfig buildDiskOrbitScenario(std::uint32_t particleCount, float dt, std::uint32_t steps,
                                      std::uint32_t seed, const std::string& solver,
                                      const std::string& integrator)
{
    ScenarioConfig cfg{};
    cfg.particleCount = particleCount;
    cfg.dt = dt;
    cfg.steps = steps;
    cfg.solver = solver;
    cfg.integrator = integrator;
    cfg.initState.mode = "disk_orbit";
    cfg.initState.seed = seed;
    cfg.initState.includeCentralBody = true;
    cfg.initState.centralMass = 1.0f;
    cfg.initState.diskRadiusMin = 1.5f;
    cfg.initState.diskRadiusMax = 11.5f;
    cfg.initState.diskThickness = 0.0f;
    cfg.initState.velocityScale = 1.0f;
    applyPassiveThermalDefaults(cfg.initState);
    return cfg;
}
ScenarioConfig buildRandomCloudScenario(std::uint32_t particleCount, float dt, std::uint32_t steps,
                                        std::uint32_t seed, const std::string& solver,
                                        const std::string& integrator)
{
    ScenarioConfig cfg{};
    cfg.particleCount = particleCount;
    cfg.dt = dt;
    cfg.steps = steps;
    cfg.solver = solver;
    cfg.integrator = integrator;
    cfg.initState.mode = "random_cloud";
    cfg.initState.seed = seed;
    cfg.initState.includeCentralBody = false;
    applyPassiveThermalDefaults(cfg.initState);
    return cfg;
}
bool waitForStepCount(const SimulationServer& server, std::uint64_t targetStep, int timeoutMs)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        if (server.getStats().steps >= targetStep) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return server.getStats().steps >= targetStep;
}
bool waitForConsumedSnapshot(SimulationServer& server, std::vector<RenderParticle>& outSnapshot,
                             int timeoutMs)
{
    return waitForSnapshot(
        [&server](std::vector<RenderParticle>& snapshot) {
            return server.tryConsumeSnapshot(snapshot);
        },
        outSnapshot, timeoutMs);
}
bool waitForPublishedSnapshot(const SimulationServer& server,
                              std::vector<RenderParticle>& outSnapshot, int timeoutMs)
{
    return waitForSnapshot(
        [&server](std::vector<RenderParticle>& snapshot) {
            return server.copyLatestSnapshot(snapshot);
        },
        outSnapshot, timeoutMs);
}
bool haveExactReplayMatch(const ScenarioResult& lhs, const ScenarioResult& rhs, std::string& error)
{
    if (lhs.final.size() != rhs.final.size()) {
        error = "final snapshot size mismatch";
        return false;
    }
    for (std::size_t i = 0; i < lhs.final.size(); ++i) {
        if (lhs.final[i].x != rhs.final[i].x || lhs.final[i].y != rhs.final[i].y ||
            lhs.final[i].z != rhs.final[i].z)
            error = "particle position mismatch at index " + std::to_string(i);
        return false;
    }
    if (lhs.stats.totalEnergy != rhs.stats.totalEnergy)
        error = "total energy mismatch";
    return false;
    if (lhs.stats.steps != rhs.stats.steps)
        error = "step count mismatch";
    return false;
    error.clear();
    return true;
}
} // namespace testsupport
