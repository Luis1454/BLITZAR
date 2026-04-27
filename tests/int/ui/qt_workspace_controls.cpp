/*
 * @file tests/int/ui/qt_workspace_controls.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

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

TEST(QtWorkspaceControlsTest, TST_UIX_UI_007_ProfilesAndCheckboxesRemainInteractiveWithoutServer)
{
    (void)testsupport::ensureQtApp();
    auto runtime = std::make_unique<grav_client::ClientRuntime>(
        "simulation.ini", testsupport::makeTransport(1u, std::string()));
    grav_qt::MainWindow window(makeWorkspaceUiConfig(), "simulation.ini", std::move(runtime));
    window.show();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    QComboBox* performanceCombo =
        testsupport::findComboByObjectName(window, "performanceProfileCombo");
    QComboBox* profileCombo = testsupport::findComboByObjectName(window, "simulationProfileCombo");
    QComboBox* presetCombo = testsupport::findComboByObjectName(window, "scenePresetCombo");
    QCheckBox* sphCheck = testsupport::findCheckBoxByText(window, "SPH");
    QCheckBox* autostartCheck = testsupport::findCheckBoxByText(window, "autostart server");
    ASSERT_NE(performanceCombo, nullptr);
    ASSERT_NE(profileCombo, nullptr);
    ASSERT_NE(presetCombo, nullptr);
    ASSERT_NE(sphCheck, nullptr);
    ASSERT_NE(autostartCheck, nullptr);
    EXPECT_GE(performanceCombo->findText("balanced"), 0);
    EXPECT_GE(performanceCombo->findText("quality"), 0);
    EXPECT_GE(profileCombo->findText("galaxy_collision"), 0);
    EXPECT_GE(profileCombo->findText("binary_star"), 0);
    EXPECT_GE(profileCombo->findText("solar_system"), 0);
    EXPECT_GE(presetCombo->findText("galaxy_collision"), 0);
    const bool initialSph = sphCheck->isChecked();
    sphCheck->click();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    EXPECT_NE(sphCheck->isChecked(), initialSph);
    const bool initialAutostart = autostartCheck->isChecked();
    autostartCheck->click();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    EXPECT_NE(autostartCheck->isChecked(), initialAutostart);
}

TEST(QtWorkspaceControlsTest, TST_UIX_UI_008_SavePersistsProfilesSelectedFromWorkspace)
{
    (void)testsupport::ensureQtApp();
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path configPath =
        std::filesystem::temp_directory_path() /
        ("gravity_qt_profiles_" + std::to_string(stamp) + ".ini");
    SimulationConfig initialConfig = makeWorkspaceUiConfig();
    ASSERT_TRUE(initialConfig.save(configPath.string()));
    auto runtime = std::make_unique<grav_client::ClientRuntime>(
        configPath.string(), testsupport::makeTransport(1u, std::string()));
    grav_qt::MainWindow window(initialConfig, configPath.string(), std::move(runtime));
    window.show();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    QComboBox* performanceCombo =
        testsupport::findComboByObjectName(window, "performanceProfileCombo");
    QComboBox* profileCombo = testsupport::findComboByObjectName(window, "simulationProfileCombo");
    QComboBox* solverCombo = testsupport::findSolverCombo(window);
    QPushButton* saveButton = testsupport::findButtonByText(window, "Save config");
    ASSERT_NE(performanceCombo, nullptr);
    ASSERT_NE(profileCombo, nullptr);
    ASSERT_NE(solverCombo, nullptr);
    ASSERT_NE(saveButton, nullptr);
    performanceCombo->setCurrentText("balanced");
    profileCombo->setCurrentText("galaxy_collision");
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
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
        std::chrono::milliseconds(2000)));
    std::error_code ec;
    std::filesystem::remove(configPath, ec);
}

TEST(QtWorkspaceControlsTest, TST_UIX_UI_009_EnergyDockIsVisibleAndSidebarStaysCompact)
{
    (void)testsupport::ensureQtApp();
    auto runtime = std::make_unique<grav_client::ClientRuntime>(
        "simulation.ini", testsupport::makeTransport(1u, std::string()));
    grav_qt::MainWindow window(makeWorkspaceUiConfig(), "simulation.ini", std::move(runtime));
    window.resize(1024, 768);
    window.show();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    QDockWidget* energyDock = window.findChild<QDockWidget*>("energyDock");
    QDockWidget* controlsDock = window.findChild<QDockWidget*>("controlsDock");
    ASSERT_NE(energyDock, nullptr);
    ASSERT_NE(controlsDock, nullptr);
    EXPECT_TRUE(energyDock->isVisible());
    EXPECT_GE(energyDock->height(), 120);
    EXPECT_LE(controlsDock->width(), 260);
}

TEST(QtWorkspaceControlsTest, TST_UIX_UI_015_ThemeTogglePersistsAcrossSaveAndReload)
{
    (void)testsupport::ensureQtApp();
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path configPath = std::filesystem::temp_directory_path() /
                                             ("gravity_qt_theme_" + std::to_string(stamp) + ".ini");
    SimulationConfig initialConfig = makeWorkspaceUiConfig();
    initialConfig.uiTheme = "dark";
    ASSERT_TRUE(initialConfig.save(configPath.string()));
    auto runtime = std::make_unique<grav_client::ClientRuntime>(
        configPath.string(), testsupport::makeTransport(1u, std::string()));
    grav_qt::MainWindow window(initialConfig, configPath.string(), std::move(runtime));
    window.show();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    QAction* lightAction = window.findChild<QAction*>("themeLightAction");
    QAction* darkAction = window.findChild<QAction*>("themeDarkAction");
    QPushButton* saveButton = testsupport::findButtonByText(window, "Save config");
    ASSERT_NE(lightAction, nullptr);
    ASSERT_NE(darkAction, nullptr);
    ASSERT_NE(saveButton, nullptr);
    EXPECT_LT(window.palette().color(QPalette::Window).lightness(), 128);
    EXPECT_TRUE(darkAction->isChecked());
    lightAction->trigger();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    EXPECT_GT(window.palette().color(QPalette::Window).lightness(), 180);
    EXPECT_TRUE(lightAction->isChecked());
    saveButton->click();
    ASSERT_TRUE(testsupport::waitUntilUi(
        [&]() {
            return testsupport::readAllFile(configPath).find("theme=light") != std::string::npos;
        },
        std::chrono::milliseconds(2000)));
    const SimulationConfig reloaded = SimulationConfig::loadOrCreate(configPath.string());
    EXPECT_EQ(reloaded.uiTheme, "light");
    auto secondRuntime = std::make_unique<grav_client::ClientRuntime>(
        configPath.string(), testsupport::makeTransport(1u, std::string()));
    grav_qt::MainWindow secondWindow(reloaded, configPath.string(), std::move(secondRuntime));
    secondWindow.show();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    EXPECT_GT(secondWindow.palette().color(QPalette::Window).lightness(), 180);
    std::error_code ec;
    std::filesystem::remove(configPath, ec);
}

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
    grav_qt::MainWindow window(config, "simulation.ini", std::move(runtime));
    window.show();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    QLabel* validationLabel = window.findChild<QLabel*>("validationLabel");
    ASSERT_NE(validationLabel, nullptr);
    const std::string text = validationLabel->text().toStdString();
    EXPECT_NE(text.find("throughput-warning"), std::string::npos) << text;
    EXPECT_NE(text.find("pairwise_cuda"), std::string::npos) << text;
    EXPECT_NE(text.find("octree_gpu"), std::string::npos) << text;
    EXPECT_NE(text.find("draw cap"), std::string::npos) << text;
}
} // namespace grav_test_qt_workspace_controls
