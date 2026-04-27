// File: tests/unit/ui/qt_mainwindow_controller.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "client/IClientRuntime.hpp"
#include "config/SimulationConfig.hpp"
#include "ui/MainWindowController.hpp"
#include <cstdint>
#include <gtest/gtest.h>
#include <optional>
#include <string>
#include <vector>
namespace grav_test_qt_ui {
class RecordingClientRuntime final : public grav_client::IClientRuntime {
public:
    bool start() override
    {
        return true;
    }
    void stop() override
    {
    }
    void setPaused(bool paused) override
    {
        pausedState = paused;
    }
    void togglePaused() override
    {
        pausedState = !pausedState;
    }
    void stepOnce() override
    {
    }
    void setParticleCount(std::uint32_t particleCount) override
    {
        configuredParticleCount = particleCount;
    }
    void setDt(float dt) override
    {
        configuredDt = dt;
    }
    void scaleDt(float factor) override
    {
        configuredDt *= factor;
    }
    void requestReset() override
    {
        resetRequested = true;
    }
    void requestRecover() override
    {
    }
    void setSolverMode(const std::string& mode) override
    {
        solverMode = mode;
    }
    void setIntegratorMode(const std::string& mode) override
    {
        integratorMode = mode;
    }
    void setPerformanceProfile(const std::string& profile) override
    {
        performanceProfile = profile;
    }
    void setOctreeParameters(float theta, float softening) override
    {
        octreeTheta = theta;
        octreeSoftening = softening;
    }
    void setSphEnabled(bool enabled) override
    {
        sphEnabled = enabled;
    }
    void setSphParameters(float smoothingLength, float restDensity, float gasConstant,
                          float viscosity) override
    {
        sphSmoothingLength = smoothingLength;
        sphRestDensity = restDensity;
        sphGasConstant = gasConstant;
        sphViscosity = viscosity;
    }
    void setSubstepPolicy(float targetDt, std::uint32_t maxSubsteps) override
    {
        substepTargetDt = targetDt;
        configuredMaxSubsteps = maxSubsteps;
    }
    void setSnapshotPublishPeriodMs(std::uint32_t periodMs) override
    {
        snapshotPublishPeriodMs = periodMs;
    }
    void setInitialStateConfig(const InitialStateConfig& config) override
    {
        initialStateConfig = config;
    }
    void setEnergyMeasurementConfig(std::uint32_t everySteps, std::uint32_t sampleLimit) override
    {
        energyEverySteps = everySteps;
        energySampleLimit = sampleLimit;
    }
    void setGpuTelemetryEnabled(bool enabled) override
    {
        gpuTelemetryEnabled = enabled;
    }
    void setExportDefaults(const std::string& directory, const std::string& format) override
    {
        exportDirectory = directory;
        exportFormat = format;
    }
    void setInitialStateFile(const std::string& path, const std::string& format) override
    {
        initialStateFile = path;
        initialStateFormat = format;
    }
    void requestExportSnapshot(const std::string&, const std::string&) override
    {
    }
    void requestSaveCheckpoint(const std::string& outputPath) override
    {
        checkpointSavePath = outputPath;
    }
    void requestLoadCheckpoint(const std::string& inputPath) override
    {
        checkpointLoadPath = inputPath;
    }
    void requestShutdown() override
    {
    }
    void setRemoteSnapshotCap(std::uint32_t maxPoints) override
    {
        remoteSnapshotCap = maxPoints;
    }
    void requestReconnect() override
    {
    }
    void configureRemoteConnector(const std::string&, std::uint16_t, bool,
                                  const std::string&) override
    {
    }
    bool isRemoteMode() const override
    {
        return false;
    }
    SimulationStats getCachedStats() const override
    {
        return {};
    }
    SimulationStats getStats() const override
    {
        return {};
    }
    std::optional<grav_client::ConsumedSnapshot> consumeLatestSnapshot() override
    {
        return std::nullopt;
    }
    bool tryConsumeSnapshot(std::vector<RenderParticle>&) override
    {
        return false;
    }
    grav_client::SnapshotPipelineState snapshotPipelineState() const override
    {
        return {};
    }
    std::string linkStateLabel() const override
    {
        return "disconnected";
    }
    std::string serverOwnerLabel() const override
    {
        return "local";
    }
    std::uint32_t statsAgeMs() const override
    {
        return 0u;
    }
    std::uint32_t snapshotAgeMs() const override
    {
        return 0u;
    }
    bool pausedState = false;
    bool resetRequested = false;
    bool sphEnabled = false;
    std::uint32_t configuredParticleCount = 0u;
    std::uint32_t configuredMaxSubsteps = 0u;
    std::uint32_t snapshotPublishPeriodMs = 0u;
    std::uint32_t energyEverySteps = 0u;
    std::uint32_t energySampleLimit = 0u;
    std::uint32_t remoteSnapshotCap = 0u;
    bool gpuTelemetryEnabled = false;
    float configuredDt = 0.0f;
    float octreeTheta = 0.0f;
    float octreeSoftening = 0.0f;
    float sphSmoothingLength = 0.0f;
    float sphRestDensity = 0.0f;
    float sphGasConstant = 0.0f;
    float sphViscosity = 0.0f;
    float substepTargetDt = 0.0f;
    std::string solverMode;
    std::string integratorMode;
    std::string performanceProfile;
    std::string exportDirectory;
    std::string exportFormat;
    std::string initialStateFile;
    std::string initialStateFormat;
    std::string checkpointSavePath;
    std::string checkpointLoadPath;
    InitialStateConfig initialStateConfig;
};
TEST(QtUiLogicTest, TST_UNT_UI_002_ControllerAppliesRuntimeMappingsWithoutWidgetDependency)
{
    SimulationConfig config{};
    config.particleCount = 2048u;
    config.dt = 0.02f;
    config.solver = "octree_gpu";
    config.integrator = "rk4";
    config.performanceProfile = "balanced";
    config.octreeTheta = 0.7f;
    config.octreeSoftening = 0.02f;
    config.sphEnabled = true;
    config.sphSmoothingLength = 0.5f;
    config.sphRestDensity = 2.0f;
    config.sphGasConstant = 3.0f;
    config.sphViscosity = 0.4f;
    config.substepTargetDt = 0.01f;
    config.maxSubsteps = 4u;
    config.snapshotPublishPeriodMs = 20u;
    config.energyMeasureEverySteps = 5u;
    config.energySampleLimit = 12u;
    config.exportDirectory = "exports";
    config.exportFormat = "vtk";
    config.clientParticleCap = 1234u;
    RecordingClientRuntime runtime;
    const grav_qt::MainWindowApplyConfigResult result =
        grav_qt::MainWindowController().applyConfig(config, runtime, true);
    ASSERT_TRUE(result.applied);
    EXPECT_TRUE(result.report.validForRun);
    EXPECT_TRUE(runtime.resetRequested);
    EXPECT_EQ(runtime.configuredParticleCount, config.particleCount);
    EXPECT_EQ(runtime.solverMode, config.solver);
    EXPECT_EQ(runtime.integratorMode, config.integrator);
    EXPECT_EQ(runtime.performanceProfile, config.performanceProfile);
    EXPECT_EQ(runtime.remoteSnapshotCap, result.clientDrawCap);
    EXPECT_EQ(runtime.exportDirectory, config.exportDirectory);
    EXPECT_EQ(runtime.exportFormat, config.exportFormat);
}
TEST(QtUiLogicTest, TST_UNT_UI_003_ControllerRejectsInvalidConfigBeforeRuntimeMutation)
{
    SimulationConfig config{};
    config.clientSnapshotQueueCapacity = 0u;
    RecordingClientRuntime runtime;
    const grav_qt::MainWindowApplyConfigResult result =
        grav_qt::MainWindowController().applyConfig(config, runtime, true);
    EXPECT_FALSE(result.applied);
    EXPECT_FALSE(result.report.validForRun);
    EXPECT_FALSE(runtime.resetRequested);
    EXPECT_EQ(runtime.configuredParticleCount, 0u);
    EXPECT_TRUE(runtime.solverMode.empty());
}
} // namespace grav_test_qt_ui
