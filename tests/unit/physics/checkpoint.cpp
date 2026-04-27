// File: tests/unit/physics/checkpoint.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

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
/// Description: Executes the makeTempCheckpointPath operation.
static std::filesystem::path makeTempCheckpointPath(const char* stem)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
           (std::string(stem) + "_" + std::to_string(stamp) + ".chk");
}
/// Description: Executes the waitForTotalTime operation.
static bool waitForTotalTime(const SimulationServer& server, float minimumTotalTime, int timeoutMs)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        if (server.getStats().totalTime >= minimumTotalTime) {
            return true;
        }
        /// Description: Executes the sleep_for operation.
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return server.getStats().totalTime >= minimumTotalTime;
}
/// Description: Executes the readFileBytes operation.
static std::vector<char> readFileBytes(const std::filesystem::path& path)
{
    /// Description: Executes the in operation.
    std::ifstream in(path, std::ios::binary);
    return std::vector<char>((std::istreambuf_iterator<char>(in)),
                             std::istreambuf_iterator<char>());
}
/// Description: Executes the TEST operation.
TEST(PhysicsTest, TST_UNT_RUNT_009_CheckpointRoundTripRestoresRuntimeState)
{
    const std::filesystem::path checkpointPath =
        /// Description: Executes the makeTempCheckpointPath operation.
        makeTempCheckpointPath("grav_checkpoint_roundtrip");
    const std::filesystem::path roundTripPath =
        /// Description: Executes the makeTempCheckpointPath operation.
        makeTempCheckpointPath("grav_checkpoint_roundtrip_reload");
    /// Description: Executes the source operation.
    SimulationServer source(24u, 0.02f);
    source.setSolverMode("octree_cpu");
    source.setIntegratorMode("euler");
    source.setPerformanceProfile("balanced");
    source.setOctreeParameters(0.72f, 0.45f);
    source.setSubstepPolicy(0.005f, 3u);
    source.setEnergyMeasurementConfig(1u, 64u);
    source.setPaused(false);
    source.start();
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(waitForTotalTime(source, 0.06f, 4000));
    source.setPaused(true);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(source.saveCheckpoint(checkpointPath.string()));
    const std::vector<char> expectedCheckpointBytes = readFileBytes(checkpointPath);
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(expectedCheckpointBytes.empty());
    const SimulationStats expectedStats = source.getStats();
    source.stop();
    /// Description: Executes the restored operation.
    SimulationServer restored(2u, 0.01f);
    restored.setPaused(true);
    restored.start();
    std::string error;
    ASSERT_TRUE(restored.loadCheckpoint(checkpointPath.string(), &error)) << error;
    std::vector<RenderParticle> restoredSnapshot;
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(testsupport::waitForPublishedSnapshot(restored, restoredSnapshot, 4000));
    const SimulationStats restoredStats = restored.getStats();
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(restored.saveCheckpoint(roundTripPath.string()));
    const std::vector<char> restoredCheckpointBytes = readFileBytes(roundTripPath);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(restoredStats.steps, expectedStats.steps);
    /// Description: Executes the EXPECT_FLOAT_EQ operation.
    EXPECT_FLOAT_EQ(restoredStats.totalTime, expectedStats.totalTime);
    /// Description: Executes the EXPECT_FLOAT_EQ operation.
    EXPECT_FLOAT_EQ(restoredStats.dt, expectedStats.dt);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(restoredStats.solverName, expectedStats.solverName);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(restoredStats.integratorName, expectedStats.integratorName);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(restoredStats.performanceProfile, expectedStats.performanceProfile);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(restoredSnapshot.size(), expectedStats.particleCount);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(restoredCheckpointBytes, expectedCheckpointBytes);
    restored.stop();
    std::error_code ec;
    /// Description: Executes the remove operation.
    std::filesystem::remove(checkpointPath, ec);
    /// Description: Executes the remove operation.
    std::filesystem::remove(roundTripPath, ec);
}
/// Description: Executes the TEST operation.
TEST(PhysicsTest, TST_UNT_RUNT_010_CheckpointRejectsInvalidMagic)
{
    const std::filesystem::path checkpointPath = makeTempCheckpointPath("grav_checkpoint_invalid");
    {
        /// Description: Executes the out operation.
        std::ofstream out(checkpointPath, std::ios::binary | std::ios::trunc);
        out << "not_a_checkpoint";
    }
    /// Description: Executes the server operation.
    SimulationServer server(8u, 0.01f);
    server.setPaused(true);
    server.start();
    std::string error;
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(server.loadCheckpoint(checkpointPath.string(), &error));
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(error.find("magic"), std::string::npos);
    server.stop();
    std::error_code ec;
    /// Description: Executes the remove operation.
    std::filesystem::remove(checkpointPath, ec);
}
/// Description: Executes the TEST operation.
TEST(PhysicsTest, TST_UNT_RUNT_011_CheckpointRejectsUnsupportedVersion)
{
    const std::filesystem::path checkpointPath = makeTempCheckpointPath("grav_checkpoint_version");
    {
        /// Description: Executes the out operation.
        std::ofstream out(checkpointPath, std::ios::binary | std::ios::trunc);
        /// Description: Executes the ASSERT_TRUE operation.
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
    /// Description: Executes the server operation.
    SimulationServer server(8u, 0.01f);
    server.setPaused(true);
    server.start();
    std::string error;
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(server.loadCheckpoint(checkpointPath.string(), &error));
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(error.find("version"), std::string::npos);
    server.stop();
    std::error_code ec;
    /// Description: Executes the remove operation.
    std::filesystem::remove(checkpointPath, ec);
}
} // namespace grav_test_server_checkpoint
