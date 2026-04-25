#include "client/ClientRuntime.hpp"
#include "protocol/ServerProtocol.hpp"
#include "tests/support/client_utils.hpp"
#include "tests/support/qt_test_utils.hpp"
#include "tests/support/server_harness.hpp"
#include "ui/EnergyGraphWidget.hpp"
#include "ui/MainWindow.hpp"
#include <QComboBox>
#include <QCoreApplication>
#include <QEventLoop>
#include <QPushButton>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <string>
namespace grav_test_qt_window {
SimulationConfig makeUiConfig()
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
TEST(QtMainWindowTest, TST_UIX_UI_001_ConstructsAndTicksWithRealRuntime)
{
    (void)testsupport::ensureQtApp();
    RealServerHarness server;
    std::string startError;
    ASSERT_TRUE(server.start(startError)) << startError;
    auto runtime = std::make_unique<grav_client::ClientRuntime>(
        "simulation.ini", testsupport::makeTransport(server.port(), server.executablePath()));
    grav_qt::MainWindow window(makeUiConfig(), "simulation.ini", std::move(runtime));
    ASSERT_TRUE(testsupport::waitUntilUi(
        [&]() {
            const QString status = testsupport::findStatusLabelText(window);
            return status.contains("Link: connected") && status.contains("Owner: external");
        },
        std::chrono::milliseconds(5000)));
    server.stop();
}
TEST(QtMainWindowTest, TST_UIX_UI_002_ShowsReconnectingWhenServerStopsAndRecovers)
{
    (void)testsupport::ensureQtApp();
    RealServerHarness server;
    std::string startError;
    ASSERT_TRUE(server.start(startError)) << startError;
    const std::uint16_t fixedPort = server.port();
    auto runtime = std::make_unique<grav_client::ClientRuntime>(
        "simulation.ini", testsupport::makeTransport(fixedPort, server.executablePath()));
    grav_qt::MainWindow window(makeUiConfig(), "simulation.ini", std::move(runtime));
    ASSERT_TRUE(testsupport::waitUntilUi(
        [&]() { return testsupport::findStatusLabelText(window).contains("Link: connected"); },
        std::chrono::milliseconds(5000)));
    server.stop();
    ASSERT_TRUE(testsupport::waitUntilUi(
        [&]() { return testsupport::findStatusLabelText(window).contains("Link: reconnecting"); },
        std::chrono::milliseconds(5000)));
    ASSERT_TRUE(server.start(startError, fixedPort)) << startError;
    ASSERT_TRUE(testsupport::waitUntilUi(
        [&]() { return testsupport::findStatusLabelText(window).contains("Link: connected"); },
        std::chrono::milliseconds(7000)));
    server.stop();
}
TEST(QtMainWindowTest, TST_UIX_UI_003_SavesConfigOnlyOnExplicitSaveAction)
{
    (void)testsupport::ensureQtApp();
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path configPath =
        std::filesystem::temp_directory_path() /
        ("gravity_qt_explicit_save_" + std::to_string(stamp) + ".ini");
    SimulationConfig initialConfig = makeUiConfig();
    ASSERT_TRUE(initialConfig.save(configPath.string()));
    RealServerHarness server;
    std::string startError;
    ASSERT_TRUE(server.start(startError)) << startError;
    auto runtime = std::make_unique<grav_client::ClientRuntime>(
        configPath.string(), testsupport::makeTransport(server.port(), server.executablePath()));
    grav_qt::MainWindow window(initialConfig, configPath.string(), std::move(runtime));
    ASSERT_TRUE(testsupport::waitUntilUi(
        [&]() { return testsupport::findStatusLabelText(window).contains("Link: connected"); },
        std::chrono::milliseconds(5000)));
    QComboBox* solverCombo = testsupport::findSolverCombo(window);
    ASSERT_NE(solverCombo, nullptr);
    solverCombo->setCurrentText("octree_gpu");
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    const std::string fileBeforeSave = testsupport::readAllFile(configPath);
    EXPECT_NE(
        fileBeforeSave.find(
            "simulation(particle_count=10000, dt=0.01, solver=pairwise_cuda, integrator=euler)"),
        std::string::npos);
    QPushButton* saveButton = testsupport::findButtonByText(window, "Save config");
    ASSERT_NE(saveButton, nullptr);
    saveButton->click();
    ASSERT_TRUE(testsupport::waitUntilUi(
        [&]() {
            return testsupport::readAllFile(configPath)
                       .find("simulation(particle_count=10000, dt=0.01, solver=octree_gpu, "
                             "integrator=euler)") != std::string::npos;
        },
        std::chrono::milliseconds(2000)));
    server.stop();
    std::error_code ec;
    std::filesystem::remove(configPath, ec);
}
TEST(QtMainWindowTest, TST_UIX_UI_004_ShowsEffectiveClientCapWhenConfiguredCapExceedsProtocolMax)
{
    (void)testsupport::ensureQtApp();
    SimulationConfig config = makeUiConfig();
    config.clientParticleCap = grav_protocol::kSnapshotMaxPoints + 5000u;
    RealServerHarness server;
    std::string startError;
    ASSERT_TRUE(server.start(startError)) << startError;
    auto runtime = std::make_unique<grav_client::ClientRuntime>(
        "simulation.ini", testsupport::makeTransport(server.port(), server.executablePath()));
    grav_qt::MainWindow window(config, "simulation.ini", std::move(runtime));
    ASSERT_TRUE(testsupport::waitUntilUi(
        [&]() {
            const QString status = testsupport::findStatusLabelText(window);
            return status.contains("Link: connected") &&
                   status.contains(QString("/ %1").arg(grav_protocol::kSnapshotMaxPoints)) &&
                   status.contains("Queue: ") && status.contains("Policy: latest-only");
        },
        std::chrono::milliseconds(5000)));
    server.stop();
}
TEST(QtMainWindowTest, TST_UIX_UI_014_RealRuntimeProgressesAndSurfacesFreshOrStaleViewportState)
{
    (void)testsupport::ensureQtApp();
    RealServerHarness server;
    std::string startError;
    ASSERT_TRUE(server.start(startError)) << startError;
    auto runtime = std::make_unique<grav_client::ClientRuntime>(
        "simulation.ini", testsupport::makeTransport(server.port(), server.executablePath()));
    grav_qt::MainWindow window(makeUiConfig(), "simulation.ini", std::move(runtime));
    window.show();
    ASSERT_TRUE(testsupport::waitUntilUi(
        [&]() { return testsupport::findStatusLabelText(window).contains("Link: connected"); },
        std::chrono::milliseconds(5000)));
    QWidget* graphWidget = window.findChild<QWidget*>("energyGraphWidget");
    auto* graph = dynamic_cast<grav_qt::EnergyGraphWidget*>(graphWidget);
    ASSERT_NE(graph, nullptr);
    const std::uint64_t initialSteps = testsupport::findSummaryUnsignedMetric(window, "Steps: ");
    const std::size_t initialSamples = graph->sampleCount();
    QPushButton* pauseButton = window.findChild<QPushButton*>("pauseToggleButton");
    ASSERT_NE(pauseButton, nullptr);
    if (pauseButton->text() == "Resume" ||
        testsupport::findStatusLabelText(window).contains("State: PAUSED")) {
        pauseButton->click();
    }
    const bool progressed = testsupport::waitUntilUi(
        [&]() {
            const QString status = testsupport::findStatusLabelText(window);
            return status.contains("Link: connected") && status.contains("State: RUNNING") &&
                   status.contains("Backend: busy") && status.contains("Viewport: fresh") &&
                   !status.contains("Backend: stalled") &&
                   testsupport::findSummaryUnsignedMetric(window, "Steps: ") > initialSteps &&
                   graph->sampleCount() > initialSamples;
        },
        std::chrono::milliseconds(7000));
    if (!progressed) {
        const std::filesystem::path evidence =
            testsupport::saveFailureEvidence(window, "gravity_qt_progression_failure");
        ADD_FAILURE() << "runtime did not show real progression; evidence at " << evidence.string();
    }
    ASSERT_TRUE(progressed);
    server.stop();
    const bool staleAfterStop = testsupport::waitUntilUi(
        [&]() {
            const QString status = testsupport::findStatusLabelText(window);
            return status.contains("Link: reconnecting") &&
                   (status.contains("Viewport: stale") ||
                    status.contains("Viewport: awaiting snapshot"));
        },
        std::chrono::milliseconds(7000));
    if (!staleAfterStop) {
        const std::filesystem::path evidence =
            testsupport::saveFailureEvidence(window, "gravity_qt_stale_failure");
        ADD_FAILURE() << "viewport did not surface stale/reconnecting state; evidence at "
                      << evidence.string();
    }
    EXPECT_TRUE(staleAfterStop);
}
} // namespace grav_test_qt_window
