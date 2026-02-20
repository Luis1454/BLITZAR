#include "sim/SimulationBackend.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

namespace {

struct ScenarioConfig {
    std::string inputPath;
    std::string inputFormat = "xyz";
    std::string solver = "pairwise_cuda";
    std::string integrator = "rk4";
    float dt = 0.002f;
    std::uint32_t steps = 100;
    std::uint32_t particleCount = 2u;
    std::uint32_t energyMeasureEverySteps = 1u;
    std::uint32_t energySampleLimit = 0u;
    int snapshotTimeoutMs = 3000;
    int stepTimeoutMs = 3000;
    InitialStateConfig initState{};
};

struct ScenarioResult {
    std::vector<RenderParticle> initial;
    std::vector<RenderParticle> final;
    SimulationStats stats{};
    float maxAbsEnergyDriftPct = 0.0f;
};

float distance(const RenderParticle &a, const RenderParticle &b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    const float dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

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

bool runScenario(const ScenarioConfig &cfg, ScenarioResult &out, std::string &error)
{
    SimulationBackend backend(std::max<std::uint32_t>(2u, cfg.particleCount), cfg.dt);
    backend.setSolverMode(cfg.solver);
    backend.setIntegratorMode(cfg.integrator);
    backend.setDt(cfg.dt);
    backend.setParticleCount(std::max<std::uint32_t>(2u, cfg.particleCount));
    backend.setSphEnabled(false);
    backend.setSphParameters(1.25f, 1.0f, 4.0f, 0.08f);
    const std::uint32_t energyEvery = std::max<std::uint32_t>(1u, cfg.energyMeasureEverySteps);
    const std::uint32_t energySampleLimit = std::max<std::uint32_t>(
        64u,
        cfg.energySampleLimit == 0u ? std::max<std::uint32_t>(cfg.particleCount, 64u) : cfg.energySampleLimit
    );
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

    if (!waitForSnapshot(backend, out.initial, cfg.snapshotTimeoutMs)) {
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
        if (!waitForStep(backend, static_cast<std::uint64_t>(s) + 1ull, cfg.stepTimeoutMs)) {
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
    if (!waitForSnapshot(backend, latest, cfg.snapshotTimeoutMs)) {
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

class PhysicsRegressionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        inputPath_ = getTwoBodyInputPath();
        ASSERT_FALSE(inputPath_.empty()) << "GRAVITY_TEST_SOURCE_DIR is not defined";
        ASSERT_TRUE(std::filesystem::exists(inputPath_)) << "Missing test data file: " << inputPath_;
    }

    std::string inputPath_;
};

TEST_F(PhysicsRegressionTest, AttractionDistance)
{
    ScenarioConfig cfg;
    cfg.inputPath = inputPath_;
    cfg.particleCount = 2u;
    cfg.initState.mode = "file";
    cfg.initState.thermalAmbientTemperature = 0.0f;
    cfg.initState.thermalSpecificHeat = 1.0f;
    cfg.initState.thermalHeatingCoeff = 0.0f;
    cfg.initState.thermalRadiationCoeff = 0.0f;
    cfg.dt = 0.002f;
    cfg.steps = 400;

    ScenarioResult result;
    std::string error;
    ASSERT_TRUE(runScenario(cfg, result, error)) << error;

    const float initialDistance = distance(result.initial[0], result.initial[1]);
    const float finalDistance = distance(result.final[0], result.final[1]);
    const float ratio = finalDistance / std::max(initialDistance, 1e-8f);

    constexpr float kMinRatio = 0.05f;
    constexpr float kMaxRatio = 0.999f;
    EXPECT_GE(ratio, kMinRatio) << "Distance collapsed too much";
    EXPECT_LE(ratio, kMaxRatio) << "No attraction detected";
}

TEST_F(PhysicsRegressionTest, CenterOfMassDrift)
{
    ScenarioConfig cfg;
    cfg.inputPath = inputPath_;
    cfg.particleCount = 2u;
    cfg.initState.mode = "file";
    cfg.initState.thermalAmbientTemperature = 0.0f;
    cfg.initState.thermalSpecificHeat = 1.0f;
    cfg.initState.thermalHeatingCoeff = 0.0f;
    cfg.initState.thermalRadiationCoeff = 0.0f;
    cfg.dt = 0.002f;
    cfg.steps = 400;

    ScenarioResult result;
    std::string error;
    ASSERT_TRUE(runScenario(cfg, result, error)) << error;

    const auto centerOfMass = [](const std::vector<RenderParticle> &snapshot) {
        const float totalMass = snapshot[0].mass + snapshot[1].mass;
        const float cx = (snapshot[0].x * snapshot[0].mass + snapshot[1].x * snapshot[1].mass) / totalMass;
        const float cy = (snapshot[0].y * snapshot[0].mass + snapshot[1].y * snapshot[1].mass) / totalMass;
        const float cz = (snapshot[0].z * snapshot[0].mass + snapshot[1].z * snapshot[1].mass) / totalMass;
        return std::array<float, 3>{cx, cy, cz};
    };

    const auto c0 = centerOfMass(result.initial);
    const auto c1 = centerOfMass(result.final);
    const float drift = std::sqrt(
        (c1[0] - c0[0]) * (c1[0] - c0[0])
        + (c1[1] - c0[1]) * (c1[1] - c0[1])
        + (c1[2] - c0[2]) * (c1[2] - c0[2]));

    constexpr float kMaxCenterOfMassDrift = 1e-2f;
    EXPECT_LE(drift, kMaxCenterOfMassDrift);
}

TEST_F(PhysicsRegressionTest, TimeStepConvergence)
{
    ScenarioConfig coarse;
    coarse.inputPath = inputPath_;
    coarse.particleCount = 2u;
    coarse.initState.mode = "file";
    coarse.initState.thermalAmbientTemperature = 0.0f;
    coarse.initState.thermalSpecificHeat = 1.0f;
    coarse.initState.thermalHeatingCoeff = 0.0f;
    coarse.initState.thermalRadiationCoeff = 0.0f;
    coarse.dt = 0.002f;
    coarse.steps = 100;

    ScenarioConfig fine;
    fine.inputPath = inputPath_;
    fine.particleCount = 2u;
    fine.initState.mode = "file";
    fine.initState.thermalAmbientTemperature = 0.0f;
    fine.initState.thermalSpecificHeat = 1.0f;
    fine.initState.thermalHeatingCoeff = 0.0f;
    fine.initState.thermalRadiationCoeff = 0.0f;
    fine.dt = 0.001f;
    fine.steps = 200;

    ScenarioResult coarseResult;
    std::string coarseError;
    ASSERT_TRUE(runScenario(coarse, coarseResult, coarseError)) << coarseError;

    ScenarioResult fineResult;
    std::string fineError;
    ASSERT_TRUE(runScenario(fine, fineResult, fineError)) << fineError;

    float maxParticleDelta = 0.0f;
    for (std::size_t i = 0; i < 2; ++i) {
        maxParticleDelta = std::max(maxParticleDelta, distance(coarseResult.final[i], fineResult.final[i]));
    }

    constexpr float kMaxConvergenceDelta = 0.05f;
    EXPECT_LE(maxParticleDelta, kMaxConvergenceDelta);
}

TEST_F(PhysicsRegressionTest, EnergyConservation)
{
    ScenarioConfig cfg;
    cfg.inputPath = inputPath_;
    cfg.particleCount = 2u;
    cfg.initState.mode = "file";
    cfg.initState.thermalAmbientTemperature = 0.0f;
    cfg.initState.thermalSpecificHeat = 1.0f;
    cfg.initState.thermalHeatingCoeff = 0.0f;
    cfg.initState.thermalRadiationCoeff = 0.0f;
    cfg.dt = 0.002f;
    cfg.steps = 800;

    ScenarioResult result;
    std::string error;
    ASSERT_TRUE(runScenario(cfg, result, error)) << error;

    const float driftPct = result.maxAbsEnergyDriftPct;
    constexpr float kMaxEnergyDriftPct = 5.0f;
    EXPECT_LE(driftPct, kMaxEnergyDriftPct)
        << "Energy drift too high: " << driftPct << "%";
    EXPECT_FALSE(result.stats.energyEstimated);
}

TEST_F(PhysicsRegressionTest, LongRunStability)
{
    ScenarioConfig cfg;
    cfg.inputPath = inputPath_;
    cfg.particleCount = 2u;
    cfg.initState.mode = "file";
    cfg.initState.thermalAmbientTemperature = 0.0f;
    cfg.initState.thermalSpecificHeat = 1.0f;
    cfg.initState.thermalHeatingCoeff = 0.0f;
    cfg.initState.thermalRadiationCoeff = 0.0f;
    cfg.dt = 0.001f;
    cfg.steps = 2000;

    ScenarioResult result;
    std::string error;
    ASSERT_TRUE(runScenario(cfg, result, error)) << error;

    ASSERT_FALSE(result.final.empty());
    for (const RenderParticle &p : result.final) {
        EXPECT_TRUE(std::isfinite(p.x));
        EXPECT_TRUE(std::isfinite(p.y));
        EXPECT_TRUE(std::isfinite(p.z));
    }

    float maxRadius = 0.0f;
    for (const RenderParticle &p : result.final) {
        const float r = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
        maxRadius = std::max(maxRadius, r);
    }
    constexpr float kMaxStableRadius = 50.0f;
    EXPECT_LE(maxRadius, kMaxStableRadius)
        << "Trajectory escaped expected stable bounds";
}

TEST_F(PhysicsRegressionTest, MultiBodyInteractions)
{
    ScenarioConfig cfg;
    cfg.particleCount = 10000u;
    cfg.dt = 0.1f;
    cfg.steps = 50;
    cfg.integrator = "euler";
    cfg.energyMeasureEverySteps = 100u;
    cfg.energySampleLimit = 512u;
    cfg.snapshotTimeoutMs = 10000;
    cfg.stepTimeoutMs = 15000;
    cfg.initState.mode = "disk_orbit";
    cfg.initState.seed = 42u;
    cfg.initState.includeCentralBody = true;
    cfg.initState.centralMass = 1.0f;
    cfg.initState.diskMass = 1000000.0f;
    cfg.initState.diskRadiusMin = 1.5f;
    cfg.initState.diskRadiusMax = 11.5f;
    cfg.initState.diskThickness = 0.0f;
    cfg.initState.velocityScale = 1.0f;
    cfg.initState.velocityTemperature = 2.0f;
    cfg.initState.particleTemperature = 0.0f;
    cfg.initState.thermalAmbientTemperature = 0.0f;
    cfg.initState.thermalSpecificHeat = 1.0f;
    cfg.initState.thermalHeatingCoeff = 0.0002f;
    cfg.initState.thermalRadiationCoeff = 0.00000001f;

    ScenarioResult result;
    std::string error;
    ASSERT_TRUE(runScenario(cfg, result, error)) << error;
    ASSERT_EQ(result.initial.size(), result.final.size());
    ASSERT_GE(result.initial.size(), 10000u);

    std::size_t movedParticles = 0u;
    float totalDisplacement = 0.0f;
    float maxDisplacement = 0.0f;
    for (std::size_t i = 0; i < result.final.size(); ++i) {
        EXPECT_TRUE(std::isfinite(result.final[i].x));
        EXPECT_TRUE(std::isfinite(result.final[i].y));
        EXPECT_TRUE(std::isfinite(result.final[i].z));
        const float dx = result.final[i].x - result.initial[i].x;
        const float dy = result.final[i].y - result.initial[i].y;
        const float dz = result.final[i].z - result.initial[i].z;
        const float disp = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (disp > 1e-4f) {
            ++movedParticles;
        }
        totalDisplacement += disp;
        maxDisplacement = std::max(maxDisplacement, disp);
    }

    const float avgDisplacement = totalDisplacement / static_cast<float>(result.final.size());
    EXPECT_GE(movedParticles, result.final.size() * 7u / 10u);
    EXPECT_GT(avgDisplacement, 4e-4f);
    EXPECT_GT(maxDisplacement, 2e-3f);

    float maxRadius = 0.0f;
    for (const RenderParticle &p : result.final) {
        const float r = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
        maxRadius = std::max(maxRadius, r);
    }
    constexpr float kMaxStableRadius = 100.0f;
    EXPECT_LE(maxRadius, kMaxStableRadius);
}

TEST_F(PhysicsRegressionTest, EnergyConservationHighMassNoSph)
{
    ScenarioConfig cfg;
    cfg.particleCount = 1000u;
    cfg.dt = 0.1f;
    cfg.steps = 120;
    cfg.integrator = "euler";
    cfg.energyMeasureEverySteps = 1u;
    cfg.energySampleLimit = 1000u;
    cfg.snapshotTimeoutMs = 10000;
    cfg.stepTimeoutMs = 20000;
    cfg.initState.mode = "disk_orbit";
    cfg.initState.seed = 42u;
    cfg.initState.includeCentralBody = true;
    cfg.initState.centralMass = 1.0f;
    cfg.initState.diskMass = 1000000.0f;
    cfg.initState.diskRadiusMin = 1.5f;
    cfg.initState.diskRadiusMax = 11.5f;
    cfg.initState.diskThickness = 0.0f;
    cfg.initState.velocityScale = 1.0f;
    cfg.initState.velocityTemperature = 2.0f;
    cfg.initState.particleTemperature = 0.0f;
    cfg.initState.thermalAmbientTemperature = 0.0f;
    cfg.initState.thermalSpecificHeat = 1.0f;
    cfg.initState.thermalHeatingCoeff = 0.0f;
    cfg.initState.thermalRadiationCoeff = 0.0f;

    ScenarioResult result;
    std::string error;
    ASSERT_TRUE(runScenario(cfg, result, error)) << error;
    EXPECT_FALSE(result.stats.energyEstimated);

    constexpr float kMaxEnergyDriftPct = 8.0f;
    EXPECT_LE(result.maxAbsEnergyDriftPct, kMaxEnergyDriftPct)
        << "High-mass no-SPH drift too high: " << result.maxAbsEnergyDriftPct << "%";
}

TEST_F(PhysicsRegressionTest, RadiationExchangeConservation)
{
    ScenarioConfig cfg;
    cfg.particleCount = 128u;
    cfg.dt = 0.1f;
    cfg.steps = 80;
    cfg.integrator = "euler";
    cfg.energyMeasureEverySteps = 1u;
    cfg.energySampleLimit = 128u;
    cfg.snapshotTimeoutMs = 10000;
    cfg.stepTimeoutMs = 10000;
    cfg.initState.mode = "random_cloud";
    cfg.initState.seed = 7u;
    cfg.initState.includeCentralBody = false;
    cfg.initState.cloudHalfExtent = 50.0f;
    cfg.initState.cloudSpeed = 0.0f;
    cfg.initState.particleMass = 1e-6f;
    cfg.initState.velocityTemperature = 0.0f;
    cfg.initState.particleTemperature = 1000.0f;
    cfg.initState.thermalAmbientTemperature = 0.0f;
    cfg.initState.thermalSpecificHeat = 1.0f;
    cfg.initState.thermalHeatingCoeff = 0.0f;
    cfg.initState.thermalRadiationCoeff = 1.0f;

    ScenarioResult result;
    std::string error;
    ASSERT_TRUE(runScenario(cfg, result, error)) << error;
    EXPECT_FALSE(result.stats.energyEstimated);

    float initialThermal = 0.0f;
    for (const RenderParticle &p : result.initial) {
        initialThermal += p.mass * std::max(0.0f, p.temperature);
    }

    constexpr float kMaxEnergyDriftPct = 1.5f;
    EXPECT_LE(result.maxAbsEnergyDriftPct, kMaxEnergyDriftPct)
        << "Radiative exchange should conserve total (thermal + radiated) energy";
    EXPECT_GT(result.stats.radiatedEnergy, 0.0f);
    EXPECT_LT(result.stats.thermalEnergy, initialThermal);
}

} // namespace
