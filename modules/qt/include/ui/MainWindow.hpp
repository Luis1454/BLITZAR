/*
 * @file modules/qt/include/ui/MainWindow.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

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
/*
 * @brief Defines the qcheck box type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class QCheckBox;
/*
 * @brief Defines the qcombo box type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class QComboBox;
/*
 * @brief Defines the qdock widget type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class QDockWidget;
/*
 * @brief Defines the qdouble spin box type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class QDoubleSpinBox;
/*
 * @brief Defines the qlabel type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class QLabel;
/*
 * @brief Defines the qline edit type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class QLineEdit;
/*
 * @brief Defines the qaction type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class QAction;
/*
 * @brief Defines the qpush button type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class QPushButton;
/*
 * @brief Defines the qslider type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class QSlider;
/*
 * @brief Defines the qspin box type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class QSpinBox;
/*
 * @brief Defines the qtab widget type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class QTabWidget;
/*
 * @brief Defines the qtimer type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class QTimer;
/*
 * @brief Defines the qwidget type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class QWidget;

namespace grav_qt {
class EnergyGraphWidget;
class MultiViewWidget;

class MainWindow : public QMainWindow {
public:
    MainWindow(SimulationConfig config, std::string configPath,
               std::unique_ptr<grav_client::IClientRuntime> runtime);
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
