/*
 * @file tests/int/runtime/runtime_unit_like.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#include "client/ClientRuntime.hpp"
#include "tests/support/client_utils.hpp"
#include <chrono>
#include <gtest/gtest.h>
#include <limits>
#include <string>
#include <vector>

namespace bltzr_test_client_runtime_unit_like {
TEST(ClientRuntimeUnitLikeTest, TST_UNT_RUNT_016_RuntimeDefaultsRemainDeterministicBeforeStart)
{
    bltzr_client::ClientRuntime runtime(
        "simulation.ini",
        testsupport::makeTransport(static_cast<std::uint16_t>(6553u), std::string()));
    EXPECT_EQ(runtime.linkStateLabel(), "reconnecting");
    EXPECT_EQ(runtime.serverOwnerLabel(), "external");
    EXPECT_EQ(runtime.statsAgeMs(), std::numeric_limits<std::uint32_t>::max());
    EXPECT_EQ(runtime.snapshotAgeMs(), std::numeric_limits<std::uint32_t>::max());
    const bltzr_client::SnapshotPipelineState pipelineState = runtime.snapshotPipelineState();
    EXPECT_EQ(pipelineState.queueDepth, 0u);
    EXPECT_GE(pipelineState.queueCapacity, 1u);
    EXPECT_EQ(pipelineState.droppedFrames, 0u);
    EXPECT_EQ(pipelineState.dropPolicy, "latest-only");
    EXPECT_EQ(pipelineState.latencyMs, std::numeric_limits<std::uint32_t>::max());
}

TEST(ClientRuntimeUnitLikeTest, TST_UNT_RUNT_017_RuntimeControlMethodsRemainBoundedWhenDisconnected)
{
    bltzr_client::ClientRuntime runtime(
        "simulation.ini",
        testsupport::makeTransport(static_cast<std::uint16_t>(6553u), std::string()));
    const auto startedAt = std::chrono::steady_clock::now();
    EXPECT_FALSE(runtime.start());
    runtime.setPaused(true);
    runtime.togglePaused();
    runtime.stepOnce();
    runtime.setParticleCount(1u);
    runtime.setDt(-1.0f);
    runtime.scaleDt(0.0f);
    runtime.requestReset();
    runtime.requestRecover();
    runtime.setSolverMode("octree_cpu");
    runtime.setIntegratorMode("rk4");
    runtime.setPerformanceProfile("balanced");
    runtime.setOctreeParameters(-1.0f, -1.0f);
    runtime.setSphEnabled(true);
    runtime.setSphParameters(-1.0f, -2.0f, -3.0f, -4.0f);
    runtime.setSubstepPolicy(-1.0f, 0u);
    runtime.setSnapshotPublishPeriodMs(0u);
    InitialStateConfig init{};
    init.mode = "random_cloud";
    runtime.setInitialStateConfig(init);
    runtime.setEnergyMeasurementConfig(0u, 0u);
    runtime.setGpuTelemetryEnabled(true);
    runtime.setExportDefaults("exports", "vtk");
    runtime.setInitialStateFile("missing.xyz", "xyz");
    runtime.requestExportSnapshot("out.snap", "vtk");
    runtime.requestSaveCheckpoint("state.chk");
    runtime.requestLoadCheckpoint("missing.chk");
    runtime.setRemoteSnapshotCap(1u);
    runtime.requestReconnect();
    runtime.configureRemoteConnector("127.0.0.1", static_cast<std::uint16_t>(6554u), false, "");
    std::vector<RenderParticle> snapshot;
    EXPECT_FALSE(runtime.tryConsumeSnapshot(snapshot));
    EXPECT_FALSE(runtime.consumeLatestSnapshot().has_value());
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startedAt);
    EXPECT_EQ(runtime.linkStateLabel(), "reconnecting");
    EXPECT_EQ(runtime.serverOwnerLabel(), "external");
    EXPECT_EQ(runtime.snapshotPipelineState().queueDepth, 0u);
    EXPECT_EQ(runtime.snapshotAgeMs(), std::numeric_limits<std::uint32_t>::max());
    EXPECT_LE(elapsedMs.count(), 3000);
    runtime.stop();
}
} // namespace bltzr_test_client_runtime_unit_like
