// File: modules/qt/include/ui/MainWindow.hpp
// Purpose: Client module implementation for BLITZAR extension workflows.

#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_MAINWINDOW_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_MAINWINDOW_HPP_
/*
 * Module: ui
 * Responsibility: Define the top-level Qt workspace and its persistent widget
 * state.
 */
#include "client/IClientRuntime.hpp"
#include "config/SimulationConfig.hpp"
#include "config/SimulationScenarioValidation.hpp"
#include "ui/MainWindowController.hpp"
#include "ui/MainWindowPresenter.hpp"
#include "ui/ThroughputAdvisor.hpp"
#include "ui/WorkspaceLayoutStore.hpp"
#include <QByteArray>
#include <QMainWindow>
#include <QPointer>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
/// Description: Defines the QCheckBox data or behavior contract.
class QCheckBox;
/// Description: Defines the QComboBox data or behavior contract.
class QComboBox;
/// Description: Defines the QDockWidget data or behavior contract.
class QDockWidget;
/// Description: Defines the QDoubleSpinBox data or behavior contract.
class QDoubleSpinBox;
/// Description: Defines the QLabel data or behavior contract.
class QLabel;
/// Description: Defines the QLineEdit data or behavior contract.
class QLineEdit;
/// Description: Defines the QAction data or behavior contract.
class QAction;
/// Description: Defines the QPushButton data or behavior contract.
class QPushButton;
/// Description: Defines the QSlider data or behavior contract.
class QSlider;
/// Description: Defines the QSpinBox data or behavior contract.
class QSpinBox;
/// Description: Defines the QTabWidget data or behavior contract.
class QTabWidget;
/// Description: Defines the QTimer data or behavior contract.
class QTimer;
/// Description: Defines the QWidget data or behavior contract.
class QWidget;

namespace grav_qt {
/// Description: Defines the EnergyGraphWidget data or behavior contract.
class EnergyGraphWidget;
/// Description: Defines the MultiViewWidget data or behavior contract.
class MultiViewWidget;

/// Owns the primary Qt workspace and synchronizes visible controls with the client runtime.
class MainWindow : public QMainWindow {
public:
    /// Builds the workspace from a configuration snapshot and takes ownership of `runtime`.
    MainWindow(SimulationConfig config, std::string configPath,
               std::unique_ptr<grav_client::IClientRuntime> runtime);
    /// Stops periodic UI updates and releases owned Qt/runtime resources.
    ~MainWindow() override;

private:
    /// Description: Describes the format from selected filter operation contract.
    static std::string formatFromSelectedFilter(const QString& filter);
    /// Description: Describes the apply config to server operation contract.
    bool applyConfigToServer(bool requestReset);
    /// Description: Describes the apply connector settings operation contract.
    void applyConnectorSettings(bool reconnectNow);
    /// Description: Describes the apply config to ui operation contract.
    void applyConfigToUi();
    /// Description: Describes the apply view settings operation contract.
    void applyViewSettings();
    /// Description: Describes the apply theme operation contract.
    void applyTheme();
    /// Description: Describes the capture ui into config operation contract.
    void captureUiIntoConfig();
    /// Description: Describes the apply performance profile to runtime operation contract.
    void applyPerformanceProfileToRuntime();
    /// Description: Describes the build menus operation contract.
    void buildMenus();
    /// Description: Describes the build workspace docks operation contract.
    void buildWorkspaceDocks(QTabWidget* sidebarTabs, QWidget* summaryPane,
                             QWidget* validationPane);
    /// Description: Describes the build telemetry pane operation contract.
    QWidget* buildTelemetryPane();
    /// Description: Describes the build sidebar tabs operation contract.
    QTabWidget* buildSidebarTabs();
    /// Description: Describes the build validation pane operation contract.
    QWidget* buildValidationPane();
    /// Description: Describes the configure remote connector from ui operation contract.
    void configureRemoteConnectorFromUi();
    /// Description: Describes the connect controls operation contract.
    void connectControls();
    /// Description: Describes the handle export request operation contract.
    void handleExportRequest();
    /// Description: Describes the handle save checkpoint request operation contract.
    void handleSaveCheckpointRequest();
    /// Description: Describes the handle load checkpoint request operation contract.
    void handleLoadCheckpointRequest();
    /// Description: Describes the handle load input request operation contract.
    void handleLoadInputRequest();
    /// Description: Describes the handle load preset request operation contract.
    void handleLoadPresetRequest();
    /// Description: Describes the initialize control state operation contract.
    void initializeControlState();
    /// Description: Describes the mark config dirty operation contract.
    void markConfigDirty(bool dirty = true);
    /// Description: Describes the refresh validation report operation contract.
    bool refreshValidationReport(bool blockOnErrors);
    /// Description: Describes the request reconnect from ui operation contract.
    void requestReconnectFromUi();
    /// Description: Describes the reset simulation from ui operation contract.
    void resetSimulationFromUi();
    /// Description: Describes the restore default workspace operation contract.
    void restoreDefaultWorkspace();
    /// Description: Describes the save workspace preset operation contract.
    void saveWorkspacePreset();
    /// Description: Describes the load workspace preset operation contract.
    void loadWorkspacePreset();
    /// Description: Describes the delete workspace preset operation contract.
    void deleteWorkspacePreset();
    /// Description: Describes the build validation text operation contract.
    QString buildValidationText(const grav_config::ScenarioValidationReport& report,
                                const ThroughputAdvisory& advisory) const;
    /// Description: Describes the save config to disk operation contract.
    bool saveConfigToDisk();
    /// Description: Describes the show throughput advisory operation contract.
    void showThroughputAdvisory(const ThroughputAdvisory& advisory);
    /// Description: Describes the update3 d camera from sliders operation contract.
    void update3DCameraFromSliders();
    /// Description: Describes the tick operation contract.
    void tick();
    SimulationConfig _config;
    std::string _configPath;
    std::unique_ptr<grav_client::IClientRuntime> _runtime;
    QPointer<MultiViewWidget> _multiView;
    QPointer<EnergyGraphWidget> _energyGraph;
    QPointer<QLabel> _validationLabel;
    QPointer<QLabel> _statusLabel;
    QPointer<QLabel> _runtimeMetricsLabel;
    QPointer<QLabel> _queueMetricsLabel;
    QPointer<QLabel> _energyMetricsLabel;
    QPointer<QLabel> _gpuMetricsLabel;
    QPointer<QPushButton> _pauseButton;
    QPointer<QPushButton> _stepButton;
    QPointer<QPushButton> _resetButton;
    QPointer<QPushButton> _recoverButton;
    QPointer<QPushButton> _applyConnectorButton;
    QPointer<QPushButton> _exportButton;
    QPointer<QPushButton> _saveConfigButton;
    QPointer<QPushButton> _loadInputButton;
    QPointer<QCheckBox> _serverAutostartCheck;
    QPointer<QLineEdit> _serverHostEdit;
    QPointer<QLineEdit> _serverBinEdit;
    QPointer<QSpinBox> _serverPortSpin;
    QPointer<QCheckBox> _sphCheck;
    QPointer<QDoubleSpinBox> _sphSmoothingSpin;
    QPointer<QDoubleSpinBox> _sphRestDensitySpin;
    QPointer<QDoubleSpinBox> _sphGasConstantSpin;
    QPointer<QDoubleSpinBox> _sphViscositySpin;
    QPointer<QDoubleSpinBox> _dtSpin;
    QPointer<QSlider> _zoomSlider;
    QPointer<QSlider> _luminositySlider;
    QPointer<QComboBox> _solverCombo;
    QPointer<QComboBox> _integratorCombo;
    QPointer<QComboBox> _performanceCombo;
    QPointer<QComboBox> _simulationProfileCombo;
    QPointer<QComboBox> _presetCombo;
    QPointer<QComboBox> _view3dCombo;
    QPointer<QDoubleSpinBox> _thetaSpin;
    QPointer<QDoubleSpinBox> _softeningSpin;
    QPointer<QPushButton> _applyPresetButton;
    QPointer<QPushButton> _loadPresetButton;
    QPointer<QSlider> _yawSlider;
    QPointer<QSlider> _pitchSlider;
    QPointer<QSlider> _rollSlider;
    QPointer<QCheckBox> _cullingCheck;
    QPointer<QCheckBox> _lodCheck;
    QPointer<QCheckBox> _octreeOverlayCheck;
    QPointer<QSpinBox> _octreeOverlayDepthSpin;
    QPointer<QSpinBox> _octreeOverlayOpacitySpin;
    QPointer<QCheckBox> _gpuTelemetryCheck;
    QPointer<QAction> _octreeOverlayAction;
    QPointer<QAction> _gpuTelemetryAction;
    QPointer<QDockWidget> _controlsDock;
    QPointer<QDockWidget> _energyDock;
    QPointer<QDockWidget> _telemetryDock;
    QPointer<QDockWidget> _validationDock;
    QPointer<QTimer> _timer;
    MainWindowController _controller;
    MainWindowPresenter _presenter;
    WorkspaceLayoutStore _workspaceLayouts;
    QByteArray _defaultWorkspaceGeometry;
    QByteArray _defaultWorkspaceState;
    std::uint64_t _lastEnergyStep;
    std::uint32_t _clientDrawCap;
    float _uiTickFps;
    bool _configDirty;
    std::chrono::steady_clock::time_point _lastUiTickAt;
};
} // namespace grav_qt
#endif // GRAVITY_MODULES_QT_INCLUDE_UI_MAINWINDOW_HPP_
