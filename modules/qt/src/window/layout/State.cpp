/*
 * @file modules/qt/src/window/layout/State.cpp
 * @brief Helper definitions for Window control initialization.
 */

#include "Constants.hpp"
#include "widgets/graphs/Graph.hpp"
#include "window/core/Window.hpp"
#include "widgets/viewport/MultiView.hpp"
#include "support/types/Enums.hpp"
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

void Window::initializeControlState()
{
    initializeComboBoxes();
    initializeObjectNames();
    initializeSpinAndSliderValues();
    initializeLabelsAndTooltips();
}

void Window::initializeComboBoxes()
{
    _widgets.run.pauseButton->setCheckable(true);
    _widgets.physics.solverCombo->addItems(kSolverList);
    _widgets.physics.solverCombo->setCurrentIndex(
        std::max(0, _widgets.physics.solverCombo->findText(QString::fromStdString(_config.solver))));
    _widgets.physics.integratorCombo->addItems(kIntegratorList);
    _widgets.physics.integratorCombo->setCurrentIndex(
        std::max(0, _widgets.physics.integratorCombo->findText(QString::fromStdString(_config.integrator))));
    _widgets.run.performanceCombo->addItems(kPerformanceList);
    _widgets.run.performanceCombo->setCurrentIndex(std::max(
        0, _widgets.run.performanceCombo->findText(QString::fromStdString(_config.performanceProfile))));
    _widgets.scene.simulationProfileCombo->addItems(kSimulationProfiles);
    _widgets.scene.simulationProfileCombo->setCurrentIndex(std::max(
        0, _widgets.scene.simulationProfileCombo->findText(QString::fromStdString(_config.simulationProfile))));
    _widgets.scene.presetCombo->addItems(kPresets);
    _widgets.scene.presetCombo->setCurrentIndex(
        std::max(0, _widgets.scene.presetCombo->findText(QString::fromStdString(_config.presetStructure))));
    _widgets.render.view3dCombo->addItems(kView3dModes);
}

void Window::initializeObjectNames()
{
    if (_widgets.run.performanceCombo)
        _widgets.run.performanceCombo->setObjectName("performanceProfileCombo");
    if (_widgets.scene.simulationProfileCombo)
        _widgets.scene.simulationProfileCombo->setObjectName("simulationProfileCombo");
    if (_widgets.scene.presetCombo)
        _widgets.scene.presetCombo->setObjectName("scenePresetCombo");
    if (_widgets.run.pauseButton)
        _widgets.run.pauseButton->setObjectName("pauseToggleButton");
    if (_widgets.run.stepButton)
        _widgets.run.stepButton->setObjectName("stepButton");
    if (_widgets.run.resetButton)
        _widgets.run.resetButton->setObjectName("resetButton");
    if (_widgets.run.recoverButton)
        _widgets.run.recoverButton->setObjectName("recoverButton");
    if (_widgets.run.applyConnectorButton)
        _widgets.run.applyConnectorButton->setObjectName("connectButton");
    if (_widgets.scene.exportButton)
        _widgets.scene.exportButton->setObjectName("exportSnapshotButton");
    if (_widgets.scene.saveConfigButton)
        _widgets.scene.saveConfigButton->setObjectName("saveConfigButton");
    if (_widgets.scene.loadInputButton)
        _widgets.scene.loadInputButton->setObjectName("loadInputButton");
    if (_widgets.scene.applyPresetButton)
        _widgets.scene.applyPresetButton->setObjectName("applyPresetButton");
    if (_widgets.scene.loadPresetButton)
        _widgets.scene.loadPresetButton->setObjectName("loadPresetButton");
    if (_widgets.run.serverAutostartCheck)
        _widgets.run.serverAutostartCheck->setObjectName("serverAutostartCheck");
    if (_widgets.physics.sphCheck)
        _widgets.physics.sphCheck->setObjectName("sphEnabledCheck");
    if (_widgets.render.cullingCheck)
        _widgets.render.cullingCheck->setObjectName("renderCullingCheck");
    if (_widgets.render.lodCheck)
        _widgets.render.lodCheck->setObjectName("renderLodCheck");
    if (_widgets.render.octreeOverlayCheck)
        _widgets.render.octreeOverlayCheck->setObjectName("octreeOverlayCheck");
    if (_widgets.render.octreeOverlayDepthSpin)
        _widgets.render.octreeOverlayDepthSpin->setObjectName("octreeOverlayDepthSpin");
    if (_widgets.render.octreeOverlayOpacitySpin)
        _widgets.render.octreeOverlayOpacitySpin->setObjectName("octreeOverlayOpacitySpin");
    if (_widgets.run.serverHostEdit)
        _widgets.run.serverHostEdit->setObjectName("serverHostEdit");
    if (_widgets.run.serverBinEdit)
        _widgets.run.serverBinEdit->setObjectName("serverBinaryEdit");
    if (_widgets.run.serverPortSpin)
        _widgets.run.serverPortSpin->setObjectName("serverPortSpin");
    if (_widgets.physics.solverCombo)
        _widgets.physics.solverCombo->setObjectName("solverCombo");
    if (_widgets.physics.integratorCombo)
        _widgets.physics.integratorCombo->setObjectName("integratorCombo");
    if (_widgets.render.view3dCombo)
        _widgets.render.view3dCombo->setObjectName("view3dModeCombo");
    if (_widgets.physics.dtSpin)
        _widgets.physics.dtSpin->setObjectName("dtSpin");
    if (_widgets.physics.thetaSpin)
        _widgets.physics.thetaSpin->setObjectName("octreeThetaSpin");
    if (_widgets.physics.softeningSpin)
        _widgets.physics.softeningSpin->setObjectName("octreeSofteningSpin");
    if (_widgets.physics.sphSmoothingSpin)
        _widgets.physics.sphSmoothingSpin->setObjectName("sphSmoothingSpin");
    if (_widgets.physics.sphRestDensitySpin)
        _widgets.physics.sphRestDensitySpin->setObjectName("sphRestDensitySpin");
    if (_widgets.physics.sphGasConstantSpin)
        _widgets.physics.sphGasConstantSpin->setObjectName("sphGasConstantSpin");
    if (_widgets.physics.sphViscositySpin)
        _widgets.physics.sphViscositySpin->setObjectName("sphViscositySpin");
    if (_widgets.render.zoomSlider)
        _widgets.render.zoomSlider->setObjectName("zoomSlider");
    if (_widgets.render.luminositySlider)
        _widgets.render.luminositySlider->setObjectName("luminositySlider");
    if (_widgets.render.yawSlider)
        _widgets.render.yawSlider->setObjectName("yawSlider");
    if (_widgets.render.pitchSlider)
        _widgets.render.pitchSlider->setObjectName("pitchSlider");
    if (_widgets.render.rollSlider)
        _widgets.render.rollSlider->setObjectName("rollSlider");
    if (_widgets.view.energyGraph)
        _widgets.view.energyGraph->setObjectName("energyGraphWidget");
    if (_widgets.render.gpuTelemetryCheck)
        _widgets.render.gpuTelemetryCheck->setObjectName("gpuTelemetryCheck");
    if (_widgets.view.multiView)
        _widgets.view.multiView->setObjectName("multiViewWidget");
}

void Window::initializeSpinAndSliderValues()
{
    _widgets.physics.dtSpin->setDecimals(5);
    _widgets.physics.dtSpin->setRange(kUiSimulationDtMin, kMaxStableInteractiveDt);
    _widgets.physics.dtSpin->setSingleStep(0.001);
    _widgets.physics.dtSpin->setValue(
        std::clamp(_config.dt, kUiSimulationDtMin, kMaxStableInteractiveDt));
    _widgets.physics.thetaSpin->setDecimals(3);
    _widgets.physics.thetaSpin->setRange(kPhysicsMinTheta, kPhysicsMaxTheta);
    _widgets.physics.thetaSpin->setSingleStep(0.05);
    _widgets.physics.thetaSpin->setValue(
        std::clamp(_config.octreeTheta, kPhysicsMinTheta, kPhysicsMaxTheta));
    _widgets.physics.softeningSpin->setDecimals(4);
    _widgets.physics.softeningSpin->setRange(kPhysicsMinSofteningDefault, kOctreeSofteningMax);
    _widgets.physics.softeningSpin->setSingleStep(0.01);
    _widgets.physics.softeningSpin->setValue(
        std::clamp(_config.octreeSoftening, kPhysicsMinSofteningDefault, kOctreeSofteningMax));
    _widgets.physics.sphCheck->setChecked(_config.sphEnabled);
    _widgets.physics.sphSmoothingSpin->setDecimals(3);
    _widgets.physics.sphSmoothingSpin->setRange(kSphSmoothingMin, kSphSmoothingMax);
    _widgets.physics.sphSmoothingSpin->setSingleStep(0.05);
    _widgets.physics.sphSmoothingSpin->setValue(
        std::max(kSphSmoothingMin, _config.sphSmoothingLength));
    _widgets.physics.sphRestDensitySpin->setDecimals(3);
    _widgets.physics.sphRestDensitySpin->setRange(kSphRestDensityMin, kSphRestDensityMax);
    _widgets.physics.sphRestDensitySpin->setSingleStep(0.05);
    _widgets.physics.sphRestDensitySpin->setValue(
        std::max(kSphRestDensityMin, _config.sphRestDensity));
    _widgets.physics.sphGasConstantSpin->setDecimals(3);
    _widgets.physics.sphGasConstantSpin->setRange(kSphGasConstantMin, kSphGasConstantMax);
    _widgets.physics.sphGasConstantSpin->setSingleStep(0.1);
    _widgets.physics.sphGasConstantSpin->setValue(
        std::max(kSphGasConstantMin, _config.sphGasConstant));
    _widgets.physics.sphViscositySpin->setDecimals(4);
    _widgets.physics.sphViscositySpin->setRange(kSphViscosityMin, kSphViscosityMax);
    _widgets.physics.sphViscositySpin->setSingleStep(0.01);
    _widgets.physics.sphViscositySpin->setValue(
        std::max(kSphViscosityMin, _config.sphViscosity));
    _widgets.render.zoomSlider->setRange(kZoomSliderMin, kZoomSliderMax);
    _widgets.render.zoomSlider->setValue(static_cast<int>(std::clamp(
        _config.defaultZoom * kZoomSliderDivisor, static_cast<float>(kZoomSliderMin),
        static_cast<float>(kZoomSliderMax))));
    _widgets.render.luminositySlider->setRange(kLuminosityMin, kLuminosityMax);
    _widgets.render.luminositySlider->setValue(
        std::clamp(_config.defaultLuminosity, kLuminosityMin, kLuminosityMax));
    _widgets.render.yawSlider->setRange(-180, 180);
    _widgets.render.pitchSlider->setRange(-90, 90);
    _widgets.render.rollSlider->setRange(-180, 180);
    _widgets.render.octreeOverlayDepthSpin->setRange(0, kOverlayDepthMax);
    _widgets.render.octreeOverlayDepthSpin->setValue(kOverlayDepthDefault);
    _widgets.render.octreeOverlayOpacitySpin->setRange(kLuminosityMin, kLuminosityMax);
    _widgets.render.octreeOverlayOpacitySpin->setValue(kOverlayOpacityDefault);
    _widgets.render.cullingCheck->setChecked(_config.renderCullingEnabled);
    _widgets.render.lodCheck->setChecked(_config.renderLODEnabled);
}

void Window::initializeLabelsAndTooltips()
{
    _widgets.run.serverHostEdit->setText(kDefaultLoopbackHost);
    _widgets.run.serverPortSpin->setRange(kNetworkPortMin, kNetworkPortMax);
    _widgets.run.serverPortSpin->setValue(kDefaultServerPort);
    _widgets.run.serverBinEdit->setPlaceholderText("blitzar-server(.exe)");
    _widgets.run.serverBinEdit->setToolTip("Path to the server executable used when autostart is enabled");
    _widgets.run.applyConnectorButton->setToolTip(
        "Apply host, port and server binary settings, then reconnect now");
    for (QLabel* label : {_widgets.telemetry.validationLabel, _widgets.telemetry.statusLabel, _widgets.telemetry.runtimeMetricsLabel, _widgets.telemetry.queueMetricsLabel,
                          _widgets.telemetry.energyMetricsLabel, _widgets.telemetry.gpuMetricsLabel}) {
        if (label) {
            label->setWordWrap(true);
            label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        }
    }
    if (_widgets.telemetry.validationLabel) {
        _widgets.telemetry.validationLabel->setObjectName("validationLabel");
        _widgets.telemetry.validationLabel->setContentsMargins(6, 4, 6, 4);
    }
    if (_widgets.telemetry.statusLabel)
        _widgets.telemetry.statusLabel->setObjectName("runtimeSummaryValue");
    if (_widgets.telemetry.runtimeMetricsLabel)
        _widgets.telemetry.runtimeMetricsLabel->setObjectName("runtimeSummaryValue");
    if (_widgets.telemetry.queueMetricsLabel)
        _widgets.telemetry.queueMetricsLabel->setObjectName("runtimeSummaryValue");
    if (_widgets.telemetry.energyMetricsLabel)
        _widgets.telemetry.energyMetricsLabel->setObjectName("runtimeSummaryValue");
    if (_widgets.telemetry.gpuMetricsLabel)
        _widgets.telemetry.gpuMetricsLabel->setObjectName("runtimeSummaryValue");
}

} // namespace bltzr_qt
