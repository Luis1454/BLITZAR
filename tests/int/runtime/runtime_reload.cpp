// File: tests/int/runtime/runtime_reload.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "client/ClientRuntime.hpp"
#include "tests/support/client_utils.hpp"
#include "tests/support/poll_utils.hpp"
#include "tests/support/server_harness.hpp"
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace grav_test_client_runtime_reload {
/// Description: Executes the writeTempXyz operation.
static std::filesystem::path writeTempXyz(const char* basename, const std::vector<float>& xs)
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

/// Description: Executes the TEST operation.
TEST(ClientRuntimeTest, TST_CNT_RUNT_008_LoadResetInvalidatesClientSnapshotCacheUntilReloaded)
{
    RealServerHarness server;
    std::string startError;
    ASSERT_TRUE(server.start(startError)) << startError;
    grav_client::ClientRuntime runtime(
        "simulation.ini", testsupport::makeTransport(server.port(), server.executablePath()));
    ASSERT_TRUE(runtime.start());
    std::optional<grav_client::ConsumedSnapshot> consumedSnapshot;
    ASSERT_TRUE(testsupport::waitUntil(
        [&]() {
            consumedSnapshot = runtime.consumeLatestSnapshot();
            return consumedSnapshot.has_value();
        },
        std::chrono::milliseconds(3000)));
    ASSERT_TRUE(consumedSnapshot.has_value());
    ASSERT_GT(consumedSnapshot->sourceSize, 0u);
    const std::filesystem::path xyz3 =
        writeTempXyz("grav_test_client_reload_3.xyz", {-2.0f, 0.0f, 2.0f});
    const std::filesystem::path xyz4 =
        writeTempXyz("grav_test_client_reload_4.xyz", {-3.0f, -1.0f, 1.0f, 3.0f});
    runtime.setInitialStateFile(xyz3.string(), "xyz");
    EXPECT_FALSE(runtime.consumeLatestSnapshot().has_value());
    EXPECT_EQ(runtime.snapshotAgeMs(), std::numeric_limits<std::uint32_t>::max());
    EXPECT_EQ(runtime.snapshotPipelineState().queueDepth, 0u);
    ASSERT_TRUE(testsupport::waitUntil(
        [&]() {
            consumedSnapshot = runtime.consumeLatestSnapshot();
            return consumedSnapshot.has_value() && consumedSnapshot->sourceSize == 3u;
        },
        std::chrono::milliseconds(4000)));
    runtime.setInitialStateFile(xyz4.string(), "xyz");
    EXPECT_FALSE(runtime.consumeLatestSnapshot().has_value());
    EXPECT_EQ(runtime.snapshotAgeMs(), std::numeric_limits<std::uint32_t>::max());
    EXPECT_EQ(runtime.snapshotPipelineState().queueDepth, 0u);
    ASSERT_TRUE(testsupport::waitUntil(
        [&]() {
            consumedSnapshot = runtime.consumeLatestSnapshot();
            return consumedSnapshot.has_value() && consumedSnapshot->sourceSize == 4u;
        },
        std::chrono::milliseconds(4000)));
    runtime.stop();
    server.stop();
    std::error_code ec;
    std::filesystem::remove(xyz3, ec);
    ec.clear();
    std::filesystem::remove(xyz4, ec);
}
} // namespace grav_test_client_runtime_reload
