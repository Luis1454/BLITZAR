#include "ui/MainWindowPresenter.hpp"

#include <gtest/gtest.h>

#include <limits>
#include <string>

namespace grav_test_qt_ui {

TEST(QtUiLogicTest, TST_UNT_UI_001_PresenterFormatsStatusAndTraceFromRuntimeState)
{
    grav_qt::MainWindowPresentationInput input;
    input.stats = {};
    input.linkLabel = "connected";
    input.ownerLabel = "external";
    input.performanceProfile = "interactive";
    input.displayedParticles = 512u;
    input.clientDrawCap = 2048u;
    input.statsAgeMs = 12u;
    input.snapshotAgeMs = 18u;
    input.snapshotLatencyMs = 7u;
    input.uiTickFps = 59.5f;
    input.snapshotPipeline.queueDepth = 2u;
    input.snapshotPipeline.queueCapacity = 4u;
    input.snapshotPipeline.droppedFrames = 3u;
    input.snapshotPipeline.dropPolicy = "latest-only";
    input.stats.paused = false;
    input.stats.faulted = false;
    input.stats.faultStep = 0u;
    input.stats.solverName = "pairwise_cuda";
    input.stats.integratorName = "euler";
    input.stats.performanceProfile = "interactive";
    input.stats.sphEnabled = false;
    input.stats.dt = 0.01f;
    input.stats.serverFps = 120.0f;
    input.stats.substeps = 2u;
    input.stats.substepDt = 0.005f;
    input.stats.substepTargetDt = 0.005f;
    input.stats.maxSubsteps = 8u;
    input.stats.snapshotPublishPeriodMs = 16u;
    input.stats.steps = 42u;
    input.stats.particleCount = 4096u;
    input.stats.kineticEnergy = 5.0f;
    input.stats.potentialEnergy = -7.0f;
    input.stats.thermalEnergy = 1.0f;
    input.stats.radiatedEnergy = 0.5f;
    input.stats.totalEnergy = -0.5f;
    input.stats.energyDriftPct = 0.25f;
    input.stats.energyEstimated = false;
    input.stats.gpuTelemetryEnabled = true;
    input.stats.gpuTelemetryAvailable = true;
    input.stats.gpuKernelMs = 1.234f;
    input.stats.gpuCopyMs = 0.456f;
    input.stats.gpuVramUsedBytes = 128u * 1024u * 1024u;
    input.stats.gpuVramTotalBytes = 512u * 1024u * 1024u;
    input.stats.exportQueueDepth = 2u;
    input.stats.exportActive = true;
    input.stats.exportCompletedCount = 4u;
    input.stats.exportFailedCount = 1u;
    input.stats.exportLastState = "writing";
    input.stats.exportLastPath = "exports/demo.vtk";
    input.stats.exportLastMessage = "background export active";

    const grav_qt::MainWindowPresentation presentation = grav_qt::MainWindowPresenter().present(input);

    EXPECT_NE(presentation.headlineText.find("Backend: busy"), std::string::npos) << presentation.headlineText;
    EXPECT_NE(presentation.headlineText.find("Viewport: fresh (18ms old)"), std::string::npos);
    EXPECT_NE(presentation.headlineText.find("Link: connected"), std::string::npos);
    EXPECT_NE(presentation.headlineText.find("Owner: external"), std::string::npos);
    EXPECT_NE(presentation.runtimeText.find("Sim rate: 1.20 sim s/s"), std::string::npos);
    EXPECT_NE(presentation.runtimeText.find("Substeps: 2 x 0.005"), std::string::npos);
    EXPECT_NE(presentation.queueText.find("Progress: open-ended"), std::string::npos);
    EXPECT_NE(presentation.queueText.find("ETA: n/a"), std::string::npos);
    EXPECT_NE(presentation.queueText.find("Export: writing"), std::string::npos);
    EXPECT_NE(presentation.queueText.find("Export backlog: 2"), std::string::npos);
    EXPECT_NE(presentation.queueText.find("Export done: 4"), std::string::npos);
    EXPECT_NE(presentation.queueText.find("Export failed: 1"), std::string::npos);
    EXPECT_NE(presentation.queueText.find("Last export: exports/demo.vtk"), std::string::npos);
    EXPECT_NE(presentation.queueText.find("Queue: 2 / 4"), std::string::npos);
    EXPECT_NE(presentation.queueText.find("Policy: latest-only"), std::string::npos);
    EXPECT_NE(presentation.queueText.find("Latency: 7ms"), std::string::npos);
    EXPECT_NE(presentation.energyText.find("Total: -0.5"), std::string::npos);
    EXPECT_NE(presentation.gpuText.find("Kernel: 1.234 ms"), std::string::npos);
    EXPECT_NE(presentation.gpuText.find("Copy: 0.456 ms"), std::string::npos);
    EXPECT_NE(presentation.gpuText.find("VRAM: 128.0 MiB / 512.0 MiB"), std::string::npos);
    EXPECT_NE(presentation.statusText.find('\n'), std::string::npos);
    EXPECT_NE(presentation.consoleTrace.find("backend_state=busy"), std::string::npos) << presentation.consoleTrace;
    EXPECT_NE(presentation.consoleTrace.find("gpu_telemetry=on"), std::string::npos);
    EXPECT_NE(presentation.consoleTrace.find("progress=\"open-ended\""), std::string::npos);
    EXPECT_NE(presentation.consoleTrace.find("export_state=writing"), std::string::npos);
    EXPECT_NE(presentation.consoleTrace.find("export_backlog=2"), std::string::npos);
    EXPECT_NE(presentation.consoleTrace.find("queue_depth=2"), std::string::npos);
    EXPECT_NE(presentation.consoleTrace.find("drop_policy=latest-only"), std::string::npos);
}

TEST(QtUiLogicTest, TST_UNT_UI_005_PresenterHighlightsStaleBackendAndKnownHorizonEta)
{
    grav_qt::MainWindowPresentationInput input;
    input.stats = {};
    input.linkLabel = "connected";
    input.ownerLabel = "embedded";
    input.performanceProfile = "quality";
    input.statsAgeMs = 2100u;
    input.snapshotAgeMs = 2600u;
    input.snapshotLatencyMs = std::numeric_limits<std::uint32_t>::max();
    input.simulationHorizonSeconds = 10.0f;
    input.snapshotPipeline.latencyMs = 45u;
    input.snapshotPipeline.dropPolicy = "latest-only";
    input.stats.paused = false;
    input.stats.faulted = false;
    input.stats.faultStep = 0u;
    input.stats.sphEnabled = false;
    input.stats.serverFps = 80.0f;
    input.stats.dt = 0.02f;
    input.stats.totalTime = 4.0f;
    input.stats.substeps = 0u;
    input.stats.substepDt = 0.0f;
    input.stats.substepTargetDt = 0.0f;
    input.stats.maxSubsteps = 0u;
    input.stats.snapshotPublishPeriodMs = 0u;
    input.stats.steps = 200u;
    input.stats.particleCount = 16384u;
    input.stats.solverName = "octree_gpu";
    input.stats.integratorName = "rk4";
    input.stats.kineticEnergy = 0.0f;
    input.stats.potentialEnergy = 0.0f;
    input.stats.thermalEnergy = 0.0f;
    input.stats.radiatedEnergy = 0.0f;
    input.stats.totalEnergy = 0.0f;
    input.stats.energyDriftPct = 0.0f;
    input.stats.energyEstimated = false;

    const grav_qt::MainWindowPresentation presentation = grav_qt::MainWindowPresenter().present(input);

    EXPECT_NE(presentation.headlineText.find("Backend: stalled"), std::string::npos) << presentation.headlineText;
    EXPECT_NE(presentation.headlineText.find("Viewport: stale (2600ms old)"), std::string::npos);
    EXPECT_NE(presentation.queueText.find("Progress: 40.0% (4.00 / 10.00 s)"), std::string::npos);
    EXPECT_NE(presentation.queueText.find("ETA: "), std::string::npos) << presentation.queueText;
    EXPECT_EQ(presentation.queueText.find("ETA: n/a"), std::string::npos) << presentation.queueText;
    EXPECT_NE(presentation.consoleTrace.find("backend_state=stalled"), std::string::npos) << presentation.consoleTrace;
    EXPECT_NE(presentation.consoleTrace.find("eta=\""), std::string::npos) << presentation.consoleTrace;
    EXPECT_EQ(presentation.consoleTrace.find("eta=\"n/a\""), std::string::npos) << presentation.consoleTrace;
}

TEST(QtUiLogicTest, TST_UNT_UI_008_PresenterReportsGpuTelemetryWaitingState)
{
    grav_qt::MainWindowPresentationInput input;
    input.stats = {};
    input.linkLabel = "connected";
    input.ownerLabel = "embedded";
    input.performanceProfile = "interactive";
    input.stats.gpuTelemetryEnabled = true;
    input.stats.gpuTelemetryAvailable = false;

    const grav_qt::MainWindowPresentation presentation = grav_qt::MainWindowPresenter().present(input);

    EXPECT_NE(presentation.gpuText.find("State: waiting"), std::string::npos) << presentation.gpuText;
    EXPECT_NE(presentation.gpuText.find("Sampling: every 8 steps"), std::string::npos);
    EXPECT_NE(presentation.statusText.find("State: waiting"), std::string::npos);
}

} // namespace grav_test_qt_ui
