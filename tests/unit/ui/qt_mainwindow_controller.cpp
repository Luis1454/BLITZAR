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
/// Description: Defines the RecordingClientRuntime data or behavior contract.
class RecordingClientRuntime final : public grav_client::IClientRuntime {
public:
    /// Description: Executes the start operation.
    bool start() override
    {
        return true;
    }
    /// Description: Executes the stop operation.
    void stop() override
    {
    }
    /// Description: Executes the setPaused operation.
    void setPaused(bool paused) override
    {
        pausedState = paused;
    }
    /// Description: Executes the togglePaused operation.
    void togglePaused() override
    {
        pausedState = !pausedState;
    }
    /// Description: Executes the stepOnce operation.
    void stepOnce() override
    {
    }
    /// Description: Executes the setParticleCount operation.
    void setParticleCount(std::uint32_t particleCount) override
    {
        configuredParticleCount = particleCount;
    }
    /// Description: Executes the setDt operation.
    void setDt(float dt) override
    {
        configuredDt = dt;
    }
    /// Description: Executes the scaleDt operation.
    void scaleDt(float factor) override
    {
        configuredDt *= factor;
    }
    /// Description: Executes the requestReset operation.
    void requestReset() override
    {
        resetRequested = true;
    }
    /// Description: Executes the requestRecover operation.
    void requestRecover() override
    {
    }
    /// Description: Executes the setSolverMode operation.
    void setSolverMode(const std::string& mode) override
    {
        solverMode = mode;
    }
    /// Description: Executes the setIntegratorMode operation.
    void setIntegratorMode(const std::string& mode) override
    {
        integratorMode = mode;
    }
    /// Description: Executes the setPerformanceProfile operation.
    void setPerformanceProfile(const std::string& profile) override
    {
        performanceProfile = profile;
    }
    /// Description: Executes the setOctreeParameters operation.
    void setOctreeParameters(float theta, float softening) override
    {
        octreeTheta = theta;
        octreeSoftening = softening;
    }
    /// Description: Executes the setSphEnabled operation.
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
    /// Description: Executes the setSubstepPolicy operation.
    void setSubstepPolicy(float targetDt, std::uint32_t maxSubsteps) override
    {
        substepTargetDt = targetDt;
        configuredMaxSubsteps = maxSubsteps;
    }
    /// Description: Executes the setSnapshotPublishPeriodMs operation.
    void setSnapshotPublishPeriodMs(std::uint32_t periodMs) override
    {
        snapshotPublishPeriodMs = periodMs;
    }
    /// Description: Executes the setInitialStateConfig operation.
    void setInitialStateConfig(const InitialStateConfig& config) override
    {
        initialStateConfig = config;
    }
    /// Description: Executes the setEnergyMeasurementConfig operation.
    void setEnergyMeasurementConfig(std::uint32_t everySteps, std::uint32_t sampleLimit) override
    {
        energyEverySteps = everySteps;
        energySampleLimit = sampleLimit;
    }
    /// Description: Executes the setGpuTelemetryEnabled operation.
    void setGpuTelemetryEnabled(bool enabled) override
    {
        gpuTelemetryEnabled = enabled;
    }
    /// Description: Executes the setExportDefaults operation.
    void setExportDefaults(const std::string& directory, const std::string& format) override
    {
        exportDirectory = directory;
        exportFormat = format;
    }
    /// Description: Executes the setInitialStateFile operation.
    void setInitialStateFile(const std::string& path, const std::string& format) override
    {
        initialStateFile = path;
        initialStateFormat = format;
    }
    /// Description: Executes the requestExportSnapshot operation.
    void requestExportSnapshot(const std::string&, const std::string&) override
    {
    }
    /// Description: Executes the requestSaveCheckpoint operation.
    void requestSaveCheckpoint(const std::string& outputPath) override
    {
        checkpointSavePath = outputPath;
    }
    /// Description: Executes the requestLoadCheckpoint operation.
    void requestLoadCheckpoint(const std::string& inputPath) override
    {
        checkpointLoadPath = inputPath;
    }
    /// Description: Executes the requestShutdown operation.
    void requestShutdown() override
    {
    }
    /// Description: Executes the setRemoteSnapshotCap operation.
    void setRemoteSnapshotCap(std::uint32_t maxPoints) override
    {
        remoteSnapshotCap = maxPoints;
    }
    /// Description: Executes the requestReconnect operation.
    void requestReconnect() override
    {
    }
    void configureRemoteConnector(const std::string&, std::uint16_t, bool,
                                  const std::string&) override
    {
    }
    /// Description: Executes the isRemoteMode operation.
    bool isRemoteMode() const override
    {
        return false;
    }
    /// Description: Executes the getCachedStats operation.
    SimulationStats getCachedStats() const override
    {
        return {};
    }
    /// Description: Executes the getStats operation.
    SimulationStats getStats() const override
    {
        return {};
    }
    /// Description: Executes the consumeLatestSnapshot operation.
    std::optional<grav_client::ConsumedSnapshot> consumeLatestSnapshot() override
    {
        return std::nullopt;
    }
    /// Description: Executes the tryConsumeSnapshot operation.
    bool tryConsumeSnapshot(std::vector<RenderParticle>&) override
    {
        return false;
    }
    /// Description: Executes the snapshotPipelineState operation.
    grav_client::SnapshotPipelineState snapshotPipelineState() const override
    {
        return {};
    }
    /// Description: Executes the linkStateLabel operation.
    std::string linkStateLabel() const override
    {
        return "disconnected";
    }
    /// Description: Executes the serverOwnerLabel operation.
    std::string serverOwnerLabel() const override
    {
        return "local";
    }
    /// Description: Executes the statsAgeMs operation.
    std::uint32_t statsAgeMs() const override
    {
        return 0u;
    }
    /// Description: Executes the snapshotAgeMs operation.
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
/// Description: Executes the TEST operation.
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
        /// Description: Executes the MainWindowController operation.
        grav_qt::MainWindowController().applyConfig(config, runtime, true);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(result.applied);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(result.report.validForRun);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(runtime.resetRequested);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(runtime.configuredParticleCount, config.particleCount);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(runtime.solverMode, config.solver);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(runtime.integratorMode, config.integrator);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(runtime.performanceProfile, config.performanceProfile);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(runtime.remoteSnapshotCap, result.clientDrawCap);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(runtime.exportDirectory, config.exportDirectory);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(runtime.exportFormat, config.exportFormat);
}
/// Description: Executes the TEST operation.
TEST(QtUiLogicTest, TST_UNT_UI_003_ControllerRejectsInvalidConfigBeforeRuntimeMutation)
{
    SimulationConfig config{};
    config.clientSnapshotQueueCapacity = 0u;
    RecordingClientRuntime runtime;
    const grav_qt::MainWindowApplyConfigResult result =
        /// Description: Executes the MainWindowController operation.
        grav_qt::MainWindowController().applyConfig(config, runtime, true);
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(result.applied);
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(result.report.validForRun);
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(runtime.resetRequested);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(runtime.configuredParticleCount, 0u);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(runtime.solverMode.empty());
}
} // namespace grav_test_qt_ui
