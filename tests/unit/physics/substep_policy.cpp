// File: tests/unit/physics/substep_policy.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "server/SimulationServer.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <thread>
#include <vector>
namespace grav_test_substep_policy {
/// Description: Executes the writeTempConfig operation.
static std::filesystem::path writeTempConfig()
{
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "grav_substep_policy.ini";
    /// Description: Executes the out operation.
    std::ofstream out(path, std::ios::trunc);
    out << "particle_count=32\n";
    out << "dt=0.01\n";
    out << "solver=octree_cpu\n";
    out << "integrator=euler\n";
    out << "substep_target_dt=0.005\n";
    out << "max_substeps=8\n";
    out << "init_config_style=preset\n";
    out << "preset_structure=random_cloud\n";
    out << "sph_enabled=false\n";
    return path;
}
/// Description: Executes the waitForSteps operation.
static bool waitForSteps(SimulationServer& server, std::uint64_t expectedSteps, int timeoutMs)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        if (server.getStats().steps >= expectedSteps) {
            return true;
        }
        /// Description: Executes the sleep_for operation.
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return server.getStats().steps >= expectedSteps;
}
/// Description: Executes the TEST operation.
TEST(PhysicsTest, TST_UNT_RUNT_004_ServerAppliesConfiguredSubstepPolicy)
{
    const std::filesystem::path configPath = writeTempConfig();
    /// Description: Executes the server operation.
    SimulationServer server(configPath.string());
    server.start();
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(waitForSteps(server, 1u, 4000));
    const SimulationStats stats = server.getStats();
    /// Description: Executes the EXPECT_FLOAT_EQ operation.
    EXPECT_FLOAT_EQ(stats.substepTargetDt, 0.005f);
    /// Description: Executes the EXPECT_FLOAT_EQ operation.
    EXPECT_FLOAT_EQ(stats.substepDt, 0.005f);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(stats.substeps, 2u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(stats.maxSubsteps, 8u);
    server.stop();
    std::error_code ec;
    /// Description: Executes the remove operation.
    std::filesystem::remove(configPath, ec);
}
/// Description: Executes the TEST operation.
TEST(PhysicsTest, TST_UNT_RUNT_005_ServerAppliesInteractivePerformancePreset)
{
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "grav_interactive_perf.ini";
    {
        /// Description: Executes the out operation.
        std::ofstream out(path, std::ios::trunc);
        /// Description: Executes the ASSERT_TRUE operation.
        ASSERT_TRUE(out.is_open());
        out << "simulation(particle_count=32, dt=0.01, solver=octree_cpu, integrator=euler)\n";
        out << "performance(profile=interactive)\n";
        out << "scene(style=preset, preset=random_cloud, mode=random_cloud, file=\"\", "
               "format=auto)\n";
        out << "sph(enabled=false, smoothing_length=1.25, rest_density=1, gas_constant=4, "
               "viscosity=0.08)\n";
    }
    /// Description: Executes the server operation.
    SimulationServer server(path.string());
    server.start();
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(waitForSteps(server, 1u, 4000));
    const SimulationStats stats = server.getStats();
    const SimulationConfig config = server.getRuntimeConfig();
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(stats.performanceProfile, "interactive");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(stats.snapshotPublishPeriodMs, 50u);
    /// Description: Executes the EXPECT_FLOAT_EQ operation.
    EXPECT_FLOAT_EQ(stats.substepTargetDt, 0.01f);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(stats.maxSubsteps, 4u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.performanceProfile, "interactive");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.snapshotPublishPeriodMs, 50u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.energyMeasureEverySteps, 30u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.energySampleLimit, 256u);
    server.stop();
    std::error_code ec;
    /// Description: Executes the remove operation.
    std::filesystem::remove(path, ec);
}
} // namespace grav_test_substep_policy
