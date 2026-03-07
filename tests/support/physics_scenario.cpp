#include "tests/support/physics_scenario.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <thread>

namespace testsupport {
namespace grav_test_physics_scenario {

bool waitForStep(SimulationBackend &backend, std::uint64_t targetStep, int timeoutMs)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        if (backend.getStats().steps >= targetStep) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return backend.getStats().steps >= targetStep;
}

bool waitForSnapshot(SimulationBackend &backend, std::vector<RenderParticle> &out, int timeoutMs)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        if (backend.tryConsumeSnapshot(out)) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return backend.tryConsumeSnapshot(out);
}

} // namespace grav_test_physics_scenario

float distance(const RenderParticle &a, const RenderParticle &b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    const float dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

std::array<float, 3> centerOfMassAll(const std::vector<RenderParticle> &snapshot)
{
    double totalMass = 0.0;
    double cx = 0.0;
    double cy = 0.0;
    double cz = 0.0;
    for (const RenderParticle &p : snapshot) {
        const double mass = static_cast<double>(std::max(1e-9f, p.mass));
        totalMass += mass;
        cx += static_cast<double>(p.x) * mass;
        cy += static_cast<double>(p.y) * mass;
        cz += static_cast<double>(p.z) * mass;
    }
    if (totalMass <= 1e-12) {
        return {0.0f, 0.0f, 0.0f};
    }
    return {
        static_cast<float>(cx / totalMass),
        static_cast<float>(cy / totalMass),
        static_cast<float>(cz / totalMass)
    };
}

float averageRadius(const std::vector<RenderParticle> &snapshot)
{
    if (snapshot.empty()) {
        return 0.0f;
    }
    double radiusSum = 0.0;
    for (const RenderParticle &p : snapshot) {
        const double x = static_cast<double>(p.x);
        const double y = static_cast<double>(p.y);
        const double z = static_cast<double>(p.z);
        radiusSum += std::sqrt(x * x + y * y + z * z);
    }
    return static_cast<float>(radiusSum / static_cast<double>(snapshot.size()));
}

bool runScenario(const ScenarioConfig &cfg, ScenarioResult &out, std::string &error)
{
    SimulationBackend backend(std::max<std::uint32_t>(2u, cfg.particleCount), cfg.dt);
    backend.setSolverMode(cfg.solver);
    backend.setIntegratorMode(cfg.integrator);
    backend.setOctreeParameters(cfg.octreeTheta, cfg.octreeSoftening);
    backend.setDt(cfg.dt);
    backend.setParticleCount(std::max<std::uint32_t>(2u, cfg.particleCount));
    backend.setSphEnabled(false);
    backend.setSphParameters(1.25f, 1.0f, 4.0f, 0.08f);

    const std::uint32_t energyEvery = std::max<std::uint32_t>(1u, cfg.energyMeasureEverySteps);
    const std::uint32_t energySampleLimit = std::max<std::uint32_t>(
        64u,
        cfg.energySampleLimit == 0u ? std::max<std::uint32_t>(cfg.particleCount, 64u) : cfg.energySampleLimit);
    backend.setEnergyMeasurementConfig(energyEvery, energySampleLimit);

    if (!cfg.inputPath.empty()) {
        backend.setInitialStateFile(cfg.inputPath, cfg.inputFormat.empty() ? "auto" : cfg.inputFormat);
    }

    InitialStateConfig init = cfg.initState;
    if (init.mode.empty()) {
        init.mode = cfg.inputPath.empty() ? "disk_orbit" : "file";
    }
    backend.setInitialStateConfig(init);
    backend.setPaused(true);
    backend.start();

    if (!grav_test_physics_scenario::waitForSnapshot(backend, out.initial, cfg.snapshotTimeoutMs)) {
        backend.stop();
        error = "initial snapshot timeout";
        return false;
    }
    if (out.initial.size() < 2) {
        backend.stop();
        error = "initial snapshot has less than 2 particles";
        return false;
    }

    for (std::uint32_t s = 0; s < cfg.steps; ++s) {
        backend.stepOnce();
        if (!grav_test_physics_scenario::waitForStep(backend, static_cast<std::uint64_t>(s) + 1ull, cfg.stepTimeoutMs)) {
            backend.stop();
            error = "step timeout at " + std::to_string(s + 1);
            return false;
        }
        const SimulationStats stats = backend.getStats();
        if (std::isfinite(stats.energyDriftPct)) {
            out.maxAbsEnergyDriftPct = std::max(out.maxAbsEnergyDriftPct, std::abs(stats.energyDriftPct));
        }
    }

    std::vector<RenderParticle> latest;
    if (!grav_test_physics_scenario::waitForSnapshot(backend, latest, cfg.snapshotTimeoutMs)) {
        backend.stop();
        error = "final snapshot timeout";
        return false;
    }
    out.final = latest;
    out.stats = backend.getStats();
    backend.stop();

    if (out.final.size() < 2) {
        error = "final snapshot has less than 2 particles";
        return false;
    }
    if (out.stats.steps < cfg.steps) {
        error = "final step count too low";
        return false;
    }
    return true;
}

std::string getTwoBodyInputPath()
{
#ifndef GRAVITY_TEST_SOURCE_DIR
    return {};
#else
    return (std::filesystem::path(GRAVITY_TEST_SOURCE_DIR) / "tests" / "data" / "two_body_rest.xyz").string();
#endif
}

#if defined(GRAVITY_ENABLE_GTEST_FIXTURE)
void PhysicsTest::SetUp()
{
    inputPath_ = getTwoBodyInputPath();
    ASSERT_FALSE(inputPath_.empty()) << "GRAVITY_TEST_SOURCE_DIR is not defined";
    ASSERT_TRUE(std::filesystem::exists(inputPath_)) << "Missing test data file: " << inputPath_;
}
#endif

} // namespace testsupport

