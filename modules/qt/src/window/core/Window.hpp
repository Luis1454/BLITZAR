/*
 * @file modules/qt/src/window/core/Window.hpp
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
#include "client/runtime/Interface.hpp"
#include "config/core/Config.hpp"
#include "config/validation/Scenario.hpp"
#include "window/control/Controller.hpp"
#include "window/core/Widgets.hpp"
#include "window/presentation/Presenter.hpp"
#include "support/performance/Throughput.hpp"
#include "support/storage/LayoutStore.hpp"
#include <QByteArray>
#include <QMainWindow>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

class QString;
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
    bool applyConfigToServer(bool requestReset);
    void applyConnectorSettings(bool reconnectNow);
    void applyConfigToUi();
    void applyViewSettings();
    void applyTheme();
    void captureUiIntoConfig();
    void applyPerformanceProfileToRuntime();
    void buildMenus();
    void buildWorkspaceDocks(QTabWidget* sidebarTabs, QWidget* summaryPane,
                             QWidget* validationPane);
    QWidget* buildTelemetryPane();
    QTabWidget* buildSidebarTabs();
    QWidget* buildValidationPane();
    void configureRemoteConnectorFromUi();
    void connectControls();
    void handleExportRequest();
    void handleSaveCheckpointRequest();
    void handleLoadCheckpointRequest();
    void handleLoadInputRequest();
    void handleLoadPresetRequest();
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
