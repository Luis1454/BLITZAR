#include "frontend/FrontendRuntime.hpp"
#include "tests/support/backend_harness.hpp"
#include "tests/support/frontend_utils.hpp"
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

    RealBackendHarness backend;
    std::string startError;
    ASSERT_TRUE(backend.start(startError)) << startError;

    auto runtime = std::make_unique<grav_frontend::FrontendRuntime>(
        "simulation.ini",
        testsupport::makeTransport(backend.port(), backend.executablePath()));
    grav_qt::MainWindow window(makeUiConfig(), "simulation.ini", std::move(runtime));

    ASSERT_TRUE(testsupport::waitUntilUi([&]() {
        const QString status = testsupport::findStatusLabelText(window);
        return status.contains("link=connected") && status.contains("owner=external");
    }, std::chrono::milliseconds(5000)));

    backend.stop();
}

TEST(QtMainWindowTest, TST_UIX_UI_002_ShowsReconnectingWhenBackendStopsAndRecovers)
{
    (void)testsupport::ensureQtApp();

    RealBackendHarness backend;
    std::string startError;
    ASSERT_TRUE(backend.start(startError)) << startError;
    const std::uint16_t fixedPort = backend.port();

    auto runtime = std::make_unique<grav_frontend::FrontendRuntime>(
        "simulation.ini",
        testsupport::makeTransport(fixedPort, backend.executablePath()));
    grav_qt::MainWindow window(makeUiConfig(), "simulation.ini", std::move(runtime));

    ASSERT_TRUE(testsupport::waitUntilUi([&]() {
        return testsupport::findStatusLabelText(window).contains("link=connected");
    }, std::chrono::milliseconds(5000)));

    backend.stop();
    ASSERT_TRUE(testsupport::waitUntilUi([&]() {
        return testsupport::findStatusLabelText(window).contains("link=reconnecting");
    }, std::chrono::milliseconds(5000)));

    ASSERT_TRUE(backend.start(startError, fixedPort)) << startError;
    ASSERT_TRUE(testsupport::waitUntilUi([&]() {
        return testsupport::findStatusLabelText(window).contains("link=connected");
    }, std::chrono::milliseconds(7000)));

    backend.stop();
}

TEST(QtMainWindowTest, TST_UIX_UI_003_SavesConfigOnlyOnExplicitSaveAction)
{
    (void)testsupport::ensureQtApp();

    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path configPath =
        std::filesystem::temp_directory_path() / ("gravity_qt_explicit_save_" + std::to_string(stamp) + ".ini");
    SimulationConfig initialConfig = makeUiConfig();
    ASSERT_TRUE(initialConfig.save(configPath.string()));

    RealBackendHarness backend;
    std::string startError;
    ASSERT_TRUE(backend.start(startError)) << startError;

    auto runtime = std::make_unique<grav_frontend::FrontendRuntime>(
        configPath.string(),
        testsupport::makeTransport(backend.port(), backend.executablePath()));
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

    backend.stop();
    std::error_code ec;
    std::filesystem::remove(configPath, ec);
}

} // namespace grav_test_qt_window
