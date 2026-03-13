#include "server/SimulationServer.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>

namespace grav_test_substep_policy {

static std::filesystem::path writeTempConfig()
{
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "grav_substep_policy.ini";
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

static bool waitForSteps(SimulationServer &server, std::uint64_t expectedSteps, int timeoutMs)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        if (server.getStats().steps >= expectedSteps) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return server.getStats().steps >= expectedSteps;
}

TEST(PhysicsTest, TST_UNT_RUNT_004_ServerAppliesConfiguredSubstepPolicy)
{
    const std::filesystem::path configPath = writeTempConfig();
    SimulationServer server(configPath.string());
    server.start();

    ASSERT_TRUE(waitForSteps(server, 1u, 4000));

    const SimulationStats stats = server.getStats();
    EXPECT_FLOAT_EQ(stats.substepTargetDt, 0.005f);
    EXPECT_FLOAT_EQ(stats.substepDt, 0.005f);
    EXPECT_EQ(stats.substeps, 2u);
    EXPECT_EQ(stats.maxSubsteps, 8u);

    server.stop();
    std::error_code ec;
    std::filesystem::remove(configPath, ec);
}

} // namespace grav_test_substep_policy
