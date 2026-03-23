#include "ui/MainWindow.hpp"

#include "client/ClientCommon.hpp"
#include "config/SimulationPerformanceProfile.hpp"
#include "config/SimulationProfile.hpp"
#include "config/SimulationScenarioValidation.hpp"
#include "server/SimulationInitConfig.hpp"
#include "ui/EnergyGraphWidget.hpp"
#include "ui/MultiViewWidget.hpp"
#include "ui/QtTheme.hpp"
#include "ui/QtViewMath.hpp"

#include <QAction>
#include <QActionGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QPushButton>
#include <QSlider>
#include <QSplitter>
#include <QSpinBox>
#include <QStatusBar>
#include <QStyleFactory>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace grav_qt {

static QFrame *makeSummaryCard(QWidget *parent, const QString &title, QLabel *content)
{
    auto *card = new QFrame(parent);
    card->setObjectName("runtimeCard");
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(4);

    auto *titleLabel = new QLabel(title, card);
    titleLabel->setObjectName("runtimeCardTitle");
    layout->addWidget(titleLabel);
    layout->addWidget(content);
    layout->addStretch(1);
    return card;
}

std::string MainWindow::formatFromSelectedFilter(const QString &filter)
{
    if (filter.startsWith("VTK ASCII")) {
        return "vtk";
    }
    if (filter.startsWith("VTK BINARY")) {
        return "vtk_binary";
    }
    if (filter.startsWith("XYZ")) {
        return "xyz";
    }
    if (filter.startsWith("Native binary")) {
        return "bin";
    }
    return {};
}
MainWindow::MainWindow(
    SimulationConfig config,
    std::string configPath,
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
      _timer(new QTimer(this)),
      _workspaceLayouts(_configPath),
      _lastEnergyStep(std::numeric_limits<std::uint64_t>::max()),
      _clientDrawCap(grav_client::resolveClientDrawCap(_config)),
      _uiTickFps(0.0f),
      _configDirty(false),
      _lastUiTickAt()
{
    if (!_runtime) {
        throw std::invalid_argument("grav_qt::MainWindow requires a valid client runtime");
    }

    setWindowTitle("N-Body Qt Client");
    menuBar()->setNativeMenuBar(false);
    setStyle(QStyleFactory::create("Fusion"));
    setDockNestingEnabled(true);
    setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks);

    _pauseButton->setCheckable(true);
    _solverCombo->addItem("pairwise_cuda");
    _solverCombo->addItem("octree_gpu");
    _solverCombo->addItem("octree_cpu");
    const int solverIndex = std::max(0, _solverCombo->findText(QString::fromStdString(_config.solver)));
    _solverCombo->setCurrentIndex(solverIndex);
    _integratorCombo->addItem("euler");
    _integratorCombo->addItem("rk4");
    const int integratorIndex = std::max(0, _integratorCombo->findText(QString::fromStdString(_config.integrator)));
    _integratorCombo->setCurrentIndex(integratorIndex);
    _performanceCombo->addItem("interactive");
    _performanceCombo->addItem("balanced");
    _performanceCombo->addItem("quality");
    _performanceCombo->addItem("custom");
    _performanceCombo->setObjectName("performanceProfileCombo");
    _performanceCombo->setCurrentIndex(std::max(0, _performanceCombo->findText(QString::fromStdString(_config.performanceProfile))));
    _simulationProfileCombo->addItem("disk_orbit");
    _simulationProfileCombo->addItem("galaxy_collision");
    _simulationProfileCombo->addItem("plummer_sphere");
    _simulationProfileCombo->addItem("binary_star");
    _simulationProfileCombo->addItem("solar_system");
    _simulationProfileCombo->addItem("sph_collapse");
    _simulationProfileCombo->setObjectName("simulationProfileCombo");
    _simulationProfileCombo->setCurrentIndex(std::max(0, _simulationProfileCombo->findText(QString::fromStdString(_config.simulationProfile))));
    _presetCombo->addItem("disk_orbit");
    _presetCombo->addItem("galaxy_collision");
    _presetCombo->addItem("random_cloud");
    _presetCombo->addItem("two_body");
    _presetCombo->addItem("three_body");
    _presetCombo->addItem("plummer_sphere");
    _presetCombo->addItem("file");
    _presetCombo->setObjectName("scenePresetCombo");
    _presetCombo->setCurrentIndex(std::max(0, _presetCombo->findText(QString::fromStdString(_config.presetStructure))));
    _pauseButton->setObjectName("pauseToggleButton");
    _stepButton->setObjectName("stepButton");
    _resetButton->setObjectName("resetButton");
    _recoverButton->setObjectName("recoverButton");
    _applyConnectorButton->setObjectName("connectButton");
    _exportButton->setObjectName("exportSnapshotButton");
    _saveConfigButton->setObjectName("saveConfigButton");
    _loadInputButton->setObjectName("loadInputButton");
    _applyPresetButton->setObjectName("applyPresetButton");
    _loadPresetButton->setObjectName("loadPresetButton");
    _serverAutostartCheck->setObjectName("serverAutostartCheck");
    _sphCheck->setObjectName("sphEnabledCheck");
    _cullingCheck->setObjectName("renderCullingCheck");
    _lodCheck->setObjectName("renderLodCheck");
    _octreeOverlayCheck->setObjectName("octreeOverlayCheck");
    _octreeOverlayDepthSpin->setObjectName("octreeOverlayDepthSpin");
    _octreeOverlayOpacitySpin->setObjectName("octreeOverlayOpacitySpin");
    _serverHostEdit->setObjectName("serverHostEdit");
    _serverBinEdit->setObjectName("serverBinaryEdit");
    _serverPortSpin->setObjectName("serverPortSpin");
    _solverCombo->setObjectName("solverCombo");
    _integratorCombo->setObjectName("integratorCombo");
    _view3dCombo->setObjectName("view3dModeCombo");
    _dtSpin->setObjectName("dtSpin");
    _thetaSpin->setObjectName("octreeThetaSpin");
    _softeningSpin->setObjectName("octreeSofteningSpin");
    _sphSmoothingSpin->setObjectName("sphSmoothingSpin");
    _sphRestDensitySpin->setObjectName("sphRestDensitySpin");
    _sphGasConstantSpin->setObjectName("sphGasConstantSpin");
    _sphViscositySpin->setObjectName("sphViscositySpin");
    _zoomSlider->setObjectName("zoomSlider");
    _luminositySlider->setObjectName("luminositySlider");
    _yawSlider->setObjectName("yawSlider");
    _pitchSlider->setObjectName("pitchSlider");
    _rollSlider->setObjectName("rollSlider");
    _sphCheck->setChecked(_config.sphEnabled);

    _dtSpin->setDecimals(5);
    _dtSpin->setRange(0.00001, 100.0);
    _dtSpin->setSingleStep(0.001);
    _dtSpin->setValue(std::max(0.00001f, _config.dt));
    _thetaSpin->setDecimals(3);
    _thetaSpin->setRange(0.05, 4.0);
    _thetaSpin->setSingleStep(0.05);
    _thetaSpin->setValue(std::clamp(_config.octreeTheta, 0.05f, 4.0f));
    _softeningSpin->setDecimals(4);
    _softeningSpin->setRange(0.0001, 5.0);
    _softeningSpin->setSingleStep(0.01);
    _softeningSpin->setValue(std::clamp(_config.octreeSoftening, 0.0001f, 5.0f));

    _sphSmoothingSpin->setDecimals(3);
    _sphSmoothingSpin->setRange(0.05, 10.0);
    _sphSmoothingSpin->setSingleStep(0.05);
    _sphSmoothingSpin->setValue(std::max(0.05f, _config.sphSmoothingLength));
    _sphRestDensitySpin->setDecimals(3);
    _sphRestDensitySpin->setRange(0.05, 1000.0);
    _sphRestDensitySpin->setSingleStep(0.05);
    _sphRestDensitySpin->setValue(std::max(0.05f, _config.sphRestDensity));
    _sphGasConstantSpin->setDecimals(3);
    _sphGasConstantSpin->setRange(0.01, 1000.0);
    _sphGasConstantSpin->setSingleStep(0.1);
    _sphGasConstantSpin->setValue(std::max(0.01f, _config.sphGasConstant));
    _sphViscositySpin->setDecimals(4);
    _sphViscositySpin->setRange(0.0, 100.0);
    _sphViscositySpin->setSingleStep(0.01);
    _sphViscositySpin->setValue(std::max(0.0f, _config.sphViscosity));

    _zoomSlider->setRange(1, 400);
    _zoomSlider->setValue(static_cast<int>(std::clamp(_config.defaultZoom * 10.0f, 1.0f, 400.0f)));
    _luminositySlider->setRange(0, 255);
    _luminositySlider->setValue(std::clamp(_config.defaultLuminosity, 0, 255));
    _view3dCombo->addItem("perspective");
    _view3dCombo->addItem("iso");
    _yawSlider->setRange(-180, 180);
    _yawSlider->setValue(0);
    _pitchSlider->setRange(-90, 90);
    _pitchSlider->setValue(0);
    _rollSlider->setRange(-180, 180);
    _rollSlider->setValue(0);
    _octreeOverlayDepthSpin->setRange(0, 8);
    _octreeOverlayDepthSpin->setValue(3);
    _octreeOverlayOpacitySpin->setRange(0, 255);
    _octreeOverlayOpacitySpin->setValue(96);
    _cullingCheck->setChecked(_config.renderCullingEnabled);
    _lodCheck->setChecked(_config.renderLODEnabled);
    _serverHostEdit->setText("127.0.0.1");
    _serverPortSpin->setRange(1, 65535);
    _serverPortSpin->setValue(4545);
    _serverAutostartCheck->setChecked(false);
    _serverBinEdit->setPlaceholderText("blitzar-server(.exe)");
    _serverBinEdit->setToolTip("Path to the server executable used when autostart is enabled");
    _applyConnectorButton->setToolTip("Apply host, port and server binary settings, then reconnect now");
    _validationLabel->setWordWrap(true);
    _validationLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    _validationLabel->setContentsMargins(6, 4, 6, 4);
    _statusLabel->setWordWrap(true);
    _statusLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    _statusLabel->setObjectName("runtimeSummaryValue");
    _runtimeMetricsLabel->setWordWrap(true);
    _runtimeMetricsLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    _runtimeMetricsLabel->setObjectName("runtimeSummaryValue");
    _queueMetricsLabel->setWordWrap(true);
    _queueMetricsLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    _queueMetricsLabel->setObjectName("runtimeSummaryValue");
    _energyMetricsLabel->setWordWrap(true);
    _energyMetricsLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    _energyMetricsLabel->setObjectName("runtimeSummaryValue");
    _gpuMetricsLabel->setWordWrap(true);
    _gpuMetricsLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    _gpuMetricsLabel->setObjectName("runtimeSummaryValue");
    _validationLabel->setObjectName("validationLabel");
    _energyGraph->setObjectName("energyGraphWidget");
    _gpuTelemetryCheck->setObjectName("gpuTelemetryCheck");
    _multiView->setObjectName("multiViewWidget");

    auto *sidebarTabs = new QTabWidget(this);
    sidebarTabs->setObjectName("workspaceSidebarTabs");
    sidebarTabs->setTabPosition(QTabWidget::West);
    sidebarTabs->setDocumentMode(true);
    sidebarTabs->setMinimumWidth(220);
    sidebarTabs->setMaximumWidth(248);
    sidebarTabs->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    auto *runPage = new QWidget(sidebarTabs);
    auto *runLayout = new QVBoxLayout(runPage);
    runLayout->setContentsMargins(4, 4, 4, 4);
    runLayout->setSpacing(8);

    auto *scenePage = new QWidget(sidebarTabs);
    auto *sceneLayout = new QVBoxLayout(scenePage);
    sceneLayout->setContentsMargins(4, 4, 4, 4);
    sceneLayout->setSpacing(8);

    auto *physicsPage = new QWidget(sidebarTabs);
    auto *physicsLayout = new QVBoxLayout(physicsPage);
    physicsLayout->setContentsMargins(4, 4, 4, 4);
    physicsLayout->setSpacing(8);

    auto *renderPage = new QWidget(sidebarTabs);
    auto *renderLayout = new QVBoxLayout(renderPage);
    renderLayout->setContentsMargins(4, 4, 4, 4);
    renderLayout->setSpacing(8);

    auto *runBox = new QGroupBox("Run Control", runPage);
    auto *runBoxLayout = new QVBoxLayout(runBox);
    runBoxLayout->setSpacing(10);
    auto *runForm = new QFormLayout();
    runForm->addRow("performance", _performanceCombo);
    runBoxLayout->addLayout(runForm);
    auto *runActions = new QGridLayout();
    runActions->setHorizontalSpacing(8);
    runActions->setVerticalSpacing(8);
    runActions->addWidget(_pauseButton, 0, 0);
    runActions->addWidget(_stepButton, 0, 1);
    runActions->addWidget(_resetButton, 1, 0);
    runActions->addWidget(_recoverButton, 1, 1);
    runBoxLayout->addLayout(runActions);

    auto *connectorBox = new QGroupBox("Connector", runPage);
    auto *connectorLayout = new QVBoxLayout(connectorBox);
    connectorLayout->setSpacing(10);
    auto *connectorForm = new QFormLayout();
    connectorForm->addRow("host", _serverHostEdit);
    connectorForm->addRow("port", _serverPortSpin);
    connectorForm->addRow("server bin", _serverBinEdit);
    connectorLayout->addLayout(connectorForm);
    connectorLayout->addWidget(_serverAutostartCheck);
    connectorLayout->addWidget(_applyConnectorButton);
    connectorLayout->addStretch(1);

    auto *sceneBox = new QGroupBox("Scene Setup", scenePage);
    auto *sceneBoxLayout = new QVBoxLayout(sceneBox);
    auto *sceneForm = new QFormLayout();
    sceneForm->addRow("profile", _simulationProfileCombo);
    sceneForm->addRow("preset", _presetCombo);
    sceneBoxLayout->addLayout(sceneForm);
    sceneBoxLayout->addWidget(_applyPresetButton);
    sceneBoxLayout->addWidget(_loadPresetButton);
    sceneBoxLayout->addWidget(_loadInputButton);

    auto *projectBox = new QGroupBox("Project", scenePage);
    auto *projectLayout = new QVBoxLayout(projectBox);
    projectLayout->addWidget(_saveConfigButton);
    projectLayout->addStretch(1);

    auto *physicsCoreBox = new QGroupBox("Physics Core", physicsPage);
    auto *physicsCoreLayout = new QVBoxLayout(physicsCoreBox);
    auto *physicsForm = new QFormLayout();
    physicsForm->addRow("solver", _solverCombo);
    physicsForm->addRow("integrator", _integratorCombo);
    physicsForm->addRow("dt", _dtSpin);
    physicsForm->addRow("theta", _thetaSpin);
    physicsForm->addRow("softening", _softeningSpin);
    physicsCoreLayout->addLayout(physicsForm);

    auto *sphBox = new QGroupBox("SPH", physicsPage);
    auto *sphLayout = new QVBoxLayout(sphBox);
    sphLayout->addWidget(_sphCheck);
    auto *sphForm = new QFormLayout();
    sphForm->addRow("h", _sphSmoothingSpin);
    sphForm->addRow("rest density", _sphRestDensitySpin);
    sphForm->addRow("gas K", _sphGasConstantSpin);
    sphForm->addRow("viscosity", _sphViscositySpin);
    sphLayout->addLayout(sphForm);

    auto *exportBox = new QGroupBox("Export", renderPage);
    auto *exportLayout = new QVBoxLayout(exportBox);
    exportLayout->addWidget(_exportButton);
    exportLayout->addWidget(_gpuTelemetryCheck);
    exportLayout->addStretch(1);

    auto *overlayBox = new QGroupBox("Octree Overlay", renderPage);
    auto *overlayLayout = new QFormLayout(overlayBox);
    overlayLayout->addRow(_octreeOverlayCheck);
    overlayLayout->addRow("depth", _octreeOverlayDepthSpin);
    overlayLayout->addRow("opacity", _octreeOverlayOpacitySpin);

    auto *cameraBox = new QGroupBox("View & Camera", renderPage);
    auto *cameraLayout = new QGridLayout(cameraBox);
    cameraLayout->addWidget(new QLabel("zoom", this), 0, 0);
    cameraLayout->addWidget(_zoomSlider, 0, 1);
    cameraLayout->addWidget(new QLabel("luminosity", this), 0, 2);
    cameraLayout->addWidget(_luminositySlider, 0, 3);
    cameraLayout->addWidget(new QLabel("3D view", this), 1, 0);
    cameraLayout->addWidget(_view3dCombo, 1, 1);
    cameraLayout->addWidget(new QLabel("yaw", this), 1, 2);
    cameraLayout->addWidget(_yawSlider, 1, 3);
    cameraLayout->addWidget(new QLabel("pitch", this), 2, 0);
    cameraLayout->addWidget(_pitchSlider, 2, 1);
    cameraLayout->addWidget(new QLabel("roll", this), 2, 2);
    cameraLayout->addWidget(_rollSlider, 2, 3);
    cameraLayout->addWidget(_cullingCheck, 3, 0);
    cameraLayout->addWidget(_lodCheck, 3, 1);

    runLayout->addWidget(runBox);
    runLayout->addWidget(connectorBox);
    runLayout->addStretch(1);

    sceneLayout->addWidget(sceneBox);
    sceneLayout->addWidget(projectBox);
    sceneLayout->addStretch(1);

    physicsLayout->addWidget(physicsCoreBox);
    physicsLayout->addWidget(sphBox);
    physicsLayout->addStretch(1);

    renderLayout->addWidget(cameraBox);
    renderLayout->addWidget(overlayBox);
    renderLayout->addWidget(exportBox);
    renderLayout->addStretch(1);

    sidebarTabs->addTab(runPage, "Run");
    sidebarTabs->addTab(scenePage, "Scene");
    sidebarTabs->addTab(physicsPage, "Physics");
    sidebarTabs->addTab(renderPage, "Render");

    auto *summaryPane = new QWidget(this);
    summaryPane->setObjectName("telemetrySummaryPane");
    auto *summaryLayout = new QGridLayout(summaryPane);
    summaryLayout->setContentsMargins(8, 8, 8, 8);
    summaryLayout->setHorizontalSpacing(8);
    summaryLayout->setVerticalSpacing(8);
    summaryLayout->addWidget(makeSummaryCard(summaryPane, "Session", _statusLabel), 0, 0);
    summaryLayout->addWidget(makeSummaryCard(summaryPane, "Timing", _runtimeMetricsLabel), 0, 1);
    summaryLayout->addWidget(makeSummaryCard(summaryPane, "Pipeline", _queueMetricsLabel), 1, 0);
    summaryLayout->addWidget(makeSummaryCard(summaryPane, "Energy", _energyMetricsLabel), 1, 1);
    summaryLayout->addWidget(makeSummaryCard(summaryPane, "GPU", _gpuMetricsLabel), 2, 0, 1, 2);

    auto *validationPane = new QWidget(this);
    auto *validationLayout = new QVBoxLayout(validationPane);
    validationLayout->setContentsMargins(8, 8, 8, 8);
    validationLayout->setSpacing(4);
    validationLayout->addWidget(_validationLabel);
    validationLayout->addStretch(1);

    setCentralWidget(_multiView);

    auto *controlsDock = new QDockWidget("Controls", this);
    controlsDock->setObjectName("controlsDock");
    controlsDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    controlsDock->setWidget(sidebarTabs);
    controlsDock->setMinimumWidth(236);
    addDockWidget(Qt::LeftDockWidgetArea, controlsDock);

    auto *telemetryDock = new QDockWidget("Telemetry", this);
    telemetryDock->setObjectName("telemetryDock");
    telemetryDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    telemetryDock->setWidget(summaryPane);
    telemetryDock->setMinimumHeight(164);
    addDockWidget(Qt::BottomDockWidgetArea, telemetryDock);

    auto *energyDock = new QDockWidget("Energy", this);
    energyDock->setObjectName("energyDock");
    energyDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    energyDock->setWidget(_energyGraph);
    energyDock->setMinimumHeight(136);
    addDockWidget(Qt::BottomDockWidgetArea, energyDock);

    auto *validationDock = new QDockWidget("Validation", this);
    validationDock->setObjectName("validationDock");
    validationDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    validationDock->setWidget(validationPane);
    addDockWidget(Qt::BottomDockWidgetArea, validationDock);
    tabifyDockWidget(telemetryDock, validationDock);
    resizeDocks({controlsDock}, {236}, Qt::Horizontal);
    resizeDocks({energyDock}, {148}, Qt::Vertical);
    energyDock->raise();
    telemetryDock->hide();
    validationDock->hide();

    auto *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("Save Config", this, [this]() { (void)saveConfigToDisk(); }, QKeySequence::Save);
    fileMenu->addAction("Load Preset...", this, [this]() { handleLoadPresetRequest(); });
    fileMenu->addAction("Load Checkpoint...", this, [this]() { handleLoadCheckpointRequest(); });
    fileMenu->addAction("Load Input...", this, [this]() { handleLoadInputRequest(); });
    fileMenu->addAction("Save Checkpoint...", this, [this]() { handleSaveCheckpointRequest(); });
    fileMenu->addAction("Export Snapshot...", this, [this]() { handleExportRequest(); });
    fileMenu->addSeparator();
    fileMenu->addAction("Quit", this, [this]() { close(); }, QKeySequence::Quit);

    auto *editMenu = menuBar()->addMenu("&Edit");
    editMenu->addAction("Validate Config", this, [this]() { (void)refreshValidationReport(false); });
    editMenu->addAction("Reconnect", this, [this]() { requestReconnectFromUi(); });

    auto *viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction(controlsDock->toggleViewAction());
    viewMenu->addAction(energyDock->toggleViewAction());
    viewMenu->addAction(telemetryDock->toggleViewAction());
    viewMenu->addAction(validationDock->toggleViewAction());
    _octreeOverlayAction = viewMenu->addAction("Octree Overlay");
    _octreeOverlayAction->setObjectName("octreeOverlayAction");
    _octreeOverlayAction->setCheckable(true);
    _octreeOverlayAction->setChecked(false);
    _gpuTelemetryAction = viewMenu->addAction("GPU Telemetry");
    _gpuTelemetryAction->setObjectName("gpuTelemetryAction");
    _gpuTelemetryAction->setCheckable(true);
    _gpuTelemetryAction->setChecked(false);
    auto *themeMenu = viewMenu->addMenu("Theme");
    auto *themeGroup = new QActionGroup(themeMenu);
    themeGroup->setExclusive(true);
    auto *lightThemeAction = themeMenu->addAction("Light");
    lightThemeAction->setObjectName("themeLightAction");
    lightThemeAction->setCheckable(true);
    themeGroup->addAction(lightThemeAction);
    auto *darkThemeAction = themeMenu->addAction("Dark");
    darkThemeAction->setObjectName("themeDarkAction");
    darkThemeAction->setCheckable(true);
    themeGroup->addAction(darkThemeAction);
    if (QtTheme::resolve(_config.uiTheme) == QtThemeMode::Dark) {
        darkThemeAction->setChecked(true);
    } else {
        lightThemeAction->setChecked(true);
    }

    auto *simulationMenu = menuBar()->addMenu("&Simulation");
    simulationMenu->addAction("Pause / Resume", this, [this]() { _pauseButton->click(); }, Qt::Key_Space);
    simulationMenu->addAction("Step", this, [this]() { _stepButton->click(); });
    simulationMenu->addAction("Reset", this, [this]() { _resetButton->click(); });
    simulationMenu->addAction("Recover", this, [this]() { _recoverButton->click(); });

    auto *windowMenu = menuBar()->addMenu("&Window");
    windowMenu->addAction("Raise Controls", this, [controlsDock]() { controlsDock->raise(); });
    windowMenu->addAction("Raise Energy", this, [energyDock]() { energyDock->raise(); });
    windowMenu->addAction("Raise Telemetry", this, [telemetryDock]() { telemetryDock->raise(); });
    windowMenu->addAction("Raise Validation", this, [validationDock]() { validationDock->raise(); });
    auto *workspaceMenu = windowMenu->addMenu("Workspace");
    workspaceMenu->addAction("Save Workspace...", this, [this]() { saveWorkspacePreset(); });
    workspaceMenu->addAction("Load Workspace...", this, [this]() { loadWorkspacePreset(); });
    workspaceMenu->addAction("Delete Workspace...", this, [this]() { deleteWorkspacePreset(); });
    workspaceMenu->addSeparator();
    workspaceMenu->addAction("Restore Default Workspace", this, [this]() { restoreDefaultWorkspace(); });

    auto *helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("About Workspace", this, [this]() {
        _statusLabel->setText("Workspace shell active");
        statusBar()->showMessage("Workspace shell active", 3000);
    });
    connect(lightThemeAction, &QAction::triggered, this, [this]() {
        _config.uiTheme = "light";
        applyTheme();
        markConfigDirty();
        statusBar()->showMessage("Theme applied: light", 3000);
    });
    connect(darkThemeAction, &QAction::triggered, this, [this]() {
        _config.uiTheme = "dark";
        applyTheme();
        markConfigDirty();
        statusBar()->showMessage("Theme applied: dark", 3000);
    });
    connect(_octreeOverlayCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        if (_octreeOverlayAction != nullptr && _octreeOverlayAction->isChecked() != enabled) {
            _octreeOverlayAction->blockSignals(true);
            _octreeOverlayAction->setChecked(enabled);
            _octreeOverlayAction->blockSignals(false);
        }
        _multiView->setOctreeOverlay(enabled, _octreeOverlayDepthSpin->value(), _octreeOverlayOpacitySpin->value());
        statusBar()->showMessage(enabled ? "Octree overlay enabled" : "Octree overlay disabled", 3000);
    });
    connect(_octreeOverlayAction, &QAction::toggled, this, [this](bool enabled) {
        if (_octreeOverlayCheck != nullptr && _octreeOverlayCheck->isChecked() != enabled) {
            _octreeOverlayCheck->blockSignals(true);
            _octreeOverlayCheck->setChecked(enabled);
            _octreeOverlayCheck->blockSignals(false);
            _multiView->setOctreeOverlay(enabled, _octreeOverlayDepthSpin->value(), _octreeOverlayOpacitySpin->value());
            statusBar()->showMessage(enabled ? "Octree overlay enabled" : "Octree overlay disabled", 3000);
        }
    });
    connect(_gpuTelemetryCheck, &QCheckBox::toggled, this, [this, telemetryDock](bool enabled) {
        if (_gpuTelemetryAction != nullptr && _gpuTelemetryAction->isChecked() != enabled) {
            _gpuTelemetryAction->blockSignals(true);
            _gpuTelemetryAction->setChecked(enabled);
            _gpuTelemetryAction->blockSignals(false);
        }
        _runtime->setGpuTelemetryEnabled(enabled);
        if (enabled) {
            telemetryDock->show();
            telemetryDock->raise();
        }
        statusBar()->showMessage(enabled ? "GPU telemetry enabled" : "GPU telemetry disabled", 3000);
    });
    connect(_gpuTelemetryAction, &QAction::toggled, this, [this](bool enabled) {
        if (_gpuTelemetryCheck != nullptr && _gpuTelemetryCheck->isChecked() != enabled) {
            _gpuTelemetryCheck->blockSignals(true);
            _gpuTelemetryCheck->setChecked(enabled);
            _gpuTelemetryCheck->blockSignals(false);
            _runtime->setGpuTelemetryEnabled(enabled);
            statusBar()->showMessage(enabled ? "GPU telemetry enabled" : "GPU telemetry disabled", 3000);
        }
    });
    applyTheme();
    statusBar()->showMessage("Qt workspace ready", 3000);

    resize(1280, 820);
    _defaultWorkspaceGeometry = saveGeometry();
    _defaultWorkspaceState = saveState();

    const bool startupConfigValid = applyConfigToServer(false);
    markConfigDirty(false);
    applyViewSettings();
    update3DCameraFromSliders();
    _applyConnectorButton->setEnabled(true);
    _serverAutostartCheck->setEnabled(true);
    _serverHostEdit->setEnabled(true);
    _serverPortSpin->setEnabled(true);
    _serverBinEdit->setEnabled(true);
    connectControls();
    const std::uint32_t fps = std::max<std::uint32_t>(1u, _config.uiFpsLimit);
    if (startupConfigValid && _runtime->start()) {
        _timer->start(std::max(1, static_cast<int>(1000 / fps)));
    } else if (!startupConfigValid) {
        _statusLabel->setText(
            QString("preflight validation failed; fix config before starting")
        );
    } else {
        _statusLabel->setText(
            QString("server connection failed (service)")
        );
    }
}

MainWindow::~MainWindow()
{
    _runtime->stop();
}

void MainWindow::applyTheme()
{
    const QtThemeMode mode = QtTheme::resolve(_config.uiTheme);
    _config.uiTheme = QtTheme::toConfigValue(mode);
    setPalette(QtTheme::buildPalette(mode));
    setAutoFillBackground(true);
    setStyleSheet(QtTheme::buildMainWindowStyleSheet(mode));
}

void MainWindow::applyViewSettings()
{
    _multiView->setZoom(static_cast<float>(_zoomSlider->value()) / 10.0f);
    _multiView->setLuminosity(_luminositySlider->value());
    _multiView->setOctreeOverlay(
        _octreeOverlayCheck->isChecked(),
        _octreeOverlayDepthSpin->value(),
        _octreeOverlayOpacitySpin->value());
    _multiView->setRenderSettings(
        _config.renderCullingEnabled,
        _config.renderLODEnabled,
        _config.renderLODNearDistance,
        _config.renderLODFarDistance
    );
    _clientDrawCap = grav_client::resolveClientDrawCap(_config);
    _multiView->setMaxDrawParticles(_clientDrawCap);
    _runtime->setRemoteSnapshotCap(_clientDrawCap);
}

void MainWindow::configureRemoteConnectorFromUi()
{
    std::string host = _serverHostEdit->text().trimmed().toStdString();
    if (host.empty()) {
        host = "127.0.0.1";
        _serverHostEdit->setText(QString::fromStdString(host));
    }
    _runtime->configureRemoteConnector(
        host,
        static_cast<std::uint16_t>(_serverPortSpin->value()),
        _serverAutostartCheck->isChecked(),
        _serverBinEdit->text().trimmed().toStdString());
}

void MainWindow::applyConnectorSettings(bool reconnectNow)
{
    configureRemoteConnectorFromUi();
    if (reconnectNow) {
        _runtime->requestReconnect();
        _energyGraph->clearHistory();
        _lastEnergyStep = std::numeric_limits<std::uint64_t>::max();
        statusBar()->showMessage("Connector updated and reconnect requested", 3000);
        return;
    }
    statusBar()->showMessage("Connector settings updated", 3000);
}

void MainWindow::requestReconnectFromUi()
{
    _runtime->requestReconnect();
    _energyGraph->clearHistory();
    _lastEnergyStep = std::numeric_limits<std::uint64_t>::max();
    statusBar()->showMessage("Reconnect requested", 3000);
}

void MainWindow::handleExportRequest()
{
    const SimulationStats stats = _runtime->getCachedStats();
    const std::string preferredFormat = grav_client::normalizeExportFormat(
        _config.exportFormat.empty() ? std::string("vtk") : _config.exportFormat);
    const QString startPath = QString::fromStdString(
        grav_client::buildSuggestedExportPath(_config.exportDirectory, preferredFormat, stats.steps));
    QString selectedFilter = "VTK ASCII (*.vtk)";
    if (preferredFormat == "vtk_binary") {
        selectedFilter = "VTK BINARY (*.vtk)";
    } else if (preferredFormat == "xyz") {
        selectedFilter = "XYZ (*.xyz)";
    } else if (preferredFormat == "bin") {
        selectedFilter = "Native binary (*.bin)";
    }

    const QString pathChosen = QFileDialog::getSaveFileName(
        this,
        "Export Snapshot",
        startPath,
        "VTK ASCII (*.vtk);;VTK BINARY (*.vtk);;XYZ (*.xyz);;Native binary (*.bin);;All files (*.*)",
        &selectedFilter);
    if (pathChosen.isEmpty()) {
        return;
    }

    std::string format = grav_client::normalizeExportFormat(formatFromSelectedFilter(selectedFilter));
    std::string path = pathChosen.toStdString();
    if (format.empty()) {
        format = grav_client::normalizeExportFormat(grav_client::inferExportFormatFromPath(path));
    }
    if (format.empty()) {
        format = grav_client::normalizeExportFormat(_config.exportFormat);
        if (format.empty()) {
            format = "vtk";
        }
    }

    std::filesystem::path outPath(path);
    if (outPath.extension().empty()) {
        const std::string ext = grav_client::extensionForExportFormat(format);
        if (!ext.empty()) {
            outPath += ext;
            path = outPath.string();
        }
    }

    _runtime->requestExportSnapshot(path, format);
    _config.exportFormat = format;
    if (outPath.has_parent_path()) {
        _config.exportDirectory = outPath.parent_path().string();
    }
    markConfigDirty();
}

void MainWindow::handleSaveCheckpointRequest()
{
    const SimulationStats stats = _runtime->getCachedStats();
    std::filesystem::path startPath =
        std::filesystem::path(_config.exportDirectory.empty() ? "exports" : _config.exportDirectory)
        / ("checkpoint_s" + std::to_string(stats.steps) + ".chk");
    const QString pathChosen = QFileDialog::getSaveFileName(
        this,
        "Save Checkpoint",
        QString::fromStdString(startPath.string()),
        "Checkpoint (*.chk);;All files (*.*)");
    if (pathChosen.isEmpty()) {
        return;
    }

    std::filesystem::path outputPath(pathChosen.toStdString());
    if (outputPath.extension().empty()) {
        outputPath += ".chk";
    }
    _runtime->requestSaveCheckpoint(outputPath.string());
    if (outputPath.has_parent_path()) {
        _config.exportDirectory = outputPath.parent_path().string();
    }
    statusBar()->showMessage("Checkpoint save requested", 3000);
    markConfigDirty();
}

void MainWindow::handleLoadCheckpointRequest()
{
    const QString startPath = _config.inputFile.empty()
        ? QString::fromStdString(_config.exportDirectory)
        : QString::fromStdString(_config.inputFile);
    const QString path = QFileDialog::getOpenFileName(
        this,
        "Load Checkpoint",
        startPath,
        "Checkpoint (*.chk);;All files (*.*)");
    if (path.isEmpty()) {
        return;
    }
    _runtime->requestLoadCheckpoint(path.toStdString());
    _energyGraph->clearHistory();
    _lastEnergyStep = std::numeric_limits<std::uint64_t>::max();
    statusBar()->showMessage("Checkpoint load requested", 3000);
}

void MainWindow::handleLoadInputRequest()
{
    const QString startPath = _config.inputFile.empty()
        ? QString::fromStdString(_config.exportDirectory)
        : QString::fromStdString(_config.inputFile);
    const QString path = QFileDialog::getOpenFileName(
        this,
        "Load Initial State",
        startPath,
        "Simulation files (*.vtk *.xyz *.bin);;All files (*.*)");
    if (path.isEmpty()) {
        return;
    }
    _config.inputFile = path.toStdString();
    _config.inputFormat = "auto";
    _config.presetStructure = "file";
    _config.initMode = "file";
    (void)applyConfigToServer(true);
    markConfigDirty();
}

void MainWindow::handleLoadPresetRequest()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        "Load Preset Config",
        QString::fromStdString(_configPath),
        "Ini files (*.ini);;All files (*.*)");
    if (path.isEmpty()) {
        return;
    }
    _config = SimulationConfig::loadOrCreate(path.toStdString());
    _configPath = path.toStdString();
    applyConfigToUi();
    applyConfigToServer(true);
    _energyGraph->clearHistory();
    _lastEnergyStep = std::numeric_limits<std::uint64_t>::max();
    markConfigDirty(false);
}

void MainWindow::resetSimulationFromUi()
{
    _runtime->requestReset();
    _energyGraph->clearHistory();
    _lastEnergyStep = std::numeric_limits<std::uint64_t>::max();
    statusBar()->showMessage("Simulation reset requested", 3000);
}

void MainWindow::connectControls()
{
    const auto applySphParams = [this]() {
        _config.sphSmoothingLength = static_cast<float>(_sphSmoothingSpin->value());
        _config.sphRestDensity = static_cast<float>(_sphRestDensitySpin->value());
        _config.sphGasConstant = static_cast<float>(_sphGasConstantSpin->value());
        _config.sphViscosity = static_cast<float>(_sphViscositySpin->value());
        _runtime->setSphParameters(
            _config.sphSmoothingLength,
            _config.sphRestDensity,
            _config.sphGasConstant,
            _config.sphViscosity);
        markConfigDirty();
    };

    connect(_pauseButton, &QPushButton::clicked, this, [this](bool checked) {
        _runtime->setPaused(checked);
        _pauseButton->setText(checked ? "Resume" : "Pause");
    });
    connect(_stepButton, &QPushButton::clicked, this, [this]() { _runtime->stepOnce(); });
    connect(_resetButton, &QPushButton::clicked, this, [this]() { resetSimulationFromUi(); });
    connect(_recoverButton, &QPushButton::clicked, this, [this]() { _runtime->requestRecover(); });
    connect(_applyConnectorButton, &QPushButton::clicked, this, [this]() { applyConnectorSettings(true); });
    connect(_exportButton, &QPushButton::clicked, this, [this]() { handleExportRequest(); });
    connect(_saveConfigButton, &QPushButton::clicked, this, [this]() { (void)saveConfigToDisk(); });
    connect(_loadInputButton, &QPushButton::clicked, this, [this]() { handleLoadInputRequest(); });
    connect(_applyPresetButton, &QPushButton::clicked, this, [this]() {
        _config.initConfigStyle = "preset";
        _config.presetStructure = _presetCombo->currentText().toStdString();
        if (_config.presetStructure != "file") {
            _config.initMode = _config.presetStructure;
        }
        applyConfigToServer(true);
        markConfigDirty();
        statusBar()->showMessage(QString("Scene preset applied: %1").arg(_presetCombo->currentText()), 3000);
    });
    connect(_loadPresetButton, &QPushButton::clicked, this, [this]() { handleLoadPresetRequest(); });
    connect(_simulationProfileCombo, &QComboBox::currentTextChanged, this, [this](const QString &profile) {
        _config.simulationProfile = profile.toStdString();
        grav_config::applySimulationProfile(_config);
        applyConfigToUi();
        (void)applyConfigToServer(true);
        markConfigDirty();
        statusBar()->showMessage(QString("Simulation profile applied: %1").arg(profile), 3000);
    });
    connect(_sphCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        _config.sphEnabled = enabled;
        _runtime->setSphEnabled(enabled);
        markConfigDirty();
    });
    connect(_sphSmoothingSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [applySphParams](double) { applySphParams(); });
    connect(_sphRestDensitySpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [applySphParams](double) { applySphParams(); });
    connect(_sphGasConstantSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [applySphParams](double) { applySphParams(); });
    connect(_sphViscositySpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [applySphParams](double) { applySphParams(); });
    connect(_dtSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        _config.dt = static_cast<float>(value);
        _runtime->setDt(static_cast<float>(value));
        markConfigDirty();
    });
    connect(_zoomSlider, &QSlider::valueChanged, this, [this](int value) {
        _config.defaultZoom = static_cast<float>(value) / 10.0f;
        _multiView->setZoom(static_cast<float>(value) / 10.0f);
        markConfigDirty();
    });
    connect(_luminositySlider, &QSlider::valueChanged, this, [this](int value) {
        _config.defaultLuminosity = value;
        _multiView->setLuminosity(value);
        markConfigDirty();
    });
    connect(_octreeOverlayDepthSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
        _multiView->setOctreeOverlay(_octreeOverlayCheck->isChecked(), value, _octreeOverlayOpacitySpin->value());
        statusBar()->showMessage(QString("Octree overlay depth: %1").arg(value), 2000);
    });
    connect(_octreeOverlayOpacitySpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
        _multiView->setOctreeOverlay(_octreeOverlayCheck->isChecked(), _octreeOverlayDepthSpin->value(), value);
        statusBar()->showMessage(QString("Octree overlay opacity: %1").arg(value), 2000);
    });
    connect(_solverCombo, &QComboBox::currentTextChanged, this, [this](const QString &solver) {
        _config.solver = solver.toStdString();
        _runtime->setSolverMode(_config.solver);
        markConfigDirty();
    });
    connect(_integratorCombo, &QComboBox::currentTextChanged, this, [this](const QString &integrator) {
        _config.integrator = integrator.toStdString();
        _runtime->setIntegratorMode(_config.integrator);
        markConfigDirty();
    });
    connect(_performanceCombo, &QComboBox::currentTextChanged, this, [this](const QString &profile) {
        _config.performanceProfile = profile.toStdString();
        grav_config::applyPerformanceProfile(_config);
        applyConfigToUi();
        applyPerformanceProfileToRuntime();
        markConfigDirty();
        statusBar()->showMessage(QString("Run profile applied: %1").arg(profile), 3000);
    });
    connect(_thetaSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        _config.octreeTheta = static_cast<float>(value);
        _runtime->setOctreeParameters(_config.octreeTheta, static_cast<float>(_softeningSpin->value()));
        markConfigDirty();
    });
    connect(_softeningSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        _config.octreeSoftening = static_cast<float>(value);
        _runtime->setOctreeParameters(static_cast<float>(_thetaSpin->value()), _config.octreeSoftening);
        markConfigDirty();
    });
    connect(_view3dCombo, &QComboBox::currentTextChanged, this, [this](const QString &value) {
        _multiView->set3DMode(value == "iso" ? grav::ViewMode::Iso : grav::ViewMode::Perspective);
    });
    connect(_cullingCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        _config.renderCullingEnabled = enabled;
        _multiView->setRenderSettings(
            _config.renderCullingEnabled,
            _config.renderLODEnabled,
            _config.renderLODNearDistance,
            _config.renderLODFarDistance);
        markConfigDirty();
    });
    connect(_lodCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        _config.renderLODEnabled = enabled;
        _multiView->setRenderSettings(
            _config.renderCullingEnabled,
            _config.renderLODEnabled,
            _config.renderLODNearDistance,
            _config.renderLODFarDistance);
        markConfigDirty();
    });
    connect(_yawSlider, &QSlider::valueChanged, this, [this]() { update3DCameraFromSliders(); });
    connect(_pitchSlider, &QSlider::valueChanged, this, [this]() { update3DCameraFromSliders(); });
    connect(_rollSlider, &QSlider::valueChanged, this, [this]() { update3DCameraFromSliders(); });
    connect(_timer, &QTimer::timeout, this, [this]() { tick(); });
}

bool MainWindow::applyConfigToServer(bool requestReset)
{
    captureUiIntoConfig();
    const MainWindowApplyConfigResult result = _controller.applyConfig(_config, *_runtime, requestReset);
    const ThroughputAdvisory advisory = ThroughputAdvisor::evaluate(_config, result.clientDrawCap);
    if (_validationLabel != nullptr) {
        _validationLabel->setText(buildValidationText(result.report, advisory));
    }
    showThroughputAdvisory(advisory);
    if (!result.report.validForRun) {
        if (_statusLabel != nullptr) {
            _statusLabel->setText(QString("preflight validation failed; fix config errors"));
        }
        return false;
    }
    _clientDrawCap = result.clientDrawCap;
    applyViewSettings();
    return true;
}

void MainWindow::applyConfigToUi()
{
    _solverCombo->blockSignals(true);
    _integratorCombo->blockSignals(true);
    _performanceCombo->blockSignals(true);
    _simulationProfileCombo->blockSignals(true);
    _presetCombo->blockSignals(true);
    _sphCheck->blockSignals(true);
    _dtSpin->blockSignals(true);
    _thetaSpin->blockSignals(true);
    _softeningSpin->blockSignals(true);
    _sphSmoothingSpin->blockSignals(true);
    _sphRestDensitySpin->blockSignals(true);
    _sphGasConstantSpin->blockSignals(true);
    _sphViscositySpin->blockSignals(true);
    _zoomSlider->blockSignals(true);
    _luminositySlider->blockSignals(true);
    _cullingCheck->blockSignals(true);
    _lodCheck->blockSignals(true);

    _solverCombo->setCurrentIndex(std::max(0, _solverCombo->findText(QString::fromStdString(_config.solver))));
    _integratorCombo->setCurrentIndex(std::max(0, _integratorCombo->findText(QString::fromStdString(_config.integrator))));
    _performanceCombo->setCurrentIndex(std::max(0, _performanceCombo->findText(QString::fromStdString(_config.performanceProfile))));
    _simulationProfileCombo->setCurrentIndex(std::max(0, _simulationProfileCombo->findText(QString::fromStdString(_config.simulationProfile))));
    const int presetIndex = std::max(0, _presetCombo->findText(QString::fromStdString(_config.presetStructure)));
    _presetCombo->setCurrentIndex(presetIndex);
    _sphCheck->setChecked(_config.sphEnabled);
    _dtSpin->setValue(std::max(0.00001f, _config.dt));
    _thetaSpin->setValue(std::clamp(_config.octreeTheta, 0.05f, 4.0f));
    _softeningSpin->setValue(std::clamp(_config.octreeSoftening, 0.0001f, 5.0f));
    _sphSmoothingSpin->setValue(std::max(0.05f, _config.sphSmoothingLength));
    _sphRestDensitySpin->setValue(std::max(0.05f, _config.sphRestDensity));
    _sphGasConstantSpin->setValue(std::max(0.01f, _config.sphGasConstant));
    _sphViscositySpin->setValue(std::max(0.0f, _config.sphViscosity));
    _zoomSlider->setValue(static_cast<int>(std::clamp(_config.defaultZoom * 10.0f, 1.0f, 400.0f)));
    _luminositySlider->setValue(std::clamp(_config.defaultLuminosity, 0, 255));
    _cullingCheck->setChecked(_config.renderCullingEnabled);
    _lodCheck->setChecked(_config.renderLODEnabled);

    _solverCombo->blockSignals(false);
    _integratorCombo->blockSignals(false);
    _performanceCombo->blockSignals(false);
    _simulationProfileCombo->blockSignals(false);
    _presetCombo->blockSignals(false);
    _sphCheck->blockSignals(false);
    _dtSpin->blockSignals(false);
    _thetaSpin->blockSignals(false);
    _softeningSpin->blockSignals(false);
    _sphSmoothingSpin->blockSignals(false);
    _sphRestDensitySpin->blockSignals(false);
    _sphGasConstantSpin->blockSignals(false);
    _sphViscositySpin->blockSignals(false);
    _zoomSlider->blockSignals(false);
    _luminositySlider->blockSignals(false);
    _cullingCheck->blockSignals(false);
    _lodCheck->blockSignals(false);

    applyViewSettings();
}

void MainWindow::captureUiIntoConfig()
{
    _config.solver = _solverCombo->currentText().toStdString();
    _config.integrator = _integratorCombo->currentText().toStdString();
    _config.performanceProfile = _performanceCombo->currentText().toStdString();
    _config.simulationProfile = _simulationProfileCombo->currentText().toStdString();
    _config.presetStructure = _presetCombo->currentText().toStdString();
    _config.sphEnabled = _sphCheck->isChecked();
    _config.dt = static_cast<float>(_dtSpin->value());
    _config.octreeTheta = static_cast<float>(_thetaSpin->value());
    _config.octreeSoftening = static_cast<float>(_softeningSpin->value());
    _config.sphSmoothingLength = static_cast<float>(_sphSmoothingSpin->value());
    _config.sphRestDensity = static_cast<float>(_sphRestDensitySpin->value());
    _config.sphGasConstant = static_cast<float>(_sphGasConstantSpin->value());
    _config.sphViscosity = static_cast<float>(_sphViscositySpin->value());
    _config.defaultZoom = static_cast<float>(_zoomSlider->value()) / 10.0f;
    _config.defaultLuminosity = _luminositySlider->value();
    _config.renderCullingEnabled = _cullingCheck->isChecked();
    _config.renderLODEnabled = _lodCheck->isChecked();
}

void MainWindow::applyPerformanceProfileToRuntime()
{
    _clientDrawCap = _controller.applyPerformanceProfile(_config, *_runtime);
    applyViewSettings();
}

void MainWindow::markConfigDirty(bool dirty)
{
    _configDirty = dirty;
    setWindowTitle(_configDirty ? "N-Body Qt Client *" : "N-Body Qt Client");
}

bool MainWindow::saveConfigToDisk()
{
    (void)refreshValidationReport(false);
    if (_configPath.empty()) {
        _configPath = "simulation.ini";
    }
    if (!_config.save(_configPath)) {
        std::cerr << "[qt] failed to save config: " << _configPath << "\n";
        return false;
    }
    markConfigDirty(false);
    std::cout << "[qt] config saved: " << _configPath << "\n";
    return true;
}

void MainWindow::saveWorkspacePreset()
{
    bool accepted = false;
    const QString rawName = QInputDialog::getText(
        this,
        "Save Workspace",
        "Preset name",
        QLineEdit::Normal,
        "default",
        &accepted);
    if (!accepted || rawName.trimmed().isEmpty()) {
        return;
    }
    const bool saved = _workspaceLayouts.savePreset(
        rawName.trimmed().toStdString(),
        saveState().toBase64().toStdString(),
        saveGeometry().toBase64().toStdString());
    _statusLabel->setText(saved
        ? QString("workspace saved: %1").arg(rawName.trimmed())
        : QString("failed to save workspace"));
    statusBar()->showMessage(saved
        ? QString("Workspace saved: %1").arg(rawName.trimmed())
        : QString("Failed to save workspace"), 3000);
}

void MainWindow::loadWorkspacePreset()
{
    const std::vector<std::string> presets = _workspaceLayouts.listPresets();
    if (presets.empty()) {
        _statusLabel->setText("no saved workspace preset");
        statusBar()->showMessage("No saved workspace preset", 3000);
        return;
    }
    QStringList items;
    for (const std::string &preset : presets) {
        items.push_back(QString::fromStdString(preset));
    }
    bool accepted = false;
    const QString selected = QInputDialog::getItem(
        this,
        "Load Workspace",
        "Preset",
        items,
        0,
        false,
        &accepted);
    if (!accepted || selected.isEmpty()) {
        return;
    }
    std::string state;
    std::string geometry;
    if (!_workspaceLayouts.loadPreset(selected.toStdString(), state, geometry)) {
        _statusLabel->setText("failed to load workspace");
        statusBar()->showMessage("Failed to load workspace", 3000);
        return;
    }
    const bool geometryRestored = restoreGeometry(QByteArray::fromBase64(QByteArray::fromStdString(geometry)));
    const bool stateRestored = restoreState(QByteArray::fromBase64(QByteArray::fromStdString(state)));
    _statusLabel->setText((geometryRestored && stateRestored)
        ? QString("workspace loaded: %1").arg(selected)
        : QString("workspace preset invalid"));
    statusBar()->showMessage((geometryRestored && stateRestored)
        ? QString("Workspace loaded: %1").arg(selected)
        : QString("Workspace preset invalid"), 3000);
}

void MainWindow::deleteWorkspacePreset()
{
    const std::vector<std::string> presets = _workspaceLayouts.listPresets();
    if (presets.empty()) {
        _statusLabel->setText("no workspace preset to delete");
        statusBar()->showMessage("No workspace preset to delete", 3000);
        return;
    }
    QStringList items;
    for (const std::string &preset : presets) {
        items.push_back(QString::fromStdString(preset));
    }
    bool accepted = false;
    const QString selected = QInputDialog::getItem(
        this,
        "Delete Workspace",
        "Preset",
        items,
        0,
        false,
        &accepted);
    if (!accepted || selected.isEmpty()) {
        return;
    }
    const bool deleted = _workspaceLayouts.deletePreset(selected.toStdString());
    _statusLabel->setText(deleted
        ? QString("workspace deleted: %1").arg(selected)
        : QString("failed to delete workspace"));
    statusBar()->showMessage(deleted
        ? QString("Workspace deleted: %1").arg(selected)
        : QString("Failed to delete workspace"), 3000);
}

void MainWindow::restoreDefaultWorkspace()
{
    const bool geometryRestored = restoreGeometry(_defaultWorkspaceGeometry);
    const bool stateRestored = restoreState(_defaultWorkspaceState);
    _statusLabel->setText((geometryRestored && stateRestored)
        ? QString("default workspace restored")
        : QString("failed to restore default workspace"));
    statusBar()->showMessage((geometryRestored && stateRestored)
        ? QString("Default workspace restored")
        : QString("Failed to restore default workspace"), 3000);
}

bool MainWindow::refreshValidationReport(bool blockOnErrors)
{
    const grav_config::ScenarioValidationReport report = _controller.validate(_config);
    const ThroughputAdvisory advisory = ThroughputAdvisor::evaluate(_config, _clientDrawCap);
    if (_validationLabel != nullptr) {
        _validationLabel->setText(buildValidationText(report, advisory));
    }
    if (blockOnErrors && !report.validForRun && _statusLabel != nullptr) {
        _statusLabel->setText(QString("preflight validation failed; fix config errors"));
    }
    return !blockOnErrors || report.validForRun;
}

QString MainWindow::buildValidationText(
    const grav_config::ScenarioValidationReport &report,
    const ThroughputAdvisory &advisory) const
{
    std::string text = grav_config::SimulationScenarioValidation::renderText(report);
    if (advisory.severity != ThroughputAdvisorySeverity::None) {
        text += "\n\n[" + std::string(advisory.severity == ThroughputAdvisorySeverity::Warning ? "throughput-warning" : "throughput-advisory") + "] ";
        text += advisory.summary;
        if (!advisory.action.empty()) {
            text += "\nAction: " + advisory.action;
        }
    }
    return QString::fromStdString(text);
}

void MainWindow::showThroughputAdvisory(const ThroughputAdvisory &advisory)
{
    if (advisory.severity == ThroughputAdvisorySeverity::None) {
        return;
    }
    std::cout << "[qt] " << advisory.statusBarText << "\n";
    statusBar()->showMessage(QString::fromStdString(advisory.statusBarText), 6000);
}

void MainWindow::update3DCameraFromSliders()
{
    constexpr float pi = 3.14159265359f;
    const float yaw = static_cast<float>(_yawSlider->value()) * pi / 180.0f;
    const float pitch = static_cast<float>(_pitchSlider->value()) * pi / 180.0f;
    const float roll = static_cast<float>(_rollSlider->value()) * pi / 180.0f;
    _multiView->set3DCameraAngles(yaw, pitch, roll);
}

void MainWindow::tick()
{
    const auto uiNow = std::chrono::steady_clock::now();
    if (_lastUiTickAt.time_since_epoch().count() != 0) {
        const std::chrono::duration<float> uiDt = uiNow - _lastUiTickAt;
        if (uiDt.count() > 1e-6f) {
            const float inst = 1.0f / uiDt.count();
            _uiTickFps = (_uiTickFps <= 0.0f) ? inst : (0.9f * _uiTickFps + 0.1f * inst);
        }
    }
    _lastUiTickAt = uiNow;

    std::vector<RenderParticle> snapshot;
    const SimulationStats stats = _runtime->getCachedStats();
    if (!_configDirty && _solverCombo && !stats.solverName.empty()) {
        const QString solverText = QString::fromStdString(stats.solverName);
        const int solverIndex = _solverCombo->findText(solverText);
        if (solverIndex >= 0 && _solverCombo->currentIndex() != solverIndex) {
            _solverCombo->blockSignals(true);
            _solverCombo->setCurrentIndex(solverIndex);
            _solverCombo->blockSignals(false);
            _config.solver = stats.solverName;
        }
    }
    if (!_configDirty && _integratorCombo && !stats.integratorName.empty()) {
        const QString integratorText = QString::fromStdString(stats.integratorName);
        const int integratorIndex = _integratorCombo->findText(integratorText);
        if (integratorIndex >= 0 && _integratorCombo->currentIndex() != integratorIndex) {
            _integratorCombo->blockSignals(true);
            _integratorCombo->setCurrentIndex(integratorIndex);
            _integratorCombo->blockSignals(false);
            _config.integrator = stats.integratorName;
        }
    }
    std::size_t snapshotSize = 0u;
    std::uint32_t consumedSnapshotLatencyMs = std::numeric_limits<std::uint32_t>::max();
    std::optional<grav_client::ConsumedSnapshot> consumedSnapshot = _runtime->consumeLatestSnapshot();
    const bool gotSnapshot = consumedSnapshot.has_value();
    if (gotSnapshot) {
        snapshotSize = consumedSnapshot->sourceSize;
        consumedSnapshotLatencyMs = consumedSnapshot->latencyMs;
        snapshot = std::move(consumedSnapshot->particles);
    }
    const grav_client::SnapshotPipelineState snapshotPipeline = _runtime->snapshotPipelineState();
    const std::string linkLabel = _runtime->linkStateLabel();
    const std::string ownerLabel = _runtime->serverOwnerLabel();
    const std::uint32_t statsAgeMs = _runtime->statsAgeMs();
    const std::uint32_t snapshotAgeMs = _runtime->snapshotAgeMs();
    if (gotSnapshot) {
        _multiView->setSnapshot(std::move(snapshot));
    }
    const std::size_t displayedParticles = _multiView->displayedParticleCount();
    if (_lastEnergyStep != std::numeric_limits<std::uint64_t>::max() && stats.steps < _lastEnergyStep) {
        _energyGraph->clearHistory();
        _lastEnergyStep = std::numeric_limits<std::uint64_t>::max();
    }
    if (stats.steps != _lastEnergyStep) {
        _energyGraph->pushSample(stats);
        _lastEnergyStep = stats.steps;
    }
    MainWindowPresentationInput presentationInput;
    presentationInput.stats = stats;
    presentationInput.snapshotPipeline = snapshotPipeline;
    presentationInput.linkLabel = linkLabel;
    presentationInput.ownerLabel = ownerLabel;
    presentationInput.performanceProfile = stats.performanceProfile.empty() ? _config.performanceProfile : stats.performanceProfile;
    presentationInput.displayedParticles = displayedParticles;
    presentationInput.clientDrawCap = _clientDrawCap;
    presentationInput.statsAgeMs = statsAgeMs;
    presentationInput.snapshotAgeMs = snapshotAgeMs;
    presentationInput.snapshotLatencyMs = consumedSnapshotLatencyMs;
    presentationInput.uiTickFps = _uiTickFps;
    const MainWindowPresentation presentation = _presenter.present(presentationInput);

    _statusLabel->setText(QString::fromStdString(presentation.headlineText));
    _runtimeMetricsLabel->setText(QString::fromStdString(presentation.runtimeText));
    _queueMetricsLabel->setText(QString::fromStdString(presentation.queueText));
    _energyMetricsLabel->setText(QString::fromStdString(presentation.energyText));
    _gpuMetricsLabel->setText(QString::fromStdString(presentation.gpuText));

    _pauseButton->blockSignals(true);
    _pauseButton->setChecked(stats.paused);
    _pauseButton->setText(stats.paused ? "Resume" : "Pause");
    _pauseButton->blockSignals(false);
    _recoverButton->setEnabled(stats.faulted);

    static auto lastConsoleTrace = std::chrono::steady_clock::now();
    const auto now = std::chrono::steady_clock::now();
    if (now - lastConsoleTrace >= std::chrono::seconds(1)) {
        std::cout << presentation.consoleTrace << " snapshot=" << snapshotSize << "\n";
        lastConsoleTrace = now;
    }
}

} // namespace grav_qt
