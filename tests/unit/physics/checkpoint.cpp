/*
 * @file tests/unit/physics/checkpoint.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#include "server/SimulationServer.hpp"
#include "tests/support/physics_test_utils.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

namespace grav_test_server_checkpoint {
static std::filesystem::path makeTempCheckpointPath(const char* stem)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
           (std::string(stem) + "_" + std::to_string(stamp) + ".chk");
}

static bool waitForTotalTime(const SimulationServer& server, float minimumTotalTime, int timeoutMs)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        if (server.getStats().totalTime >= minimumTotalTime) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return server.getStats().totalTime >= minimumTotalTime;
}

static std::vector<char> readFileBytes(const std::filesystem::path& path)
{
    std::ifstream in(path, std::ios::binary);
    return std::vector<char>((std::istreambuf_iterator<char>(in)),
                             std::istreambuf_iterator<char>());
}

TEST(PhysicsTest, TST_UNT_RUNT_009_CheckpointRoundTripRestoresRuntimeState)
{
    const std::filesystem::path checkpointPath =
        makeTempCheckpointPath("grav_checkpoint_roundtrip");
    const std::filesystem::path roundTripPath =
        makeTempCheckpointPath("grav_checkpoint_roundtrip_reload");
    SimulationServer source(24u, 0.02f);
    source.setSolverMode("octree_cpu");
    source.setIntegratorMode("euler");
    source.setPerformanceProfile("balanced");
    source.setOctreeParameters(0.72f, 0.45f);
    source.setSubstepPolicy(0.005f, 3u);
    source.setEnergyMeasurementConfig(1u, 64u);
    source.setPaused(false);
    source.start();
    ASSERT_TRUE(waitForTotalTime(source, 0.06f, 4000));
    source.setPaused(true);
    ASSERT_TRUE(source.saveCheckpoint(checkpointPath.string()));
    const std::vector<char> expectedCheckpointBytes = readFileBytes(checkpointPath);
    ASSERT_FALSE(expectedCheckpointBytes.empty());
    const SimulationStats expectedStats = source.getStats();
    source.stop();
    SimulationServer restored(2u, 0.01f);
    restored.setPaused(true);
    restored.start();
    std::string error;
    ASSERT_TRUE(restored.loadCheckpoint(checkpointPath.string(), &error)) << error;
    std::vector<RenderParticle> restoredSnapshot;
    ASSERT_TRUE(testsupport::waitForPublishedSnapshot(restored, restoredSnapshot, 4000));
    const SimulationStats restoredStats = restored.getStats();
    ASSERT_TRUE(restored.saveCheckpoint(roundTripPath.string()));
    const std::vector<char> restoredCheckpointBytes = readFileBytes(roundTripPath);
    EXPECT_EQ(restoredStats.steps, expectedStats.steps);
    EXPECT_FLOAT_EQ(restoredStats.totalTime, expectedStats.totalTime);
    EXPECT_FLOAT_EQ(restoredStats.dt, expectedStats.dt);
    EXPECT_EQ(restoredStats.solverName, expectedStats.solverName);
    EXPECT_EQ(restoredStats.integratorName, expectedStats.integratorName);
    EXPECT_EQ(restoredStats.performanceProfile, expectedStats.performanceProfile);
    EXPECT_EQ(restoredSnapshot.size(), expectedStats.particleCount);
    EXPECT_EQ(restoredCheckpointBytes, expectedCheckpointBytes);
    restored.stop();
    std::error_code ec;
    std::filesystem::remove(checkpointPath, ec);
    std::filesystem::remove(roundTripPath, ec);
}

TEST(PhysicsTest, TST_UNT_RUNT_010_CheckpointRejectsInvalidMagic)
{
    const std::filesystem::path checkpointPath = makeTempCheckpointPath("grav_checkpoint_invalid");
    {
        std::ofstream out(checkpointPath, std::ios::binary | std::ios::trunc);
        out << "not_a_checkpoint";
    }
    SimulationServer server(8u, 0.01f);
    server.setPaused(true);
    server.start();
    std::string error;
    EXPECT_FALSE(server.loadCheckpoint(checkpointPath.string(), &error));
    EXPECT_NE(error.find("magic"), std::string::npos);
    server.stop();
    std::error_code ec;
    std::filesystem::remove(checkpointPath, ec);
}

TEST(PhysicsTest, TST_UNT_RUNT_011_CheckpointRejectsUnsupportedVersion)
{
    const std::filesystem::path checkpointPath = makeTempCheckpointPath("grav_checkpoint_version");
    {
        std::ofstream out(checkpointPath, std::ios::binary | std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out.write("BLTZCHK1", 8);
        const std::uint32_t version = 999u;
        out.write(reinterpret_cast<const char*>(&version), sizeof(version));
        const std::uint32_t flags = 0u;
        out.write(reinterpret_cast<const char*>(&flags), sizeof(flags));
        const std::uint64_t steps = 0u;
        out.write(reinterpret_cast<const char*>(&steps), sizeof(steps));
        const std::uint32_t count = 2u;
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
    }
    SimulationServer server(8u, 0.01f);
    server.setPaused(true);
    server.start();
    std::string error;
    EXPECT_FALSE(server.loadCheckpoint(checkpointPath.string(), &error));
    EXPECT_NE(error.find("version"), std::string::npos);
    server.stop();
    std::error_code ec;
    std::filesystem::remove(checkpointPath, ec);
}
} // namespace grav_test_server_checkpoint
