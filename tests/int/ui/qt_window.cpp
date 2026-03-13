#include "client/ClientRuntime.hpp"
#include "protocol/ServerProtocol.hpp"
#include "tests/support/server_harness.hpp"
#include "tests/support/client_utils.hpp"
#include "tests/support/qt_test_utils.hpp"
#include "ui/MainWindow.hpp"

#include <gtest/gtest.h>

#include <QComboBox>
#include <QCoreApplication>
#include <QEventLoop>
#include <QPushButton>

#include <chrono>
#include <cstdint>
#include <filesystem>
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
        "simulation.ini",
        testsupport::makeTransport(server.port(), server.executablePath()));
    grav_qt::MainWindow window(makeUiConfig(), "simulation.ini", std::move(runtime));

    ASSERT_TRUE(testsupport::waitUntilUi([&]() {
        const QString status = testsupport::findStatusLabelText(window);
        return status.contains("link=connected") && status.contains("owner=external");
    }, std::chrono::milliseconds(5000)));

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
        "simulation.ini",
        testsupport::makeTransport(fixedPort, server.executablePath()));
    grav_qt::MainWindow window(makeUiConfig(), "simulation.ini", std::move(runtime));

    ASSERT_TRUE(testsupport::waitUntilUi([&]() {
        return testsupport::findStatusLabelText(window).contains("link=connected");
    }, std::chrono::milliseconds(5000)));

    server.stop();
    ASSERT_TRUE(testsupport::waitUntilUi([&]() {
        return testsupport::findStatusLabelText(window).contains("link=reconnecting");
    }, std::chrono::milliseconds(5000)));

    ASSERT_TRUE(server.start(startError, fixedPort)) << startError;
    ASSERT_TRUE(testsupport::waitUntilUi([&]() {
        return testsupport::findStatusLabelText(window).contains("link=connected");
    }, std::chrono::milliseconds(7000)));

    server.stop();
}

TEST(QtMainWindowTest, TST_UIX_UI_003_SavesConfigOnlyOnExplicitSaveAction)
{
    (void)testsupport::ensureQtApp();

    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path configPath =
        std::filesystem::temp_directory_path() / ("gravity_qt_explicit_save_" + std::to_string(stamp) + ".ini");
    SimulationConfig initialConfig = makeUiConfig();
    ASSERT_TRUE(initialConfig.save(configPath.string()));

    RealServerHarness server;
    std::string startError;
    ASSERT_TRUE(server.start(startError)) << startError;

    auto runtime = std::make_unique<grav_client::ClientRuntime>(
        configPath.string(),
        testsupport::makeTransport(server.port(), server.executablePath()));
    grav_qt::MainWindow window(initialConfig, configPath.string(), std::move(runtime));

    ASSERT_TRUE(testsupport::waitUntilUi([&]() {
        return testsupport::findStatusLabelText(window).contains("link=connected");
    }, std::chrono::milliseconds(5000)));

    QComboBox *solverCombo = testsupport::findSolverCombo(window);
    ASSERT_NE(solverCombo, nullptr);
    solverCombo->setCurrentText("octree_gpu");
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);

    const std::string fileBeforeSave = testsupport::readAllFile(configPath);
    EXPECT_NE(fileBeforeSave.find("solver=pairwise_cuda"), std::string::npos);

    QPushButton *saveButton = testsupport::findButtonByText(window, "Save config");
    ASSERT_NE(saveButton, nullptr);
    saveButton->click();

    ASSERT_TRUE(testsupport::waitUntilUi([&]() {
        return testsupport::readAllFile(configPath).find("solver=octree_gpu") != std::string::npos;
    }, std::chrono::milliseconds(2000)));

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
        "simulation.ini",
        testsupport::makeTransport(server.port(), server.executablePath()));
    grav_qt::MainWindow window(config, "simulation.ini", std::move(runtime));

    ASSERT_TRUE(testsupport::waitUntilUi([&]() {
        const QString status = testsupport::findStatusLabelText(window);
        return status.contains("link=connected")
            && status.contains(QString("cap=%1").arg(grav_protocol::kSnapshotMaxPoints));
    }, std::chrono::milliseconds(5000)));

    server.stop();
}

} // namespace grav_test_qt_window
