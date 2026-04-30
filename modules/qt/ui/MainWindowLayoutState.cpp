/*
 * @file modules/qt/ui/MainWindowLayoutState.cpp
 * @brief Helper definitions for MainWindow control initialization.
 */

#include "ui/EnergyGraphWidget.hpp"
#include "ui/MainWindow.hpp"
#include "ui/MultiViewWidget.hpp"
#include "ui/UiEnums.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QStringList>
#include <algorithm>

namespace bltzr_qt {

static const QStringList kSolverList = {QString::fromStdString(to_string(Solver::PairwiseCuda)),
                                        QString::fromStdString(to_string(Solver::OctreeGpu)),
                                        QString::fromStdString(to_string(Solver::OctreeCpu))};
static const QStringList kIntegratorList = {QString::fromStdString(to_string(Integrator::Euler)),
                                            QString::fromStdString(to_string(Integrator::Rk4))};
static const QStringList kPerformanceList = {
    QString::fromStdString(to_string(PerformanceProfile::Interactive)),
    QString::fromStdString(to_string(PerformanceProfile::Balanced)),
    QString::fromStdString(to_string(PerformanceProfile::Quality)),
    QString::fromStdString(to_string(PerformanceProfile::Custom))};
static const QStringList kSimulationProfiles = {"disk_orbit",  "galaxy_collision", "plummer_sphere",
                                                "binary_star", "solar_system",     "sph_collapse"};
static const QStringList kPresets = {"disk_orbit", "galaxy_collision", "random_cloud", "two_body",
                                     "three_body", "plummer_sphere",   "file"};
static const QStringList kView3dModes = {"perspective", "iso"};

void MainWindow::initializeControlState()
{
    initializeComboBoxes();
    initializeObjectNames();
    initializeSpinAndSliderValues();
    initializeLabelsAndTooltips();
}

void MainWindow::initializeComboBoxes()
{
    _pauseButton->setCheckable(true);
    _solverCombo->addItems(kSolverList);
    _solverCombo->setCurrentIndex(
        std::max(0, _solverCombo->findText(QString::fromStdString(_config.solver))));
    _integratorCombo->addItems(kIntegratorList);
    _integratorCombo->setCurrentIndex(
        std::max(0, _integratorCombo->findText(QString::fromStdString(_config.integrator))));
    _performanceCombo->addItems(kPerformanceList);
    _performanceCombo->setCurrentIndex(std::max(
        0, _performanceCombo->findText(QString::fromStdString(_config.performanceProfile))));
    _simulationProfileCombo->addItems(kSimulationProfiles);
    _simulationProfileCombo->setCurrentIndex(std::max(
        0, _simulationProfileCombo->findText(QString::fromStdString(_config.simulationProfile))));
    _presetCombo->addItems(kPresets);
    _presetCombo->setCurrentIndex(
        std::max(0, _presetCombo->findText(QString::fromStdString(_config.presetStructure))));
    _view3dCombo->addItems(kView3dModes);
}

void MainWindow::initializeObjectNames()
{
    if (_performanceCombo)
        _performanceCombo->setObjectName("performanceProfileCombo");
    if (_simulationProfileCombo)
        _simulationProfileCombo->setObjectName("simulationProfileCombo");
    if (_presetCombo)
        _presetCombo->setObjectName("scenePresetCombo");
    if (_pauseButton)
        _pauseButton->setObjectName("pauseToggleButton");
    if (_stepButton)
        _stepButton->setObjectName("stepButton");
    if (_resetButton)
        _resetButton->setObjectName("resetButton");
    if (_recoverButton)
        _recoverButton->setObjectName("recoverButton");
    if (_applyConnectorButton)
        _applyConnectorButton->setObjectName("connectButton");
    if (_exportButton)
        _exportButton->setObjectName("exportSnapshotButton");
    if (_saveConfigButton)
        _saveConfigButton->setObjectName("saveConfigButton");
    if (_loadInputButton)
        _loadInputButton->setObjectName("loadInputButton");
    if (_applyPresetButton)
        _applyPresetButton->setObjectName("applyPresetButton");
    if (_loadPresetButton)
        _loadPresetButton->setObjectName("loadPresetButton");
    if (_serverAutostartCheck)
        _serverAutostartCheck->setObjectName("serverAutostartCheck");
    if (_sphCheck)
        _sphCheck->setObjectName("sphEnabledCheck");
    if (_cullingCheck)
        _cullingCheck->setObjectName("renderCullingCheck");
    if (_lodCheck)
        _lodCheck->setObjectName("renderLodCheck");
    if (_octreeOverlayCheck)
        _octreeOverlayCheck->setObjectName("octreeOverlayCheck");
    if (_octreeOverlayDepthSpin)
        _octreeOverlayDepthSpin->setObjectName("octreeOverlayDepthSpin");
    if (_octreeOverlayOpacitySpin)
        _octreeOverlayOpacitySpin->setObjectName("octreeOverlayOpacitySpin");
    if (_serverHostEdit)
        _serverHostEdit->setObjectName("serverHostEdit");
    if (_serverBinEdit)
        _serverBinEdit->setObjectName("serverBinaryEdit");
    if (_serverPortSpin)
        _serverPortSpin->setObjectName("serverPortSpin");
    if (_solverCombo)
        _solverCombo->setObjectName("solverCombo");
    if (_integratorCombo)
        _integratorCombo->setObjectName("integratorCombo");
    if (_view3dCombo)
        _view3dCombo->setObjectName("view3dModeCombo");
    if (_dtSpin)
        _dtSpin->setObjectName("dtSpin");
    if (_thetaSpin)
        _thetaSpin->setObjectName("octreeThetaSpin");
    if (_softeningSpin)
        _softeningSpin->setObjectName("octreeSofteningSpin");
    if (_sphSmoothingSpin)
        _sphSmoothingSpin->setObjectName("sphSmoothingSpin");
    if (_sphRestDensitySpin)
        _sphRestDensitySpin->setObjectName("sphRestDensitySpin");
    if (_sphGasConstantSpin)
        _sphGasConstantSpin->setObjectName("sphGasConstantSpin");
    if (_sphViscositySpin)
        _sphViscositySpin->setObjectName("sphViscositySpin");
    if (_zoomSlider)
        _zoomSlider->setObjectName("zoomSlider");
    if (_luminositySlider)
        _luminositySlider->setObjectName("luminositySlider");
    if (_yawSlider)
        _yawSlider->setObjectName("yawSlider");
    if (_pitchSlider)
        _pitchSlider->setObjectName("pitchSlider");
    if (_rollSlider)
        _rollSlider->setObjectName("rollSlider");
    if (_energyGraph)
        _energyGraph->setObjectName("energyGraphWidget");
    if (_gpuTelemetryCheck)
        _gpuTelemetryCheck->setObjectName("gpuTelemetryCheck");
    if (_multiView)
        _multiView->setObjectName("multiViewWidget");
}

void MainWindow::initializeSpinAndSliderValues()
{
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
}

void MainWindow::initializeLabelsAndTooltips()
{
    _serverHostEdit->setText("127.0.0.1");
    _serverPortSpin->setRange(1, 65535);
    _serverPortSpin->setValue(4545);
    _serverBinEdit->setPlaceholderText("blitzar-server(.exe)");
    _serverBinEdit->setToolTip("Path to the server executable used when autostart is enabled");
    _applyConnectorButton->setToolTip(
        "Apply host, port and server binary settings, then reconnect now");
    for (QLabel* label : {_validationLabel, _statusLabel, _runtimeMetricsLabel, _queueMetricsLabel,
                          _energyMetricsLabel, _gpuMetricsLabel}) {
        if (label) {
            label->setWordWrap(true);
            label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        }
    }
    if (_validationLabel) {
        _validationLabel->setObjectName("validationLabel");
        _validationLabel->setContentsMargins(6, 4, 6, 4);
    }
    if (_statusLabel)
        _statusLabel->setObjectName("runtimeSummaryValue");
    if (_runtimeMetricsLabel)
        _runtimeMetricsLabel->setObjectName("runtimeSummaryValue");
    if (_queueMetricsLabel)
        _queueMetricsLabel->setObjectName("runtimeSummaryValue");
    if (_energyMetricsLabel)
        _energyMetricsLabel->setObjectName("runtimeSummaryValue");
    if (_gpuMetricsLabel)
        _gpuMetricsLabel->setObjectName("runtimeSummaryValue");
}

} // namespace bltzr_qt
