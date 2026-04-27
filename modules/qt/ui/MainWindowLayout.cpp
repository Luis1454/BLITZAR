// File: modules/qt/ui/MainWindowLayout.cpp
// Purpose: Client module implementation for BLITZAR extension workflows.

#include "ui/EnergyGraphWidget.hpp"
#include "ui/MainWindow.hpp"
#include "ui/MultiViewWidget.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSizePolicy>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>
namespace grav_qt {
/// Description: Executes the initializeControlState operation.
void MainWindow::initializeControlState()
{
    _pauseButton->setCheckable(true);
    _solverCombo->addItems({"pairwise_cuda", "octree_gpu", "octree_cpu"});
    _solverCombo->setCurrentIndex(
        /// Description: Executes the max operation.
        std::max(0, _solverCombo->findText(QString::fromStdString(_config.solver))));
    _integratorCombo->addItems({"euler", "rk4"});
    _integratorCombo->setCurrentIndex(
        /// Description: Executes the max operation.
        std::max(0, _integratorCombo->findText(QString::fromStdString(_config.integrator))));
    _performanceCombo->addItems({"interactive", "balanced", "quality", "custom"});
    _performanceCombo->setCurrentIndex(std::max(
        0, _performanceCombo->findText(QString::fromStdString(_config.performanceProfile))));
    _simulationProfileCombo->addItems({"disk_orbit", "galaxy_collision", "plummer_sphere",
                                       "binary_star", "solar_system", "sph_collapse"});
    _simulationProfileCombo->setCurrentIndex(std::max(
        0, _simulationProfileCombo->findText(QString::fromStdString(_config.simulationProfile))));
    _presetCombo->addItems({"disk_orbit", "galaxy_collision", "random_cloud", "two_body",
                            "three_body", "plummer_sphere", "file"});
    _presetCombo->setCurrentIndex(
        /// Description: Executes the max operation.
        std::max(0, _presetCombo->findText(QString::fromStdString(_config.presetStructure))));
    _view3dCombo->addItems({"perspective", "iso"});
    _performanceCombo->setObjectName("performanceProfileCombo");
    _simulationProfileCombo->setObjectName("simulationProfileCombo");
    _presetCombo->setObjectName("scenePresetCombo");
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
    _energyGraph->setObjectName("energyGraphWidget");
    _gpuTelemetryCheck->setObjectName("gpuTelemetryCheck");
    _multiView->setObjectName("multiViewWidget");
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
    _sphCheck->setChecked(_config.sphEnabled);
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
    _yawSlider->setRange(-180, 180);
    _pitchSlider->setRange(-90, 90);
    _rollSlider->setRange(-180, 180);
    _octreeOverlayDepthSpin->setRange(0, 8);
    _octreeOverlayDepthSpin->setValue(3);
    _octreeOverlayOpacitySpin->setRange(0, 255);
    _octreeOverlayOpacitySpin->setValue(96);
    _cullingCheck->setChecked(_config.renderCullingEnabled);
    _lodCheck->setChecked(_config.renderLODEnabled);
    _serverHostEdit->setText("127.0.0.1");
    _serverPortSpin->setRange(1, 65535);
    _serverPortSpin->setValue(4545);
    _serverBinEdit->setPlaceholderText("blitzar-server(.exe)");
    _serverBinEdit->setToolTip("Path to the server executable used when autostart is enabled");
    _applyConnectorButton->setToolTip(
        "Apply host, port and server binary settings, then reconnect now");
    for (QLabel* label : {_validationLabel, _statusLabel, _runtimeMetricsLabel, _queueMetricsLabel,
                          _energyMetricsLabel, _gpuMetricsLabel}) {
        label->setWordWrap(true);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    }
    _validationLabel->setObjectName("validationLabel");
    _validationLabel->setContentsMargins(6, 4, 6, 4);
    _statusLabel->setObjectName("runtimeSummaryValue");
    _runtimeMetricsLabel->setObjectName("runtimeSummaryValue");
    _queueMetricsLabel->setObjectName("runtimeSummaryValue");
    _energyMetricsLabel->setObjectName("runtimeSummaryValue");
    _gpuMetricsLabel->setObjectName("runtimeSummaryValue");
}
/// Description: Executes the buildSidebarTabs operation.
QTabWidget* MainWindow::buildSidebarTabs()
{
    auto* sidebarTabs = new QTabWidget(this);
    sidebarTabs->setObjectName("workspaceSidebarTabs");
    sidebarTabs->setTabPosition(QTabWidget::West);
    sidebarTabs->setDocumentMode(true);
    sidebarTabs->setMinimumWidth(220);
    sidebarTabs->setMaximumWidth(248);
    sidebarTabs->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    auto* runPage = new QWidget(sidebarTabs);
    auto* runLayout = new QVBoxLayout(runPage);
    auto* scenePage = new QWidget(sidebarTabs);
    auto* sceneLayout = new QVBoxLayout(scenePage);
    auto* physicsPage = new QWidget(sidebarTabs);
    auto* physicsLayout = new QVBoxLayout(physicsPage);
    auto* renderPage = new QWidget(sidebarTabs);
    auto* renderLayout = new QVBoxLayout(renderPage);
    for (QVBoxLayout* layout : {runLayout, sceneLayout, physicsLayout, renderLayout}) {
        layout->setContentsMargins(4, 4, 4, 4);
        layout->setSpacing(8);
    }
    auto* runBox = new QGroupBox("Run Control", runPage);
    auto* runBoxLayout = new QVBoxLayout(runBox);
    auto* runForm = new QFormLayout();
    runForm->addRow("performance", _performanceCombo);
    auto* runActions = new QGridLayout();
    runActions->addWidget(_pauseButton, 0, 0);
    runActions->addWidget(_stepButton, 0, 1);
    runActions->addWidget(_resetButton, 1, 0);
    runActions->addWidget(_recoverButton, 1, 1);
    runBoxLayout->addLayout(runForm);
    runBoxLayout->addLayout(runActions);
    auto* connectorBox = new QGroupBox("Connector", runPage);
    auto* connectorLayout = new QVBoxLayout(connectorBox);
    auto* connectorForm = new QFormLayout();
    connectorForm->addRow("host", _serverHostEdit);
    connectorForm->addRow("port", _serverPortSpin);
    connectorForm->addRow("server bin", _serverBinEdit);
    connectorLayout->addLayout(connectorForm);
    connectorLayout->addWidget(_serverAutostartCheck);
    connectorLayout->addWidget(_applyConnectorButton);
    connectorLayout->addStretch(1);
    auto* sceneBox = new QGroupBox("Scene Setup", scenePage);
    auto* sceneBoxLayout = new QVBoxLayout(sceneBox);
    auto* sceneForm = new QFormLayout();
    sceneForm->addRow("profile", _simulationProfileCombo);
    sceneForm->addRow("preset", _presetCombo);
    sceneBoxLayout->addLayout(sceneForm);
    sceneBoxLayout->addWidget(_applyPresetButton);
    sceneBoxLayout->addWidget(_loadPresetButton);
    sceneBoxLayout->addWidget(_loadInputButton);
    auto* projectBox = new QGroupBox("Project", scenePage);
    auto* projectLayout = new QVBoxLayout(projectBox);
    projectLayout->addWidget(_saveConfigButton);
    projectLayout->addStretch(1);
    auto* physicsCoreBox = new QGroupBox("Physics Core", physicsPage);
    auto* physicsCoreLayout = new QVBoxLayout(physicsCoreBox);
    auto* physicsForm = new QFormLayout();
    physicsForm->addRow("solver", _solverCombo);
    physicsForm->addRow("integrator", _integratorCombo);
    physicsForm->addRow("dt", _dtSpin);
    physicsForm->addRow("theta", _thetaSpin);
    physicsForm->addRow("softening", _softeningSpin);
    physicsCoreLayout->addLayout(physicsForm);
    auto* sphBox = new QGroupBox("SPH", physicsPage);
    auto* sphLayout = new QVBoxLayout(sphBox);
    auto* sphForm = new QFormLayout();
    sphForm->addRow("h", _sphSmoothingSpin);
    sphForm->addRow("rest density", _sphRestDensitySpin);
    sphForm->addRow("gas K", _sphGasConstantSpin);
    sphForm->addRow("viscosity", _sphViscositySpin);
    sphLayout->addWidget(_sphCheck);
    sphLayout->addLayout(sphForm);
    auto* cameraBox = new QGroupBox("View & Camera", renderPage);
    auto* cameraLayout = new QGridLayout(cameraBox);
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
    auto* overlayBox = new QGroupBox("Octree Overlay", renderPage);
    auto* overlayLayout = new QFormLayout(overlayBox);
    overlayLayout->addRow(_octreeOverlayCheck);
    overlayLayout->addRow("depth", _octreeOverlayDepthSpin);
    overlayLayout->addRow("opacity", _octreeOverlayOpacitySpin);
    auto* exportBox = new QGroupBox("Export", renderPage);
    auto* exportLayout = new QVBoxLayout(exportBox);
    exportLayout->addWidget(_exportButton);
    exportLayout->addWidget(_gpuTelemetryCheck);
    exportLayout->addStretch(1);
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
    return sidebarTabs;
}
} // namespace grav_qt
