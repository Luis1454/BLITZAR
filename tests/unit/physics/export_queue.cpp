/*
 * @file tests/unit/physics/export_queue.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#include "server/SimulationServer.hpp"
#include "tests/support/physics_test_utils.hpp"
#include <chrono>
#include <filesystem>
#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <vector>

namespace bltzr_test_server_export_queue {
static std::filesystem::path makeTempExportPath(const char* stem)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
           (std::string(stem) + "_" + std::to_string(stamp) + ".vtk");
}

static bool waitForExportCompletion(SimulationServer& server, const std::filesystem::path& path,
                                    std::uint64_t expectedCompletedCount, std::uint32_t timeoutMs,
                                    bool& outObservedBacklog)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() <= deadline) {
        const SimulationStats stats = server.getStats();
        if (stats.exportQueueDepth > 0u || stats.exportActive)
            outObservedBacklog = true;
        if (std::filesystem::exists(path) && stats.exportCompletedCount >= expectedCompletedCount &&
            !stats.exportActive && stats.exportQueueDepth == 0u) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return false;
}

TEST(PhysicsTest, TST_UNT_RUNT_007_ServerPublishesAsyncExportQueueStatus)
{
    SimulationServer server(32u, 0.01f);
    server.setSolverMode("octree_cpu");
    server.setPaused(true);
    server.start();
    std::vector<RenderParticle> snapshot;
    ASSERT_TRUE(testsupport::waitForPublishedSnapshot(server, snapshot, 4000));
    const std::filesystem::path exportPath = makeTempExportPath("bltzr_export_async");
    server.requestExportSnapshot(exportPath.string(), "vtk");
    bool observedBacklog = false;
    ASSERT_TRUE(waitForExportCompletion(server, exportPath, 1u, 4000u, observedBacklog));
    const SimulationStats stats = server.getStats();
    EXPECT_TRUE(observedBacklog);
    EXPECT_EQ(stats.exportQueueDepth, 0u);
    EXPECT_FALSE(stats.exportActive);
    EXPECT_EQ(stats.exportCompletedCount, 1u);
    EXPECT_EQ(stats.exportFailedCount, 0u);
    EXPECT_EQ(stats.exportLastState, "completed");
    EXPECT_EQ(stats.exportLastPath, exportPath.string());
    server.stop();
    std::error_code ec;
    std::filesystem::remove(exportPath, ec);
}

TEST(PhysicsTest, TST_UNT_RUNT_008_ServerStopFlushesQueuedExports)
{
    SimulationServer server(32u, 0.01f);
    server.setSolverMode("octree_cpu");
    server.setPaused(true);
    server.start();
    std::vector<RenderParticle> snapshot;
    ASSERT_TRUE(testsupport::waitForPublishedSnapshot(server, snapshot, 4000));
    const std::filesystem::path exportPath = makeTempExportPath("bltzr_export_flush");
    server.requestExportSnapshot(exportPath.string(), "vtk");
    server.stop();
    ASSERT_TRUE(std::filesystem::exists(exportPath));
    EXPECT_GT(std::filesystem::file_size(exportPath), 0u);
    const SimulationStats stats = server.getStats();
    EXPECT_EQ(stats.exportQueueDepth, 0u);
    EXPECT_FALSE(stats.exportActive);
    EXPECT_EQ(stats.exportCompletedCount, 1u);
    EXPECT_EQ(stats.exportFailedCount, 0u);
    EXPECT_EQ(stats.exportLastState, "completed");
    std::error_code ec;
    std::filesystem::remove(exportPath, ec);
}
} // namespace bltzr_test_server_export_queue
