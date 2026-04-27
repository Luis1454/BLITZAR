/*
 * @file tests/int/ui/qt_workspace_runtime_controls.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

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
class RecordingRuntime final : public grav_client::IClientRuntime {
public:
    bool start() override
    {
        return false;
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
        stepCount += 1;
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
        resetCount += 1;
    }

    void requestRecover() override
    {
        recoverCount += 1;
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

    void setSphParameters(float h, float rho, float gasK, float visc) override
    {
        sphSmoothingLength = h;
        sphRestDensity = rho;
        sphGasConstant = gasK;
        sphViscosity = visc;
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

    void requestSaveCheckpoint(const std::string&) override
    {
    }

    void requestLoadCheckpoint(const std::string&) override
    {
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

    bool isRemoteMode() const override
    {
        return true;
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
        return "external";
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

TEST(QtWorkspaceRuntimeControlsTest, TST_UIX_UI_010_RunButtonsAndConnectorForwardToRuntime)
{
    (void)testsupport::ensureQtApp();
    auto runtime = std::make_unique<RecordingRuntime>();
    RecordingRuntime* spy = runtime.get();
    grav_qt::MainWindow window(makeRuntimeUiConfig(), "simulation.ini", std::move(runtime));
    window.show();
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
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    EXPECT_EQ(spy->connectorHost, "10.0.0.4");
    EXPECT_EQ(spy->connectorPort, 4567u);
    EXPECT_TRUE(spy->connectorAutoStart);
    EXPECT_EQ(spy->connectorServerExecutable, "C:/bin/blitzar-server.exe");
    EXPECT_EQ(spy->reconnectCount, 1u);
    EXPECT_TRUE(spy->pausedState);
    EXPECT_EQ(window.findChild<QPushButton*>("pauseToggleButton")->text().toStdString(), "Resume");
    EXPECT_EQ(spy->stepCount, 1u);
    EXPECT_GE(spy->resetCount, 1u);
    EXPECT_EQ(spy->recoverCount, 1u);
}

TEST(QtWorkspaceRuntimeControlsTest, TST_UIX_UI_011_RunAndSceneProfilesPropagateToRuntimeAndUi)
{
    (void)testsupport::ensureQtApp();
    auto runtime = std::make_unique<RecordingRuntime>();
    RecordingRuntime* spy = runtime.get();
    grav_qt::MainWindow window(makeRuntimeUiConfig(), "simulation.ini", std::move(runtime));
    window.show();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    window.findChild<QComboBox*>("performanceProfileCombo")->setCurrentText("balanced");
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    EXPECT_EQ(spy->performanceProfile, "balanced");
    EXPECT_EQ(spy->snapshotPublishPeriodMs, 33u);
    EXPECT_EQ(spy->energyEverySteps, 20u);
    EXPECT_EQ(spy->energySampleLimit, 1024u);
    EXPECT_FLOAT_EQ(spy->substepTargetDt, 0.005f);
    EXPECT_EQ(spy->configuredMaxSubsteps, 8u);
    EXPECT_EQ(spy->remoteSnapshotCap, 8192u);
    window.findChild<QComboBox*>("simulationProfileCombo")->setCurrentText("binary_star");
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    EXPECT_EQ(window.findChild<QComboBox*>("solverCombo")->currentText().toStdString(),
              "pairwise_cuda");
    EXPECT_EQ(window.findChild<QComboBox*>("integratorCombo")->currentText().toStdString(), "rk4");
    EXPECT_EQ(window.findChild<QComboBox*>("scenePresetCombo")->currentText().toStdString(),
              "two_body");
    EXPECT_NEAR(window.findChild<QDoubleSpinBox*>("dtSpin")->value(), 0.001, 1e-6);
    EXPECT_EQ(spy->initialStateConfig.mode, "two_body");
    EXPECT_GE(spy->resetCount, 1u);
}

TEST(QtWorkspaceRuntimeControlsTest, TST_UIX_UI_012_PhysicsAndRenderControlsPersistWorkspaceChoices)
{
    (void)testsupport::ensureQtApp();
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path configPath =
        std::filesystem::temp_directory_path() /
        ("gravity_qt_workspace_runtime_" + std::to_string(stamp) + ".ini");
    ASSERT_TRUE(makeRuntimeUiConfig().save(configPath.string()));
    auto runtime = std::make_unique<RecordingRuntime>();
    RecordingRuntime* spy = runtime.get();
    grav_qt::MainWindow window(makeRuntimeUiConfig(), configPath.string(), std::move(runtime));
    window.show();
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
        std::chrono::milliseconds(2000)));
    EXPECT_EQ(spy->solverMode, "octree_cpu");
    EXPECT_EQ(spy->integratorMode, "euler");
    EXPECT_FLOAT_EQ(spy->configuredDt, 0.25f);
    EXPECT_FLOAT_EQ(spy->octreeTheta, 0.65f);
    EXPECT_FLOAT_EQ(spy->octreeSoftening, 0.1234f);
    EXPECT_TRUE(spy->sphEnabled);
    EXPECT_FLOAT_EQ(spy->sphSmoothingLength, 0.8f);
    EXPECT_FLOAT_EQ(spy->sphRestDensity, 2.5f);
    EXPECT_FLOAT_EQ(spy->sphGasConstant, 5.5f);
    EXPECT_FLOAT_EQ(spy->sphViscosity, 0.33f);
    std::error_code ec;
    std::filesystem::remove(configPath, ec);
}

TEST(QtWorkspaceRuntimeControlsTest, TST_UIX_UI_013_ResetClearsEnergyTimelineHistory)
{
    (void)testsupport::ensureQtApp();
    auto runtime = std::make_unique<RecordingRuntime>();
    grav_qt::MainWindow window(makeRuntimeUiConfig(), "simulation.ini", std::move(runtime));
    window.show();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    QWidget* graphWidget = window.findChild<QWidget*>("energyGraphWidget");
    auto* graph = dynamic_cast<grav_qt::EnergyGraphWidget*>(graphWidget);
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
    ASSERT_GE(graph->sampleCount(), 2u);
    window.findChild<QPushButton*>("resetButton")->click();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    EXPECT_EQ(graph->sampleCount(), 0u);
}

TEST(QtWorkspaceRuntimeControlsTest, TST_UIX_UI_019_GpuTelemetryToggleForwardsToRuntime)
{
    (void)testsupport::ensureQtApp();
    auto runtime = std::make_unique<RecordingRuntime>();
    RecordingRuntime* spy = runtime.get();
    grav_qt::MainWindow window(makeRuntimeUiConfig(), "simulation.ini", std::move(runtime));
    window.show();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    QCheckBox* gpuTelemetryCheck = window.findChild<QCheckBox*>("gpuTelemetryCheck");
    ASSERT_NE(gpuTelemetryCheck, nullptr);
    EXPECT_FALSE(spy->gpuTelemetryEnabled);
    gpuTelemetryCheck->setChecked(true);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    EXPECT_TRUE(spy->gpuTelemetryEnabled);
    gpuTelemetryCheck->setChecked(false);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    EXPECT_FALSE(spy->gpuTelemetryEnabled);
}
} // namespace grav_test_qt_workspace_runtime_controls
