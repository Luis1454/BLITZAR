/*
 * @file modules/qt/src/window/layout/State.cpp
 * @brief Helper definitions for Window control initialization.
 */

#include "Constants.hpp"
#include "widgets/graphs/Graph.hpp"
#include "widgets/graphs/SpectrumGraph.hpp"
#include "widgets/viewport/MultiView.hpp"
#include "window/core/Window.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <algorithm>
#include <limits>

namespace bltzr_qt {

void Window::initializeControlState()
{
    initializeComboBoxes();
    initializeObjectNames();
    initializeSpinAndSliderValues();
    initializeLabelsAndTooltips();
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
    if (_widgets.physics.particleCountSpin)
        _widgets.physics.particleCountSpin->setObjectName("particleCountSpin");
    if (_widgets.physics.treePmEnabledCheck)
        _widgets.physics.treePmEnabledCheck->setObjectName("treePmEnabledCheck");
    if (_widgets.physics.treePmPresetCombo)
        _widgets.physics.treePmPresetCombo->setObjectName("treePmPresetCombo");
    if (_widgets.physics.treePmModelCombo)
        _widgets.physics.treePmModelCombo->setObjectName("treePmModelCombo");
    if (_widgets.physics.treePmLayoutCombo)
        _widgets.physics.treePmLayoutCombo->setObjectName("treePmLayoutCombo");
    if (_widgets.physics.treePmPrecisionCombo)
        _widgets.physics.treePmPrecisionCombo->setObjectName("treePmPrecisionCombo");
    if (_widgets.physics.treePmAssignmentCombo)
        _widgets.physics.treePmAssignmentCombo->setObjectName("treePmAssignmentCombo");
    if (_widgets.physics.treePmLocalGridCheck)
        _widgets.physics.treePmLocalGridCheck->setObjectName("treePmLocalGridCheck");
    if (_widgets.physics.treePmGridSizeSpin)
        _widgets.physics.treePmGridSizeSpin->setObjectName("treePmGridSizeSpin");
    if (_widgets.physics.treePmJacobiIterationsSpin)
        _widgets.physics.treePmJacobiIterationsSpin->setObjectName("treePmJacobiIterationsSpin");
    if (_widgets.physics.treePmCutoffFactorSpin)
        _widgets.physics.treePmCutoffFactorSpin->setObjectName("treePmCutoffFactorSpin");
    if (_widgets.physics.treePmMaxLocalNeighborsSpin)
        _widgets.physics.treePmMaxLocalNeighborsSpin->setObjectName("treePmMaxLocalNeighborsSpin");
    if (_widgets.physics.treePmParticleLimitSpin)
        _widgets.physics.treePmParticleLimitSpin->setObjectName("treePmParticleLimitSpin");
    if (_widgets.physics.treePmDenseCellThresholdSpin)
        _widgets.physics.treePmDenseCellThresholdSpin->setObjectName(
            "treePmDenseCellThresholdSpin");
    if (_widgets.physics.treePmGravityOnlyBuffersCheck)
        _widgets.physics.treePmGravityOnlyBuffersCheck->setObjectName(
            "treePmGravityOnlyBuffersCheck");
    if (_widgets.physics.adaptiveTimeStepsCheck)
        _widgets.physics.adaptiveTimeStepsCheck->setObjectName("adaptiveTimeStepsCheck");
    if (_widgets.physics.adaptiveMaxLevelSpin)
        _widgets.physics.adaptiveMaxLevelSpin->setObjectName("adaptiveMaxLevelSpin");
    if (_widgets.physics.adaptiveEtaSpin)
        _widgets.physics.adaptiveEtaSpin->setObjectName("adaptiveEtaSpin");
    if (_widgets.physics.adaptiveCostGuardCheck)
        _widgets.physics.adaptiveCostGuardCheck->setObjectName("adaptiveCostGuardCheck");
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
    if (_widgets.view.spectrumGraph)
        _widgets.view.spectrumGraph->setObjectName("spectrumGraphWidget");
    if (_widgets.render.gpuTelemetryCheck)
        _widgets.render.gpuTelemetryCheck->setObjectName("gpuTelemetryCheck");
    if (_widgets.view.multiView)
        _widgets.view.multiView->setObjectName("multiViewWidget");
}

void Window::initializeSpinAndSliderValues()
{
    _widgets.physics.particleCountSpin->setRange(2, std::numeric_limits<int>::max());
    _widgets.physics.particleCountSpin->setSingleStep(1000);
    _widgets.physics.particleCountSpin->setValue(static_cast<int>(std::min<std::uint32_t>(
        _config.particleCount, static_cast<std::uint32_t>(std::numeric_limits<int>::max()))));
    _widgets.physics.dtSpin->setDecimals(5);
    _widgets.physics.dtSpin->setRange(kUiSimulationDtMin, kMaxStableInteractiveDt);
    _widgets.physics.dtSpin->setSingleStep(0.001);
    _widgets.physics.dtSpin->setValue(
        std::clamp(_config.dt, kUiSimulationDtMin, kMaxStableInteractiveDt));
    _widgets.physics.treePmEnabledCheck->setChecked(_config.treePmEnabled);
    _widgets.physics.treePmLocalGridCheck->setChecked(_config.treePmLocalGrid);
    _widgets.physics.treePmGridSizeSpin->setRange(16, 256);
    _widgets.physics.treePmGridSizeSpin->setSingleStep(16);
    _widgets.physics.treePmGridSizeSpin->setValue(
        static_cast<int>(std::clamp(_config.treePmGridSize, 16u, 256u)));
    _widgets.physics.treePmJacobiIterationsSpin->setRange(0, 128);
    _widgets.physics.treePmJacobiIterationsSpin->setValue(
        static_cast<int>(std::min(_config.treePmJacobiIterations, 128u)));
    _widgets.physics.treePmCutoffFactorSpin->setDecimals(3);
    _widgets.physics.treePmCutoffFactorSpin->setRange(0.0, 8.0);
    _widgets.physics.treePmCutoffFactorSpin->setSingleStep(0.1);
    _widgets.physics.treePmCutoffFactorSpin->setValue(
        std::clamp(_config.treePmCutoffFactor, 0.0f, 8.0f));
    _widgets.physics.treePmMaxLocalNeighborsSpin->setRange(0, 256);
    _widgets.physics.treePmMaxLocalNeighborsSpin->setValue(
        static_cast<int>(std::min(_config.treePmMaxLocalNeighbors, 256u)));
    _widgets.physics.treePmParticleLimitSpin->setRange(0, 100000000);
    _widgets.physics.treePmParticleLimitSpin->setValue(
        static_cast<int>(std::min(_config.treePmParticleLimit, 100000000u)));
    _widgets.physics.treePmDenseCellThresholdSpin->setRange(1, 4096);
    _widgets.physics.treePmDenseCellThresholdSpin->setValue(
        static_cast<int>(std::min(_config.treePmDenseCellThreshold, 4096u)));
    _widgets.physics.treePmGravityOnlyBuffersCheck->setChecked(_config.treePmGravityOnlyBuffers);
    _widgets.physics.adaptiveTimeStepsCheck->setChecked(_config.adaptiveTimeStepsEnabled);
    _widgets.physics.adaptiveMaxLevelSpin->setRange(0, 12);
    _widgets.physics.adaptiveMaxLevelSpin->setValue(
        static_cast<int>(std::min<std::uint32_t>(12u, _config.adaptiveTimeStepMaxLevel)));
    _widgets.physics.adaptiveEtaSpin->setDecimals(3);
    _widgets.physics.adaptiveEtaSpin->setRange(0.01, 1.0);
    _widgets.physics.adaptiveEtaSpin->setSingleStep(0.05);
    _widgets.physics.adaptiveEtaSpin->setValue(
        std::clamp(_config.adaptiveTimeStepEta, 0.01f, 1.0f));
    _widgets.physics.adaptiveCostGuardCheck->setChecked(_config.adaptiveTimeStepCostGuard);
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
    _widgets.physics.sphViscositySpin->setValue(std::max(kSphViscosityMin, _config.sphViscosity));
    _widgets.render.zoomSlider->setRange(kZoomSliderMin, kZoomSliderMax);
    _widgets.render.zoomSlider->setValue(zoomToSliderValue(_config.defaultZoom));
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

} // namespace bltzr_qt
