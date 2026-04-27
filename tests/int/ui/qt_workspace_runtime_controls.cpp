// File: tests/int/ui/qt_workspace_runtime_controls.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "tests/support/qt_test_utils.hpp"
#include "ui/EnergyGraphWidget.hpp"
#include "ui/MainWindow.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDoubleSpinBox>
#include <QEventLoop>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <chrono>
#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>
namespace grav_test_qt_workspace_runtime_controls {
/// Description: Defines the RecordingRuntime data or behavior contract.
class RecordingRuntime final : public grav_client::IClientRuntime {
public:
    /// Description: Executes the start operation.
    bool start() override
    {
        return false;
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
        stepCount += 1;
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
        resetCount += 1;
    }
    /// Description: Executes the requestRecover operation.
    void requestRecover() override
    {
        recoverCount += 1;
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
    /// Description: Executes the setSphParameters operation.
    void setSphParameters(float h, float rho, float gasK, float visc) override
    {
        sphSmoothingLength = h;
        sphRestDensity = rho;
        sphGasConstant = gasK;
        sphViscosity = visc;
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
    void requestSaveCheckpoint(const std::string&) override
    {
    }
    /// Description: Executes the requestLoadCheckpoint operation.
    void requestLoadCheckpoint(const std::string&) override
    {
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
        reconnectCount += 1;
    }
    void configureRemoteConnector(const std::string& host, std::uint16_t port, bool autoStart,
                                  const std::string& serverExecutable) override
    {
        connectorHost = host;
        connectorPort = port;
        connectorAutoStart = autoStart;
        connectorServerExecutable = serverExecutable;
    }
    /// Description: Executes the isRemoteMode operation.
    bool isRemoteMode() const override
    {
        return true;
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
        return "external";
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
    bool sphEnabled = false;
    bool connectorAutoStart = false;
    std::uint32_t configuredParticleCount = 0u;
    std::uint32_t configuredMaxSubsteps = 0u;
    std::uint32_t snapshotPublishPeriodMs = 0u;
    std::uint32_t energyEverySteps = 0u;
    std::uint32_t energySampleLimit = 0u;
    std::uint32_t remoteSnapshotCap = 0u;
    std::uint32_t stepCount = 0u;
    std::uint32_t resetCount = 0u;
    std::uint32_t recoverCount = 0u;
    std::uint32_t reconnectCount = 0u;
    std::uint16_t connectorPort = 0u;
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
    std::string connectorHost;
    std::string connectorServerExecutable;
    InitialStateConfig initialStateConfig;
};
/// Description: Executes the makeRuntimeUiConfig operation.
static SimulationConfig makeRuntimeUiConfig()
{
    SimulationConfig config{};
    config.uiFpsLimit = 60u;
    config.exportDirectory = "exports";
    config.exportFormat = "vtk";
    config.solver = "pairwise_cuda";
    config.integrator = "euler";
    config.sphEnabled = false;
    config.dt = 0.01f;
    return config;
}
/// Description: Executes the TEST operation.
TEST(QtWorkspaceRuntimeControlsTest, TST_UIX_UI_010_RunButtonsAndConnectorForwardToRuntime)
{
    (void)testsupport::ensureQtApp();
    auto runtime = std::make_unique<RecordingRuntime>();
    RecordingRuntime* spy = runtime.get();
    /// Description: Executes the window operation.
    grav_qt::MainWindow window(makeRuntimeUiConfig(), "simulation.ini", std::move(runtime));
    window.show();
    /// Description: Executes the processEvents operation.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    window.findChild<QLineEdit*>("serverHostEdit")->setText("10.0.0.4");
    window.findChild<QSpinBox*>("serverPortSpin")->setValue(4567);
    window.findChild<QLineEdit*>("serverBinaryEdit")->setText("C:/bin/blitzar-server.exe");
    window.findChild<QCheckBox*>("serverAutostartCheck")->setChecked(true);
    window.findChild<QPushButton*>("connectButton")->click();
    window.findChild<QPushButton*>("pauseToggleButton")->click();
    window.findChild<QPushButton*>("stepButton")->click();
    window.findChild<QPushButton*>("resetButton")->click();
    window.findChild<QPushButton*>("recoverButton")->click();
    /// Description: Executes the processEvents operation.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(spy->connectorHost, "10.0.0.4");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(spy->connectorPort, 4567u);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(spy->connectorAutoStart);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(spy->connectorServerExecutable, "C:/bin/blitzar-server.exe");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(spy->reconnectCount, 1u);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(spy->pausedState);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(window.findChild<QPushButton*>("pauseToggleButton")->text().toStdString(), "Resume");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(spy->stepCount, 1u);
    /// Description: Executes the EXPECT_GE operation.
    EXPECT_GE(spy->resetCount, 1u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(spy->recoverCount, 1u);
}
/// Description: Executes the TEST operation.
TEST(QtWorkspaceRuntimeControlsTest, TST_UIX_UI_011_RunAndSceneProfilesPropagateToRuntimeAndUi)
{
    (void)testsupport::ensureQtApp();
    auto runtime = std::make_unique<RecordingRuntime>();
    RecordingRuntime* spy = runtime.get();
    /// Description: Executes the window operation.
    grav_qt::MainWindow window(makeRuntimeUiConfig(), "simulation.ini", std::move(runtime));
    window.show();
    /// Description: Executes the processEvents operation.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    window.findChild<QComboBox*>("performanceProfileCombo")->setCurrentText("balanced");
    /// Description: Executes the processEvents operation.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(spy->performanceProfile, "balanced");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(spy->snapshotPublishPeriodMs, 33u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(spy->energyEverySteps, 20u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(spy->energySampleLimit, 1024u);
    /// Description: Executes the EXPECT_FLOAT_EQ operation.
    EXPECT_FLOAT_EQ(spy->substepTargetDt, 0.005f);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(spy->configuredMaxSubsteps, 8u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(spy->remoteSnapshotCap, 8192u);
    window.findChild<QComboBox*>("simulationProfileCombo")->setCurrentText("binary_star");
    /// Description: Executes the processEvents operation.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    EXPECT_EQ(window.findChild<QComboBox*>("solverCombo")->currentText().toStdString(),
              "pairwise_cuda");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(window.findChild<QComboBox*>("integratorCombo")->currentText().toStdString(), "rk4");
    EXPECT_EQ(window.findChild<QComboBox*>("scenePresetCombo")->currentText().toStdString(),
              "two_body");
    /// Description: Executes the EXPECT_NEAR operation.
    EXPECT_NEAR(window.findChild<QDoubleSpinBox*>("dtSpin")->value(), 0.001, 1e-6);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(spy->initialStateConfig.mode, "two_body");
    /// Description: Executes the EXPECT_GE operation.
    EXPECT_GE(spy->resetCount, 1u);
}
/// Description: Executes the TEST operation.
TEST(QtWorkspaceRuntimeControlsTest, TST_UIX_UI_012_PhysicsAndRenderControlsPersistWorkspaceChoices)
{
    (void)testsupport::ensureQtApp();
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path configPath =
        std::filesystem::temp_directory_path() /
        ("gravity_qt_workspace_runtime_" + std::to_string(stamp) + ".ini");
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(makeRuntimeUiConfig().save(configPath.string()));
    auto runtime = std::make_unique<RecordingRuntime>();
    RecordingRuntime* spy = runtime.get();
    /// Description: Executes the window operation.
    grav_qt::MainWindow window(makeRuntimeUiConfig(), configPath.string(), std::move(runtime));
    window.show();
    /// Description: Executes the processEvents operation.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    window.findChild<QComboBox*>("solverCombo")->setCurrentText("octree_cpu");
    window.findChild<QComboBox*>("integratorCombo")->setCurrentText("euler");
    window.findChild<QDoubleSpinBox*>("dtSpin")->setValue(0.25);
    window.findChild<QDoubleSpinBox*>("octreeThetaSpin")->setValue(0.65);
    window.findChild<QDoubleSpinBox*>("octreeSofteningSpin")->setValue(0.1234);
    window.findChild<QCheckBox*>("sphEnabledCheck")->setChecked(true);
    window.findChild<QDoubleSpinBox*>("sphSmoothingSpin")->setValue(0.8);
    window.findChild<QDoubleSpinBox*>("sphRestDensitySpin")->setValue(2.5);
    window.findChild<QDoubleSpinBox*>("sphGasConstantSpin")->setValue(5.5);
    window.findChild<QDoubleSpinBox*>("sphViscositySpin")->setValue(0.33);
    window.findChild<QCheckBox*>("renderCullingCheck")->setChecked(false);
    window.findChild<QCheckBox*>("renderLodCheck")->setChecked(false);
    window.findChild<QSlider*>("zoomSlider")->setValue(120);
    window.findChild<QSlider*>("luminositySlider")->setValue(77);
    window.findChild<QComboBox*>("view3dModeCombo")->setCurrentText("iso");
    window.findChild<QSlider*>("yawSlider")->setValue(25);
    window.findChild<QSlider*>("pitchSlider")->setValue(-15);
    window.findChild<QSlider*>("rollSlider")->setValue(35);
    window.findChild<QPushButton*>("saveConfigButton")->click();
    ASSERT_TRUE(testsupport::waitUntilUi(
        [&]() {
            const std::string saved = testsupport::readAllFile(configPath);
            return saved.find("simulation(particle_count=10000, dt=0.25, solver=octree_cpu, "
                              "integrator=euler)") != std::string::npos &&
                   saved.find("octree(theta=0.65, softening=0.1234") != std::string::npos &&
                   saved.find("client(zoom=12, luminosity=77") != std::string::npos &&
                   saved.find("sph(enabled=true, smoothing_length=0.8, rest_density=2.5, "
                              "gas_constant=5.5, viscosity=0.33") != std::string::npos &&
                   saved.find("render(culling=false, lod=false") != std::string::npos;
        },
        /// Description: Executes the milliseconds operation.
        std::chrono::milliseconds(2000)));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(spy->solverMode, "octree_cpu");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(spy->integratorMode, "euler");
    /// Description: Executes the EXPECT_FLOAT_EQ operation.
    EXPECT_FLOAT_EQ(spy->configuredDt, 0.25f);
    /// Description: Executes the EXPECT_FLOAT_EQ operation.
    EXPECT_FLOAT_EQ(spy->octreeTheta, 0.65f);
    /// Description: Executes the EXPECT_FLOAT_EQ operation.
    EXPECT_FLOAT_EQ(spy->octreeSoftening, 0.1234f);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(spy->sphEnabled);
    /// Description: Executes the EXPECT_FLOAT_EQ operation.
    EXPECT_FLOAT_EQ(spy->sphSmoothingLength, 0.8f);
    /// Description: Executes the EXPECT_FLOAT_EQ operation.
    EXPECT_FLOAT_EQ(spy->sphRestDensity, 2.5f);
    /// Description: Executes the EXPECT_FLOAT_EQ operation.
    EXPECT_FLOAT_EQ(spy->sphGasConstant, 5.5f);
    /// Description: Executes the EXPECT_FLOAT_EQ operation.
    EXPECT_FLOAT_EQ(spy->sphViscosity, 0.33f);
    std::error_code ec;
    /// Description: Executes the remove operation.
    std::filesystem::remove(configPath, ec);
}
/// Description: Executes the TEST operation.
TEST(QtWorkspaceRuntimeControlsTest, TST_UIX_UI_013_ResetClearsEnergyTimelineHistory)
{
    (void)testsupport::ensureQtApp();
    auto runtime = std::make_unique<RecordingRuntime>();
    /// Description: Executes the window operation.
    grav_qt::MainWindow window(makeRuntimeUiConfig(), "simulation.ini", std::move(runtime));
    window.show();
    /// Description: Executes the processEvents operation.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    QWidget* graphWidget = window.findChild<QWidget*>("energyGraphWidget");
    auto* graph = dynamic_cast<grav_qt::EnergyGraphWidget*>(graphWidget);
    /// Description: Executes the ASSERT_NE operation.
    ASSERT_NE(graph, nullptr);
    SimulationStats first{};
    first.steps = 10u;
    first.dt = 0.01f;
    first.totalTime = 0.10f;
    first.totalEnergy = -5.0f;
    first.energyDriftPct = 0.1f;
    graph->pushSample(first);
    SimulationStats second = first;
    second.steps = 20u;
    second.totalTime = 0.20f;
    second.totalEnergy = -4.0f;
    graph->pushSample(second);
    /// Description: Executes the ASSERT_GE operation.
    ASSERT_GE(graph->sampleCount(), 2u);
    window.findChild<QPushButton*>("resetButton")->click();
    /// Description: Executes the processEvents operation.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(graph->sampleCount(), 0u);
}
/// Description: Executes the TEST operation.
TEST(QtWorkspaceRuntimeControlsTest, TST_UIX_UI_019_GpuTelemetryToggleForwardsToRuntime)
{
    (void)testsupport::ensureQtApp();
    auto runtime = std::make_unique<RecordingRuntime>();
    RecordingRuntime* spy = runtime.get();
    /// Description: Executes the window operation.
    grav_qt::MainWindow window(makeRuntimeUiConfig(), "simulation.ini", std::move(runtime));
    window.show();
    /// Description: Executes the processEvents operation.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    QCheckBox* gpuTelemetryCheck = window.findChild<QCheckBox*>("gpuTelemetryCheck");
    /// Description: Executes the ASSERT_NE operation.
    ASSERT_NE(gpuTelemetryCheck, nullptr);
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(spy->gpuTelemetryEnabled);
    gpuTelemetryCheck->setChecked(true);
    /// Description: Executes the processEvents operation.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(spy->gpuTelemetryEnabled);
    gpuTelemetryCheck->setChecked(false);
    /// Description: Executes the processEvents operation.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(spy->gpuTelemetryEnabled);
}
} // namespace grav_test_qt_workspace_runtime_controls
