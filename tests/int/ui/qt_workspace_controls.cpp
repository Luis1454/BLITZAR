// File: tests/int/ui/qt_workspace_controls.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "client/ClientRuntime.hpp"
#include "tests/support/client_utils.hpp"
#include "tests/support/qt_test_utils.hpp"
#include "ui/MainWindow.hpp"
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDockWidget>
#include <QEventLoop>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <chrono>
#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <string>
namespace grav_test_qt_workspace_controls {
/// Description: Executes the makeWorkspaceUiConfig operation.
static SimulationConfig makeWorkspaceUiConfig()
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
TEST(QtWorkspaceControlsTest, TST_UIX_UI_007_ProfilesAndCheckboxesRemainInteractiveWithoutServer)
{
    (void)testsupport::ensureQtApp();
    auto runtime = std::make_unique<grav_client::ClientRuntime>(
        "simulation.ini", testsupport::makeTransport(1u, std::string()));
    /// Description: Executes the window operation.
    grav_qt::MainWindow window(makeWorkspaceUiConfig(), "simulation.ini", std::move(runtime));
    window.show();
    /// Description: Executes the processEvents operation.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    QComboBox* performanceCombo =
        /// Description: Executes the findComboByObjectName operation.
        testsupport::findComboByObjectName(window, "performanceProfileCombo");
    QComboBox* profileCombo = testsupport::findComboByObjectName(window, "simulationProfileCombo");
    QComboBox* presetCombo = testsupport::findComboByObjectName(window, "scenePresetCombo");
    QCheckBox* sphCheck = testsupport::findCheckBoxByText(window, "SPH");
    QCheckBox* autostartCheck = testsupport::findCheckBoxByText(window, "autostart server");
    /// Description: Executes the ASSERT_NE operation.
    ASSERT_NE(performanceCombo, nullptr);
    /// Description: Executes the ASSERT_NE operation.
    ASSERT_NE(profileCombo, nullptr);
    /// Description: Executes the ASSERT_NE operation.
    ASSERT_NE(presetCombo, nullptr);
    /// Description: Executes the ASSERT_NE operation.
    ASSERT_NE(sphCheck, nullptr);
    /// Description: Executes the ASSERT_NE operation.
    ASSERT_NE(autostartCheck, nullptr);
    /// Description: Executes the EXPECT_GE operation.
    EXPECT_GE(performanceCombo->findText("balanced"), 0);
    /// Description: Executes the EXPECT_GE operation.
    EXPECT_GE(performanceCombo->findText("quality"), 0);
    /// Description: Executes the EXPECT_GE operation.
    EXPECT_GE(profileCombo->findText("galaxy_collision"), 0);
    /// Description: Executes the EXPECT_GE operation.
    EXPECT_GE(profileCombo->findText("binary_star"), 0);
    /// Description: Executes the EXPECT_GE operation.
    EXPECT_GE(profileCombo->findText("solar_system"), 0);
    /// Description: Executes the EXPECT_GE operation.
    EXPECT_GE(presetCombo->findText("galaxy_collision"), 0);
    const bool initialSph = sphCheck->isChecked();
    sphCheck->click();
    /// Description: Executes the processEvents operation.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(sphCheck->isChecked(), initialSph);
    const bool initialAutostart = autostartCheck->isChecked();
    autostartCheck->click();
    /// Description: Executes the processEvents operation.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(autostartCheck->isChecked(), initialAutostart);
}
/// Description: Executes the TEST operation.
TEST(QtWorkspaceControlsTest, TST_UIX_UI_008_SavePersistsProfilesSelectedFromWorkspace)
{
    (void)testsupport::ensureQtApp();
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path configPath =
        std::filesystem::temp_directory_path() /
        ("gravity_qt_profiles_" + std::to_string(stamp) + ".ini");
    SimulationConfig initialConfig = makeWorkspaceUiConfig();
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(initialConfig.save(configPath.string()));
    auto runtime = std::make_unique<grav_client::ClientRuntime>(
        configPath.string(), testsupport::makeTransport(1u, std::string()));
    /// Description: Executes the window operation.
    grav_qt::MainWindow window(initialConfig, configPath.string(), std::move(runtime));
    window.show();
    /// Description: Executes the processEvents operation.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    QComboBox* performanceCombo =
        /// Description: Executes the findComboByObjectName operation.
        testsupport::findComboByObjectName(window, "performanceProfileCombo");
    QComboBox* profileCombo = testsupport::findComboByObjectName(window, "simulationProfileCombo");
    QComboBox* solverCombo = testsupport::findSolverCombo(window);
    QPushButton* saveButton = testsupport::findButtonByText(window, "Save config");
    /// Description: Executes the ASSERT_NE operation.
    ASSERT_NE(performanceCombo, nullptr);
    /// Description: Executes the ASSERT_NE operation.
    ASSERT_NE(profileCombo, nullptr);
    /// Description: Executes the ASSERT_NE operation.
    ASSERT_NE(solverCombo, nullptr);
    /// Description: Executes the ASSERT_NE operation.
    ASSERT_NE(saveButton, nullptr);
    performanceCombo->setCurrentText("balanced");
    profileCombo->setCurrentText("galaxy_collision");
    /// Description: Executes the processEvents operation.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(solverCombo->currentText().toStdString(), "octree_gpu");
    saveButton->click();
    ASSERT_TRUE(testsupport::waitUntilUi(
        [&]() {
            const std::string saved = testsupport::readAllFile(configPath);
            return saved.find("performance(profile=balanced)") != std::string::npos &&
                   saved.find(
                       "scene(style=preset, preset=galaxy_collision, mode=galaxy_collision") !=
                       std::string::npos &&
                   saved.find("simulation(particle_count=40000, dt=0.02, solver=octree_gpu, "
                              "integrator=euler)") != std::string::npos;
        },
        /// Description: Executes the milliseconds operation.
        std::chrono::milliseconds(2000)));
    std::error_code ec;
    /// Description: Executes the remove operation.
    std::filesystem::remove(configPath, ec);
}
/// Description: Executes the TEST operation.
TEST(QtWorkspaceControlsTest, TST_UIX_UI_009_EnergyDockIsVisibleAndSidebarStaysCompact)
{
    (void)testsupport::ensureQtApp();
    auto runtime = std::make_unique<grav_client::ClientRuntime>(
        "simulation.ini", testsupport::makeTransport(1u, std::string()));
    /// Description: Executes the window operation.
    grav_qt::MainWindow window(makeWorkspaceUiConfig(), "simulation.ini", std::move(runtime));
    window.resize(1024, 768);
    window.show();
    /// Description: Executes the processEvents operation.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    QDockWidget* energyDock = window.findChild<QDockWidget*>("energyDock");
    QDockWidget* controlsDock = window.findChild<QDockWidget*>("controlsDock");
    /// Description: Executes the ASSERT_NE operation.
    ASSERT_NE(energyDock, nullptr);
    /// Description: Executes the ASSERT_NE operation.
    ASSERT_NE(controlsDock, nullptr);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(energyDock->isVisible());
    /// Description: Executes the EXPECT_GE operation.
    EXPECT_GE(energyDock->height(), 120);
    /// Description: Executes the EXPECT_LE operation.
    EXPECT_LE(controlsDock->width(), 260);
}
/// Description: Executes the TEST operation.
TEST(QtWorkspaceControlsTest, TST_UIX_UI_015_ThemeTogglePersistsAcrossSaveAndReload)
{
    (void)testsupport::ensureQtApp();
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path configPath = std::filesystem::temp_directory_path() /
                                             ("gravity_qt_theme_" + std::to_string(stamp) + ".ini");
    SimulationConfig initialConfig = makeWorkspaceUiConfig();
    initialConfig.uiTheme = "dark";
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(initialConfig.save(configPath.string()));
    auto runtime = std::make_unique<grav_client::ClientRuntime>(
        configPath.string(), testsupport::makeTransport(1u, std::string()));
    /// Description: Executes the window operation.
    grav_qt::MainWindow window(initialConfig, configPath.string(), std::move(runtime));
    window.show();
    /// Description: Executes the processEvents operation.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    QAction* lightAction = window.findChild<QAction*>("themeLightAction");
    QAction* darkAction = window.findChild<QAction*>("themeDarkAction");
    QPushButton* saveButton = testsupport::findButtonByText(window, "Save config");
    /// Description: Executes the ASSERT_NE operation.
    ASSERT_NE(lightAction, nullptr);
    /// Description: Executes the ASSERT_NE operation.
    ASSERT_NE(darkAction, nullptr);
    /// Description: Executes the ASSERT_NE operation.
    ASSERT_NE(saveButton, nullptr);
    /// Description: Executes the EXPECT_LT operation.
    EXPECT_LT(window.palette().color(QPalette::Window).lightness(), 128);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(darkAction->isChecked());
    lightAction->trigger();
    /// Description: Executes the processEvents operation.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    /// Description: Executes the EXPECT_GT operation.
    EXPECT_GT(window.palette().color(QPalette::Window).lightness(), 180);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(lightAction->isChecked());
    saveButton->click();
    ASSERT_TRUE(testsupport::waitUntilUi(
        [&]() {
            return testsupport::readAllFile(configPath).find("theme=light") != std::string::npos;
        },
        /// Description: Executes the milliseconds operation.
        std::chrono::milliseconds(2000)));
    const SimulationConfig reloaded = SimulationConfig::loadOrCreate(configPath.string());
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(reloaded.uiTheme, "light");
    auto secondRuntime = std::make_unique<grav_client::ClientRuntime>(
        configPath.string(), testsupport::makeTransport(1u, std::string()));
    /// Description: Executes the secondWindow operation.
    grav_qt::MainWindow secondWindow(reloaded, configPath.string(), std::move(secondRuntime));
    secondWindow.show();
    /// Description: Executes the processEvents operation.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    /// Description: Executes the EXPECT_GT operation.
    EXPECT_GT(secondWindow.palette().color(QPalette::Window).lightness(), 180);
    std::error_code ec;
    /// Description: Executes the remove operation.
    std::filesystem::remove(configPath, ec);
}
/// Description: Executes the TEST operation.
TEST(QtWorkspaceControlsTest, TST_UIX_UI_018_ShowsThroughputAdvisoryForHeavyConfig)
{
    (void)testsupport::ensureQtApp();
    SimulationConfig config = makeWorkspaceUiConfig();
    config.particleCount = 100000u;
    config.dt = 0.1f;
    config.substepTargetDt = 0.01f;
    config.maxSubsteps = 6u;
    config.clientParticleCap = 100000u;
    auto runtime = std::make_unique<grav_client::ClientRuntime>(
        "simulation.ini", testsupport::makeTransport(1u, std::string()));
    /// Description: Executes the window operation.
    grav_qt::MainWindow window(config, "simulation.ini", std::move(runtime));
    window.show();
    /// Description: Executes the processEvents operation.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    QLabel* validationLabel = window.findChild<QLabel*>("validationLabel");
    /// Description: Executes the ASSERT_NE operation.
    ASSERT_NE(validationLabel, nullptr);
    const std::string text = validationLabel->text().toStdString();
    EXPECT_NE(text.find("throughput-warning"), std::string::npos) << text;
    EXPECT_NE(text.find("pairwise_cuda"), std::string::npos) << text;
    EXPECT_NE(text.find("octree_gpu"), std::string::npos) << text;
    EXPECT_NE(text.find("draw cap"), std::string::npos) << text;
}
} // namespace grav_test_qt_workspace_controls
