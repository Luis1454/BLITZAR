// File: modules/qt/ui/MainWindow.cpp
// Purpose: Client module implementation for BLITZAR extension workflows.

#include "ui/MainWindow.hpp"
#include "client/ClientCommon.hpp"
#include "ui/EnergyGraphWidget.hpp"
#include "ui/MultiViewWidget.hpp"
#include "ui/QtTheme.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QStyleFactory>
#include <QTimer>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
namespace grav_qt {
MainWindow::MainWindow(SimulationConfig config, std::string configPath,
                       std::unique_ptr<grav_client::IClientRuntime> runtime)
    : QMainWindow(nullptr),
      _config(std::move(config)),
      _configPath(std::move(configPath)),
      _runtime(std::move(runtime)),
      _multiView(new MultiViewWidget()),
      _energyGraph(new EnergyGraphWidget()),
      _validationLabel(new QLabel(this)),
      _statusLabel(new QLabel(this)),
      _runtimeMetricsLabel(new QLabel(this)),
      _queueMetricsLabel(new QLabel(this)),
      _energyMetricsLabel(new QLabel(this)),
      _gpuMetricsLabel(new QLabel(this)),
      _pauseButton(new QPushButton("Pause", this)),
      _stepButton(new QPushButton("Step", this)),
      _resetButton(new QPushButton("Reset", this)),
      _recoverButton(new QPushButton("Recover", this)),
      _applyConnectorButton(new QPushButton("Connect", this)),
      _exportButton(new QPushButton("Export", this)),
      _saveConfigButton(new QPushButton("Save config", this)),
      _loadInputButton(new QPushButton("Load input", this)),
      _serverAutostartCheck(new QCheckBox("autostart server", this)),
      _serverHostEdit(new QLineEdit(this)),
      _serverBinEdit(new QLineEdit(this)),
      _serverPortSpin(new QSpinBox(this)),
      _sphCheck(new QCheckBox("SPH", this)),
      _sphSmoothingSpin(new QDoubleSpinBox(this)),
      _sphRestDensitySpin(new QDoubleSpinBox(this)),
      _sphGasConstantSpin(new QDoubleSpinBox(this)),
      _sphViscositySpin(new QDoubleSpinBox(this)),
      _dtSpin(new QDoubleSpinBox(this)),
      _zoomSlider(new QSlider(Qt::Horizontal, this)),
      _luminositySlider(new QSlider(Qt::Horizontal, this)),
      _solverCombo(new QComboBox(this)),
      _integratorCombo(new QComboBox(this)),
      _performanceCombo(new QComboBox(this)),
      _simulationProfileCombo(new QComboBox(this)),
      _presetCombo(new QComboBox(this)),
      _view3dCombo(new QComboBox(this)),
      _thetaSpin(new QDoubleSpinBox(this)),
      _softeningSpin(new QDoubleSpinBox(this)),
      _applyPresetButton(new QPushButton("Apply preset", this)),
      _loadPresetButton(new QPushButton("Load preset file", this)),
      _yawSlider(new QSlider(Qt::Horizontal, this)),
      _pitchSlider(new QSlider(Qt::Horizontal, this)),
      _rollSlider(new QSlider(Qt::Horizontal, this)),
      _cullingCheck(new QCheckBox("Culling", this)),
      _lodCheck(new QCheckBox("LOD", this)),
      _octreeOverlayCheck(new QCheckBox("Octree overlay", this)),
      _octreeOverlayDepthSpin(new QSpinBox(this)),
      _octreeOverlayOpacitySpin(new QSpinBox(this)),
      _gpuTelemetryCheck(new QCheckBox("GPU telemetry", this)),
      _octreeOverlayAction(nullptr),
      _gpuTelemetryAction(nullptr),
      _controlsDock(nullptr),
      _energyDock(nullptr),
      _telemetryDock(nullptr),
      _validationDock(nullptr),
      _timer(new QTimer(this)),
      _workspaceLayouts(_configPath),
      _lastEnergyStep(std::numeric_limits<std::uint64_t>::max()),
      _clientDrawCap(grav_client::resolveClientDrawCap(_config)),
      _uiTickFps(0.0f),
      _configDirty(false),
      /// Description: Executes the _lastUiTickAt operation.
      _lastUiTickAt()
{
    if (!_runtime) {
        /// Description: Executes the invalid_argument operation.
        throw std::invalid_argument("grav_qt::MainWindow requires a valid client runtime");
    }
    /// Description: Executes the setWindowTitle operation.
    setWindowTitle("N-Body Qt Client");
    /// Description: Executes the menuBar operation.
    menuBar()->setNativeMenuBar(false);
    /// Description: Executes the setStyle operation.
    setStyle(QStyleFactory::create("Fusion"));
    /// Description: Executes the setDockNestingEnabled operation.
    setDockNestingEnabled(true);
    setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowNestedDocks |
                   QMainWindow::AllowTabbedDocks);
    /// Description: Executes the initializeControlState operation.
    initializeControlState();
    /// Description: Executes the setCentralWidget operation.
    setCentralWidget(_multiView);
    /// Description: Executes the buildWorkspaceDocks operation.
    buildWorkspaceDocks(buildSidebarTabs(), buildTelemetryPane(), buildValidationPane());
    /// Description: Executes the buildMenus operation.
    buildMenus();
    /// Description: Executes the applyTheme operation.
    applyTheme();
    /// Description: Executes the statusBar operation.
    statusBar()->showMessage("Qt workspace ready", 3000);
    /// Description: Executes the resize operation.
    resize(1280, 820);
    _defaultWorkspaceGeometry = saveGeometry();
    _defaultWorkspaceState = saveState();
    const bool startupConfigValid = applyConfigToServer(false);
    /// Description: Executes the markConfigDirty operation.
    markConfigDirty(false);
    /// Description: Executes the applyViewSettings operation.
    applyViewSettings();
    /// Description: Executes the update3DCameraFromSliders operation.
    update3DCameraFromSliders();
    _applyConnectorButton->setEnabled(true);
    _serverAutostartCheck->setEnabled(true);
    _serverHostEdit->setEnabled(true);
    _serverPortSpin->setEnabled(true);
    _serverBinEdit->setEnabled(true);
    /// Description: Executes the connectControls operation.
    connectControls();
    const std::uint32_t fps = std::max<std::uint32_t>(1u, _config.uiFpsLimit);
    if (startupConfigValid && _runtime->start()) {
        _timer->start(std::max(1, static_cast<int>(1000 / fps)));
    }
    else if (!startupConfigValid) {
        _statusLabel->setText(QString("preflight validation failed; fix config before starting"));
    }
    else {
        _statusLabel->setText(QString("server connection failed (service)"));
    }
}
/// Description: Releases resources owned by MainWindow.
MainWindow::~MainWindow()
{
    _runtime->stop();
}
/// Description: Executes the applyTheme operation.
void MainWindow::applyTheme()
{
    const QtThemeMode mode = QtTheme::resolve(_config.uiTheme);
    _config.uiTheme = QtTheme::toConfigValue(mode);
    /// Description: Executes the setPalette operation.
    setPalette(QtTheme::buildPalette(mode));
    /// Description: Executes the setAutoFillBackground operation.
    setAutoFillBackground(true);
    /// Description: Executes the setStyleSheet operation.
    setStyleSheet(QtTheme::buildMainWindowStyleSheet(mode));
}
/// Description: Executes the applyViewSettings operation.
void MainWindow::applyViewSettings()
{
    _multiView->setZoom(static_cast<float>(_zoomSlider->value()) / 10.0f);
    _multiView->setLuminosity(_luminositySlider->value());
    _multiView->setOctreeOverlay(_octreeOverlayCheck->isChecked(), _octreeOverlayDepthSpin->value(),
                                 _octreeOverlayOpacitySpin->value());
    _multiView->setRenderSettings(_config.renderCullingEnabled, _config.renderLODEnabled,
                                  _config.renderLODNearDistance, _config.renderLODFarDistance);
    _clientDrawCap = grav_client::resolveClientDrawCap(_config);
    _multiView->setMaxDrawParticles(_clientDrawCap);
    _runtime->setRemoteSnapshotCap(_clientDrawCap);
}
} // namespace grav_qt
