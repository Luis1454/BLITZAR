#include "tests/support/physics_scenario.hpp"
#include "tests/support/physics_test_utils.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
namespace testsupport {
namespace grav_test_physics_scenario {
std::filesystem::path twoBodyInputPath()
{
    const std::filesystem::path sourceFile(__FILE__);
    return sourceFile.parent_path().parent_path() / "data" / "two_body_rest.xyz";
}
} // namespace grav_test_physics_scenario
float distance(const RenderParticle& a, const RenderParticle& b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    const float dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}
std::array<float, 3> centerOfMassAll(const std::vector<RenderParticle>& snapshot)
{
    double totalMass = 0.0;
    double cx = 0.0;
    double cy = 0.0;
    double cz = 0.0;
    for (const RenderParticle& p : snapshot) {
        const double mass = static_cast<double>(std::max(1e-9f, p.mass));
        totalMass += mass;
        cx += static_cast<double>(p.x) * mass;
        cy += static_cast<double>(p.y) * mass;
        cz += static_cast<double>(p.z) * mass;
    }
    if (totalMass <= 1e-12)
        return {0.0f, 0.0f, 0.0f};
    return {static_cast<float>(cx / totalMass), static_cast<float>(cy / totalMass),
            static_cast<float>(cz / totalMass)};
}
float averageRadius(const std::vector<RenderParticle>& snapshot)
{
    if (snapshot.empty()) {
        return 0.0f;
    }
    double radiusSum = 0.0;
    for (const RenderParticle& p : snapshot) {
        const double x = static_cast<double>(p.x);
        const double y = static_cast<double>(p.y);
        const double z = static_cast<double>(p.z);
        radiusSum += std::sqrt(x * x + y * y + z * z);
    }
    return static_cast<float>(radiusSum / static_cast<double>(snapshot.size()));
}
bool runScenario(const ScenarioConfig& cfg, ScenarioResult& out, std::string& error)
{
    SimulationServer server(std::max<std::uint32_t>(2u, cfg.particleCount), cfg.dt);
    server.setSolverMode(cfg.solver);
    server.setIntegratorMode(cfg.integrator);
    server.setPerformanceProfile(cfg.performanceProfile);
    server.setOctreeParameters(cfg.octreeTheta, cfg.octreeSoftening);
    server.setDt(cfg.dt);
    server.setParticleCount(std::max<std::uint32_t>(2u, cfg.particleCount));
    server.setSphEnabled(cfg.sphEnabled);
    server.setSphParameters(1.25f, 1.0f, 4.0f, 0.08f);
    const std::uint32_t energyEvery = std::max<std::uint32_t>(1u, cfg.energyMeasureEverySteps);
    const std::uint32_t energySampleLimit = std::max<std::uint32_t>(
        64u, cfg.energySampleLimit == 0u ? std::max<std::uint32_t>(cfg.particleCount, 64u)
                                         : cfg.energySampleLimit);
    server.setEnergyMeasurementConfig(energyEvery, energySampleLimit);
    if (!cfg.inputPath.empty()) {
        server.setInitialStateFile(cfg.inputPath,
                                   cfg.inputFormat.empty() ? "auto" : cfg.inputFormat);
    }
    InitialStateConfig init = cfg.initState;
    if (init.mode.empty()) {
        init.mode = cfg.inputPath.empty() ? "disk_orbit" : "file";
    }
    server.setInitialStateConfig(init);
    server.setPaused(true);
    server.start();
    if (!waitForConsumedSnapshot(server, out.initial, cfg.snapshotTimeoutMs)) {
        server.stop();
        error = "initial snapshot timeout";
        return false;
    }
    if (out.initial.size() < 2) {
        server.stop();
        error = "initial snapshot has less than 2 particles";
        return false;
    }
    for (std::uint32_t s = 0; s < cfg.steps; ++s) {
        server.stepOnce();
        if (!waitForStepCount(server, static_cast<std::uint64_t>(s) + 1ull, cfg.stepTimeoutMs)) {
            server.stop();
            error = "step timeout at " + std::to_string(s + 1);
            return false;
        }
        const SimulationStats stats = server.getStats();
        if (std::isfinite(stats.energyDriftPct)) {
            out.maxAbsEnergyDriftPct =
                std::max(out.maxAbsEnergyDriftPct, std::abs(stats.energyDriftPct));
        }
    }
    std::vector<RenderParticle> latest;
    if (!waitForConsumedSnapshot(server, latest, cfg.snapshotTimeoutMs)) {
        server.stop();
        error = "final snapshot timeout";
        return false;
    }
    out.final = latest;
    out.stats = server.getStats();
    server.stop();
    if (out.final.size() < 2) {
        error = "final snapshot has less than 2 particles";
        return false;
    }
    if (out.stats.steps < cfg.steps) {
        error = "final step count too low";
        return false;
    }
    const std::size_t comparableCount = std::min(out.initial.size(), out.final.size());
    for (std::size_t index = 0; index < comparableCount; index += 1u) {
        out.maxParticleDeltaFromInitial = std::max(out.maxParticleDeltaFromInitial,
                                                   distance(out.initial[index], out.final[index]));
    }
    return true;
}
std::string getTwoBodyInputPath()
{
    return grav_test_physics_scenario::twoBodyInputPath().string();
}
bool prepareTwoBodyScenario(ScenarioConfig& cfg, std::string& error)
{
    const std::filesystem::path inputPath = grav_test_physics_scenario::twoBodyInputPath();
    if (!std::filesystem::exists(inputPath)) {
        error = "Missing test data file: " + inputPath.string();
        return false;
    }
    cfg.inputPath = inputPath.string();
    cfg.initState.mode = "file";
    cfg.initState.thermalAmbientTemperature = 0.0f;
    cfg.initState.thermalSpecificHeat = 1.0f;
    cfg.initState.thermalHeatingCoeff = 0.0f;
    cfg.initState.thermalRadiationCoeff = 0.0f;
    return true;
}
bool prepareGeneratedCalibrationScenario(const std::string& mode, ScenarioConfig& cfg,
                                         std::string& error)
{
    cfg.inputPath.clear();
    cfg.inputFormat = "auto";
    cfg.initState = InitialStateConfig{};
    cfg.initState.mode = mode;
    cfg.initState.includeCentralBody = false;
    cfg.initState.thermalAmbientTemperature = 0.0f;
    cfg.initState.thermalSpecificHeat = 1.0f;
    cfg.initState.thermalHeatingCoeff = 0.0f;
    cfg.initState.thermalRadiationCoeff = 0.0f;
    cfg.energyMeasureEverySteps = 1u;
    if (mode == "two_body") {
        cfg.particleCount = 2u;
        cfg.integrator = "rk4";
        cfg.dt = 0.002f;
        cfg.steps = 600u;
        cfg.initState.cloudHalfExtent = 6.0f;
        cfg.initState.particleMass = 1.0f;
        cfg.initState.velocityScale = 1.0f;
        return true;
    }
    if (mode == "three_body") {
        cfg.particleCount = 3u;
        cfg.integrator = "rk4";
        cfg.dt = 0.0015f;
        cfg.steps = 400u;
        cfg.initState.cloudHalfExtent = 1.8f;
        cfg.initState.particleMass = 1.0f;
        cfg.initState.velocityScale = 1.0f;
        return true;
    }
    if (mode == "plummer_sphere") {
        cfg.particleCount = 96u;
        cfg.integrator = "euler";
        cfg.dt = 0.001f;
        cfg.steps = 80u;
        cfg.snapshotTimeoutMs = 6000;
        cfg.stepTimeoutMs = 6000;
        cfg.initState.cloudHalfExtent = 4.0f;
        cfg.initState.particleMass = 0.01f;
        cfg.initState.velocityScale = 0.75f;
        return true;
    }
    error = "unknown generated calibration mode: " + mode;
    return false;
}
} // namespace testsupport
