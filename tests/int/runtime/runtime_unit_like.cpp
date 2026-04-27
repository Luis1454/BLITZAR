// File: tests/int/runtime/runtime_unit_like.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "client/ClientRuntime.hpp"
#include "tests/support/client_utils.hpp"
#include <chrono>
#include <gtest/gtest.h>
#include <limits>
#include <string>
#include <vector>
namespace grav_test_client_runtime_unit_like {
/// Description: Executes the TEST operation.
TEST(ClientRuntimeUnitLikeTest, TST_UNT_RUNT_016_RuntimeDefaultsRemainDeterministicBeforeStart)
{
    grav_client::ClientRuntime runtime(
        "simulation.ini",
        /// Description: Executes the makeTransport operation.
        testsupport::makeTransport(static_cast<std::uint16_t>(6553u), std::string()));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(runtime.linkStateLabel(), "reconnecting");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(runtime.serverOwnerLabel(), "external");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(runtime.statsAgeMs(), std::numeric_limits<std::uint32_t>::max());
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(runtime.snapshotAgeMs(), std::numeric_limits<std::uint32_t>::max());
    const grav_client::SnapshotPipelineState pipelineState = runtime.snapshotPipelineState();
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(pipelineState.queueDepth, 0u);
    /// Description: Executes the EXPECT_GE operation.
    EXPECT_GE(pipelineState.queueCapacity, 1u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(pipelineState.droppedFrames, 0u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(pipelineState.dropPolicy, "latest-only");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(pipelineState.latencyMs, std::numeric_limits<std::uint32_t>::max());
}
/// Description: Executes the TEST operation.
TEST(ClientRuntimeUnitLikeTest, TST_UNT_RUNT_017_RuntimeControlMethodsRemainBoundedWhenDisconnected)
{
    grav_client::ClientRuntime runtime(
        "simulation.ini",
        /// Description: Executes the makeTransport operation.
        testsupport::makeTransport(static_cast<std::uint16_t>(6553u), std::string()));
    const auto startedAt = std::chrono::steady_clock::now();
    /// Description: Executes the EXPECT_FALSE operation.
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
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(runtime.tryConsumeSnapshot(snapshot));
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(runtime.consumeLatestSnapshot().has_value());
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        /// Description: Executes the now operation.
        std::chrono::steady_clock::now() - startedAt);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(runtime.linkStateLabel(), "reconnecting");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(runtime.serverOwnerLabel(), "external");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(runtime.snapshotPipelineState().queueDepth, 0u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(runtime.snapshotAgeMs(), std::numeric_limits<std::uint32_t>::max());
    /// Description: Executes the EXPECT_LE operation.
    EXPECT_LE(elapsedMs.count(), 3000);
    runtime.stop();
}
} // namespace grav_test_client_runtime_unit_like
