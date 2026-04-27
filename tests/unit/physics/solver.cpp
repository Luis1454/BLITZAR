// File: tests/unit/physics/solver.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "tests/support/physics_scenario.hpp"
#include "tests/support/physics_test_utils.hpp"
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <vector>
namespace testsupport {
static std::size_t countOccurrences(const std::string& text, const std::string& pattern)
{
    if (pattern.empty()) {
        return 0u;
    }
    std::size_t count = 0u;
    std::size_t offset = 0u;
    while ((offset = text.find(pattern, offset)) != std::string::npos) {
        ++count;
        offset += pattern.size();
    }
    return count;
}
static float maxParticleDelta(const std::vector<RenderParticle>& baseline,
                              const std::vector<RenderParticle>& candidate)
{
    if (baseline.size() != candidate.size()) {
        return std::numeric_limits<float>::infinity();
    }
    float maxDelta = 0.0f;
    for (std::size_t index = 0; index < baseline.size(); index += 1u) {
        maxDelta = std::max(maxDelta, testsupport::distance(baseline[index], candidate[index]));
    }
    return maxDelta;
}
static std::filesystem::path writeStabilityConfig(float minSoftening, float minDistance2)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("gravity_physics_stability_" + std::to_string(stamp) + ".ini");
    std::ofstream out(path, std::ios::trunc);
    out << "particle_count=2\n";
    out << "dt=0.001\n";
    out << "simulation_profile=manual_override\n";
    out << "performance_profile=custom\n";
    out << "solver=octree_cpu\n";
    out << "integrator=euler\n";
    out << "init_config_style=detailed\n";
    out << "input_file=" << getTwoBodyInputPath() << "\n";
    out << "input_format=xyz\n";
    out << "init_mode=file\n";
    out << "octree_theta=0.35\n";
    out << "octree_softening=0.01\n";
    out << "physics_max_acceleration=64\n";
    out << "physics_min_softening=" << minSoftening << "\n";
    out << "physics_min_distance2=" << minDistance2 << "\n";
    out << "physics_min_theta=0.05\n";
    out << "energy_measure_every_steps=1\n";
    out << "energy_sample_limit=64\n";
    return path;
}
static float runTotalEnergyFromConfig(const std::filesystem::path& path)
{
    SimulationServer server(path.string());
    server.setPaused(true);
    server.start();
    std::vector<RenderParticle> snapshot;
    EXPECT_TRUE(waitForConsumedSnapshot(server, snapshot, 4000));
    EXPECT_EQ(snapshot.size(), 2u);
    server.stepOnce();
    EXPECT_TRUE(waitForStepCount(server, 1u, 4000));
    EXPECT_TRUE(waitForConsumedSnapshot(server, snapshot, 4000));
    const float totalEnergy = server.getStats().totalEnergy;
    server.stop();
    return totalEnergy;
}
TEST(PhysicsTest, TST_UNT_PHYS_009_SolverParityWithinTolerance)
{
    ScenarioConfig base =
        buildDiskOrbitScenario(96u, 0.004f, 36u, 12345u, "pairwise_cuda", "euler");
    setScenarioEnergySampling(base, 1u, 96u);
    setScenarioTiming(base, 8000, 8000);
    base.octreeTheta = 0.35f;
    base.octreeSoftening = 0.08f;
    base.initState.diskMass = 0.75f;
    base.initState.velocityTemperature = 0.1f;
    ScenarioConfig pairwiseCfg = base;
    ScenarioResult pairwise;
    std::string pairwiseError;
    ASSERT_TRUE(runScenario(pairwiseCfg, pairwise, pairwiseError)) << pairwiseError;
    if (pairwise.stats.solverName != pairwiseCfg.solver) {
        GTEST_SKIP() << "pairwise_cuda unavailable in this environment (actual solver="
                     << pairwise.stats.solverName << ")";
    }
    ScenarioConfig octreeCpuCfg = base;
    octreeCpuCfg.solver = "octree_cpu";
    ScenarioResult octreeCpu;
    std::string octreeCpuError;
    ASSERT_TRUE(runScenario(octreeCpuCfg, octreeCpu, octreeCpuError)) << octreeCpuError;
    ScenarioConfig octreeGpuCfg = base;
    octreeGpuCfg.solver = "octree_gpu";
    ScenarioResult octreeGpu;
    std::string octreeGpuError;
    ASSERT_TRUE(runScenario(octreeGpuCfg, octreeGpu, octreeGpuError)) << octreeGpuError;
    if (octreeGpu.stats.solverName != octreeGpuCfg.solver) {
        GTEST_SKIP() << "octree_gpu unavailable in this environment (actual solver="
                     << octreeGpu.stats.solverName << ")";
    }
    const auto pairwiseCom = centerOfMassAll(pairwise.final);
    const auto octreeCpuCom = centerOfMassAll(octreeCpu.final);
    const auto octreeGpuCom = centerOfMassAll(octreeGpu.final);
    const float comCpuDelta =
        std::sqrt((pairwiseCom[0] - octreeCpuCom[0]) * (pairwiseCom[0] - octreeCpuCom[0]) +
                  (pairwiseCom[1] - octreeCpuCom[1]) * (pairwiseCom[1] - octreeCpuCom[1]) +
                  (pairwiseCom[2] - octreeCpuCom[2]) * (pairwiseCom[2] - octreeCpuCom[2]));
    const float comGpuDelta =
        std::sqrt((pairwiseCom[0] - octreeGpuCom[0]) * (pairwiseCom[0] - octreeGpuCom[0]) +
                  (pairwiseCom[1] - octreeGpuCom[1]) * (pairwiseCom[1] - octreeGpuCom[1]) +
                  (pairwiseCom[2] - octreeGpuCom[2]) * (pairwiseCom[2] - octreeGpuCom[2]));
    const float pairwiseAvgRadius = averageRadius(pairwise.final);
    const float octreeCpuAvgRadius = averageRadius(octreeCpu.final);
    const float octreeGpuAvgRadius = averageRadius(octreeGpu.final);
    const float pairwiseEnergy = pairwise.stats.totalEnergy;
    const float cpuEnergyDiffPct = std::fabs(octreeCpu.stats.totalEnergy - pairwiseEnergy) /
                                   std::max(std::fabs(pairwiseEnergy), 1e-6f) * 100.0f;
    const float gpuEnergyDiffPct = std::fabs(octreeGpu.stats.totalEnergy - pairwiseEnergy) /
                                   std::max(std::fabs(pairwiseEnergy), 1e-6f) * 100.0f;
    const float cpuParticleDelta = maxParticleDelta(pairwise.final, octreeCpu.final);
    const float gpuParticleDelta = maxParticleDelta(pairwise.final, octreeGpu.final);
    EXPECT_LE(comCpuDelta, 0.02f);
    EXPECT_LE(comGpuDelta, 0.02f);
    EXPECT_LE(std::fabs(octreeCpuAvgRadius - pairwiseAvgRadius), 0.05f);
    EXPECT_LE(std::fabs(octreeGpuAvgRadius - pairwiseAvgRadius), 0.05f);
    EXPECT_LE(cpuEnergyDiffPct, 0.25f);
    EXPECT_LE(gpuEnergyDiffPct, 0.25f);
    EXPECT_LE(cpuParticleDelta, 0.05f);
    EXPECT_LE(gpuParticleDelta, 0.05f);
}
TEST(PhysicsTest, TST_UNT_RUNT_001_ServerLogsEffectiveModesAfterReset)
{
    SimulationServer server(48u, 0.01f);
    server.setSolverMode("octree_cpu");
    server.setIntegratorMode("rk4");
    server.setPaused(true);
    testing::internal::CaptureStdout();
    server.start();
    std::vector<RenderParticle> snapshot;
    const bool startupReady = waitForConsumedSnapshot(server, snapshot, 4000);
    server.requestReset();
    const bool resetReady = waitForConsumedSnapshot(server, snapshot, 4000);
    server.stop();
    const std::string output = testing::internal::GetCapturedStdout();
    const std::string expected = "[server] active solver=octree_cpu integrator=rk4";
    ASSERT_TRUE(startupReady);
    ASSERT_TRUE(resetReady);
    EXPECT_EQ(countOccurrences(output, expected), 2u);
    EXPECT_NE(output.find("physics_max_acceleration=64"), std::string::npos);
    EXPECT_NE(output.find("physics_min_softening=0.0001"), std::string::npos);
    EXPECT_NE(output.find("physics_min_distance2=1e-12"), std::string::npos);
    EXPECT_NE(output.find("physics_min_theta=0.05"), std::string::npos);
    EXPECT_EQ(output.find("[solver] using"), std::string::npos);
    EXPECT_EQ(output.find("[integrator] mode="), std::string::npos);
}
TEST(PhysicsTest, TST_UNT_RUNT_002_ParticleSystemCtorKeepsExplicitInitialState)
{
    std::vector<Particle> initialParticles(2);
    initialParticles[0].setPosition(Vector3(-2.0f, 0.5f, 0.0f));
    initialParticles[0].setVelocity(Vector3(0.0f, 0.25f, 0.0f));
    initialParticles[0].setMass(3.0f);
    initialParticles[0].setTemperature(7.0f);
    initialParticles[1].setPosition(Vector3(4.0f, -1.0f, 0.0f));
    initialParticles[1].setVelocity(Vector3(-0.1f, 0.0f, 0.0f));
    initialParticles[1].setMass(0.5f);
    initialParticles[1].setTemperature(2.0f);
    ParticleSystem system(std::move(initialParticles));
    const std::vector<Particle>& configuredParticles = system.getParticles();
    ASSERT_EQ(configuredParticles.size(), 2u);
    EXPECT_FLOAT_EQ(configuredParticles[0].getPosition().x, -2.0f);
    EXPECT_FLOAT_EQ(configuredParticles[0].getPosition().y, 0.5f);
    EXPECT_FLOAT_EQ(configuredParticles[0].getVelocity().y, 0.25f);
    EXPECT_FLOAT_EQ(configuredParticles[0].getMass(), 3.0f);
    EXPECT_FLOAT_EQ(configuredParticles[0].getTemperature(), 7.0f);
    EXPECT_FLOAT_EQ(configuredParticles[1].getPosition().x, 4.0f);
    EXPECT_FLOAT_EQ(configuredParticles[1].getPosition().y, -1.0f);
    EXPECT_FLOAT_EQ(configuredParticles[1].getVelocity().x, -0.1f);
    EXPECT_FLOAT_EQ(configuredParticles[1].getMass(), 0.5f);
    EXPECT_FLOAT_EQ(configuredParticles[1].getTemperature(), 2.0f);
}
TEST(PhysicsTest, TST_UNT_PHYS_010_DeterministicReplayIdentical)
{
    ScenarioConfig cfg = buildDiskOrbitScenario(64u, 0.005f, 20u, 77777u, "pairwise_cuda", "euler");
    setScenarioEnergySampling(cfg, 1u, 64u);
    setScenarioTiming(cfg, 6000, 6000);
    cfg.initState.diskMass = 0.5f;
    cfg.initState.diskRadiusMax = 8.0f;
    ScenarioResult runA;
    std::string errorA;
    ASSERT_TRUE(runScenario(cfg, runA, errorA)) << errorA;
    if (runA.stats.solverName != cfg.solver) {
        GTEST_SKIP() << "solver unavailable (actual=" << runA.stats.solverName << ")";
    }
    ScenarioResult runB;
    std::string errorB;
    ASSERT_TRUE(runScenario(cfg, runB, errorB)) << errorB;
    std::string replayError;
    EXPECT_TRUE(haveExactReplayMatch(runA, runB, replayError)) << replayError;
}
TEST(PhysicsTest, TST_UNT_PHYS_018_EnergyEstimateUsesConfiguredStabilityConstants)
{
    const std::filesystem::path permissivePath = writeStabilityConfig(0.01f, 0.01f);
    const std::filesystem::path clampedPath = writeStabilityConfig(0.5f, 5.0f);
    const float permissiveEnergy = runTotalEnergyFromConfig(permissivePath);
    const float clampedEnergy = runTotalEnergyFromConfig(clampedPath);
    std::error_code ec;
    std::filesystem::remove(permissivePath, ec);
    std::filesystem::remove(clampedPath, ec);
    EXPECT_LT(permissiveEnergy, -0.1f);
    EXPECT_GT(clampedEnergy, permissiveEnergy + 0.1f);
}
} // namespace testsupport
