#include "ui/MainWindowPresenter.hpp"

#include <gtest/gtest.h>

#include <limits>
#include <string>

namespace grav_test_qt_ui {

TEST(QtUiLogicTest, TST_UNT_UI_001_PresenterFormatsStatusAndTraceFromRuntimeState)
{
    grav_qt::MainWindowPresentationInput input;
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

    const grav_qt::MainWindowPresentation presentation = grav_qt::MainWindowPresenter().present(input);

    EXPECT_NE(presentation.statusText.find("link=connected"), std::string::npos);
    EXPECT_NE(presentation.statusText.find("q=2/4"), std::string::npos);
    EXPECT_NE(presentation.statusText.find("policy=latest-only"), std::string::npos);
    EXPECT_NE(presentation.statusText.find("lat=7ms"), std::string::npos);
    EXPECT_NE(presentation.consoleTrace.find("queue_depth=2"), std::string::npos);
    EXPECT_NE(presentation.consoleTrace.find("drop_policy=latest-only"), std::string::npos);
}

} // namespace grav_test_qt_ui
