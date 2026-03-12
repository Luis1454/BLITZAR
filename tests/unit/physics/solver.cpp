#include "tests/support/physics_scenario.hpp"

#include <chrono>
#include <cmath>
#include <string>
#include <thread>
#include <vector>

namespace testsupport {

static bool waitForSnapshot(SimulationBackend &backend, std::vector<RenderParticle> &outSnapshot, int timeoutMs)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        if (backend.tryConsumeSnapshot(outSnapshot)) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return backend.tryConsumeSnapshot(outSnapshot);
}

static std::size_t countOccurrences(const std::string &text, const std::string &pattern)
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

TEST_F(PhysicsTest, TST_UNT_PHYS_009_SolverParityWithinTolerance)
{
    ScenarioConfig base;
    base.particleCount = 96u;
    base.dt = 0.004f;
    base.steps = 36;
    base.integrator = "euler";
    base.energyMeasureEverySteps = 1u;
    base.energySampleLimit = 96u;
    base.snapshotTimeoutMs = 8000;
    base.stepTimeoutMs = 8000;
    base.octreeTheta = 0.35f;
    base.octreeSoftening = 0.08f;
    base.initState.mode = "disk_orbit";
    base.initState.seed = 12345u;
    base.initState.includeCentralBody = true;
    base.initState.centralMass = 1.0f;
    base.initState.diskMass = 0.75f;
    base.initState.diskRadiusMin = 1.5f;
    base.initState.diskRadiusMax = 11.5f;
    base.initState.diskThickness = 0.0f;
    base.initState.velocityScale = 1.0f;
    base.initState.velocityTemperature = 0.1f;
    base.initState.particleTemperature = 0.0f;
    base.initState.thermalAmbientTemperature = 0.0f;
    base.initState.thermalSpecificHeat = 1.0f;
    base.initState.thermalHeatingCoeff = 0.0f;
    base.initState.thermalRadiationCoeff = 0.0f;

    ScenarioConfig pairwiseCfg = base;
    pairwiseCfg.solver = "pairwise_cuda";
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

    const float comCpuDelta = std::sqrt(
        (pairwiseCom[0] - octreeCpuCom[0]) * (pairwiseCom[0] - octreeCpuCom[0])
        + (pairwiseCom[1] - octreeCpuCom[1]) * (pairwiseCom[1] - octreeCpuCom[1])
        + (pairwiseCom[2] - octreeCpuCom[2]) * (pairwiseCom[2] - octreeCpuCom[2]));
    const float comGpuDelta = std::sqrt(
        (pairwiseCom[0] - octreeGpuCom[0]) * (pairwiseCom[0] - octreeGpuCom[0])
        + (pairwiseCom[1] - octreeGpuCom[1]) * (pairwiseCom[1] - octreeGpuCom[1])
        + (pairwiseCom[2] - octreeGpuCom[2]) * (pairwiseCom[2] - octreeGpuCom[2]));

    const float pairwiseAvgRadius = averageRadius(pairwise.final);
    const float octreeCpuAvgRadius = averageRadius(octreeCpu.final);
    const float octreeGpuAvgRadius = averageRadius(octreeGpu.final);

    const float pairwiseEnergy = pairwise.stats.totalEnergy;
    const float cpuEnergyDiffPct = std::fabs(octreeCpu.stats.totalEnergy - pairwiseEnergy)
        / std::max(std::fabs(pairwiseEnergy), 1e-6f) * 100.0f;
    const float gpuEnergyDiffPct = std::fabs(octreeGpu.stats.totalEnergy - pairwiseEnergy)
        / std::max(std::fabs(pairwiseEnergy), 1e-6f) * 100.0f;

    EXPECT_LE(comCpuDelta, 0.45f);
    EXPECT_LE(comGpuDelta, 0.45f);
    EXPECT_LE(std::fabs(octreeCpuAvgRadius - pairwiseAvgRadius), 0.9f);
    EXPECT_LE(std::fabs(octreeGpuAvgRadius - pairwiseAvgRadius), 0.9f);
    EXPECT_LE(cpuEnergyDiffPct, 8.0f);
    EXPECT_LE(gpuEnergyDiffPct, 8.0f);
}

TEST_F(PhysicsTest, TST_UNT_RUNT_001_BackendLogsEffectiveModesAfterReset)
{
    SimulationBackend backend(48u, 0.01f);
    backend.setSolverMode("octree_cpu");
    backend.setIntegratorMode("rk4");
    backend.setPaused(true);

    testing::internal::CaptureStdout();
    backend.start();

    std::vector<RenderParticle> snapshot;
    const bool startupReady = waitForSnapshot(backend, snapshot, 4000);
    backend.requestReset();
    const bool resetReady = waitForSnapshot(backend, snapshot, 4000);
    backend.stop();

    const std::string output = testing::internal::GetCapturedStdout();
    const std::string expected = "[backend] active solver=octree_cpu integrator=rk4";

    ASSERT_TRUE(startupReady);
    ASSERT_TRUE(resetReady);
    EXPECT_EQ(countOccurrences(output, expected), 2u);
    EXPECT_EQ(output.find("[solver] using"), std::string::npos);
    EXPECT_EQ(output.find("[integrator] mode="), std::string::npos);
}

} // namespace testsupport

