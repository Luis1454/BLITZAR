#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_MAINWINDOW_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_MAINWINDOW_HPP_
/* * Module: ui * Responsibility: Define the top-level Qt workspace and its persistent widget
 * state. */
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
class QCheckBox;
class QComboBox;
class QDockWidget;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QAction;
class QPushButton;
class QSlider;
class QSpinBox;
class QTabWidget;
class QTimer;
class QWidget;
namespace grav_qt {
class EnergyGraphWidget;
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
    void markConfigDirty(bool dirty = true);
    bool refreshValidationReport(bool blockOnErrors);
    void requestReconnectFromUi();
    void resetSimulationFromUi();
    void restoreDefaultWorkspace();
    void saveWorkspacePreset();
    void loadWorkspacePreset();
    void deleteWorkspacePreset();
    QString buildValidationText(const grav_config::ScenarioValidationReport& report,
                                const ThroughputAdvisory& advisory) const;
    bool saveConfigToDisk();
    void showThroughputAdvisory(const ThroughputAdvisory& advisory);
    void update3DCameraFromSliders();
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
