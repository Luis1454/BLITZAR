#include "tests/support/physics_scenario.hpp"

#include <cmath>
#include <string>

namespace testsupport {
TEST_F(PhysicsTest, TST_UNT_PHYS_009_SolverParityWithinTolerance)
{
    ScenarioConfig base;
    base.particleCount = 384u;
    base.dt = 0.004f;
    base.steps = 180;
    base.integrator = "euler";
    base.energyMeasureEverySteps = 1u;
    base.energySampleLimit = 384u;
    base.snapshotTimeoutMs = 10000;
    base.stepTimeoutMs = 10000;
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

} // namespace testsupport

