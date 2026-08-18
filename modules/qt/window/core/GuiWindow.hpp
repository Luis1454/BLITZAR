/*
 * @file modules/qt/window/core/GuiWindow.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#ifndef BLITZAR_MODULES_QT_SRC_WINDOW_CORE_WINDOW_HPP_
#define BLITZAR_MODULES_QT_SRC_WINDOW_CORE_WINDOW_HPP_
/*
 * Module: qt
 * Responsibility: Define the top-level Qt workspace and its persistent widget
 * state.
 */
#include "client/runtime/CliInterface.hpp"
#include "config/core/configuration/CfgConfig.hpp"
#include "config/validation/scenario/CfgScenario.hpp"
#include "window/control/GuiController.hpp"
#include "window/core/GuiWidgets.hpp"
#include "window/presentation/GuiPresenter.hpp"
#include "support/performance/GuiThroughput.hpp"
#include "support/storage/GuiLayoutStore.hpp"
#include <QByteArray>
#include <QMainWindow>
#include <QPointer>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

class QString;
class QMenu;
class QTabWidget;
class QWidget;

namespace bltzr_qt {
class Window : public QMainWindow {
public:
    Window(SimulationConfig config, std::string configPath,
               std::unique_ptr<bltzr_client::Interface> runtime);
    ~Window() override;

private:
    static std::string formatFromSelectedFilter(const QString& filter);
    bool applyConfigToServer(bool requestReset, bool captureUi = true);
    void applyConnectorSettings(bool reconnectNow);
    void applyConfigToUi();
    void applyViewSettings();
    void applyTheme();
    void captureUiIntoConfig();
    void applyPerformanceProfileToRuntime();
    void buildMenus();
    void buildFileMenu(QMenu* menu);
    void buildEditMenu(QMenu* menu);
    void buildViewMenu(QMenu* menu);
    void buildSimulationMenu(QMenu* menu);
    void buildWindowMenu(QMenu* menu);
    void buildHelpMenu(QMenu* menu);
    void buildThemeMenu(QMenu* menu);
    void connectOverlayControls();
    void connectTelemetryControls();
    void buildWorkspaceDocks(QPointer<QTabWidget> sidebarTabs, QPointer<QWidget> summaryPane,
                             QPointer<QWidget> validationPane);
    QPointer<QWidget> buildTelemetryPane();
    QPointer<QTabWidget> buildSidebarTabs();
    QPointer<QWidget> buildValidationPane();
    void configureRemoteConnectorFromUi();
    void connectControls();
    void handleExportRequest();
    void handleSaveCheckpointRequest();
    void handleLoadCheckpointRequest();
    void handleLoadInputRequest();
    void handleLoadPresetRequest();
    void editLoadedConfiguration();
    bool applyEditedConfiguration(const SimulationConfig& candidate);
    void initializeControlState();
    // Helpers to keep initialization concise and testable
    void initializeComboBoxes();
    void initializeObjectNames();
    void initializeSpinAndSliderValues();
    void initializeLabelsAndTooltips();
    void markConfigDirty(bool dirty = true);
    bool refreshValidationReport(bool blockOnErrors);
    void requestReconnectFromUi();
    void resetSimulationFromUi();
    void restoreDefaultWorkspace();
    void saveWorkspacePreset();
    void loadWorkspacePreset();
    void deleteWorkspacePreset();
    QString buildValidationText(const bltzr_config::ScenarioValidationReport& report,
                                const Advisory& advisory) const;
    bool saveConfigToDisk();
    void showThroughputAdvisory(const Advisory& advisory);
    void update3DCameraFromSliders();
    void tick();
    SimulationConfig _config;
    std::string _configPath;
    std::unique_ptr<bltzr_client::Interface> _runtime;
    QPointer<class ConfigurationEditor> _configurationEditor;
    QPointer<class SceneEditor> _sceneEditor;
    Widgets _widgets;
    Controller _controller;
    Presenter _presenter;
    LayoutStore _workspaceLayouts;
    QByteArray _defaultWorkspaceGeometry;
    QByteArray _defaultWorkspaceState;
    std::uint64_t _lastEnergyStep;
    std::uint32_t _clientDrawCap;
    float _uiTickFps;
    bool _configDirty;
    std::chrono::steady_clock::time_point _lastUiTickAt;
};
} // namespace bltzr_qt
#endif // BLITZAR_MODULES_QT_SRC_WINDOW_CORE_WINDOW_HPP_
