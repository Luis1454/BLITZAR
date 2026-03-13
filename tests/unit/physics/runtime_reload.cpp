#include "server/SimulationServer.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

namespace grav_test_server_reload {

static bool waitForSnapshot(SimulationServer &server, std::vector<RenderParticle> &outSnapshot, int timeoutMs)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        if (server.copyLatestSnapshot(outSnapshot)) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return server.copyLatestSnapshot(outSnapshot);
}

static std::filesystem::path writeTempXyz(const char *basename, const std::vector<float> &xs)
{
    const std::filesystem::path path = std::filesystem::temp_directory_path() / basename;
    std::ofstream out(path, std::ios::trunc);
    out << xs.size() << "\n";
    out << basename << "\n";
    for (std::size_t i = 0; i < xs.size(); ++i) {
        out << "P " << xs[i] << " 0 0 1 " << static_cast<float>(i) << "\n";
    }
    return path;
}

TEST(PhysicsTest, TST_UNT_RUNT_003_ServerReloadClearsPublishedSnapshotCache)
{
    SimulationServer server(24u, 0.01f);
    server.setSolverMode("octree_cpu");
    server.setPaused(true);
    server.start();

    std::vector<RenderParticle> snapshot;
    ASSERT_TRUE(waitForSnapshot(server, snapshot, 4000));
    ASSERT_EQ(snapshot.size(), 24u);

    const std::filesystem::path xyz3 = writeTempXyz("grav_test_reload_3.xyz", {-2.0f, 0.0f, 2.0f});
    const std::filesystem::path xyz4 = writeTempXyz("grav_test_reload_4.xyz", {-3.0f, -1.0f, 1.0f, 3.0f});

    server.setInitialStateFile(xyz3.string(), "xyz");
    EXPECT_FALSE(server.copyLatestSnapshot(snapshot));
    ASSERT_TRUE(waitForSnapshot(server, snapshot, 4000));
    EXPECT_EQ(snapshot.size(), 3u);
    EXPECT_EQ(server.getStats().particleCount, 3u);

    server.setInitialStateFile(xyz4.string(), "xyz");
    EXPECT_FALSE(server.copyLatestSnapshot(snapshot));
    ASSERT_TRUE(waitForSnapshot(server, snapshot, 4000));
    EXPECT_EQ(snapshot.size(), 4u);
    EXPECT_EQ(server.getStats().particleCount, 4u);

    server.stop();
    std::error_code ec;
    std::filesystem::remove(xyz3, ec);
    ec.clear();
    std::filesystem::remove(xyz4, ec);
}

} // namespace grav_test_server_reload
