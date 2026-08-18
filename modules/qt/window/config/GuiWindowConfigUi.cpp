/*
 * @file modules/qt/window/config/GuiWindowConfigUi.cpp
 * @brief Qt widget synchronization for the loaded simulation configuration.
 */

#include "window/config/GuiConfigurationEditor.hpp"
#include "window/core/GuiWindow.hpp"
#include "window/scene/GuiSceneEditor.hpp"
#include "widgets/viewport/GuiMultiView.hpp"
#include "core/constants/FndConstants.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <algorithm>
#include <limits>

namespace bltzr_qt {
void Window::applyConfigToUi()
{
    _widgets.physics.solverCombo->blockSignals(true);
    _widgets.physics.integratorCombo->blockSignals(true);
    _widgets.physics.particleCountSpin->blockSignals(true);
    _widgets.physics.treePmEnabledCheck->blockSignals(true);
    _widgets.physics.treePmPresetCombo->blockSignals(true);
    _widgets.physics.treePmModelCombo->blockSignals(true);
    _widgets.physics.treePmPrecisionCombo->blockSignals(true);
    _widgets.physics.treePmAssignmentCombo->blockSignals(true);
    _widgets.physics.treePmLocalGridCheck->blockSignals(true);
    _widgets.physics.treePmGridSizeSpin->blockSignals(true);
    _widgets.physics.treePmJacobiIterationsSpin->blockSignals(true);
    _widgets.physics.treePmCutoffFactorSpin->blockSignals(true);
    _widgets.physics.treePmMaxLocalNeighborsSpin->blockSignals(true);
    _widgets.physics.treePmParticleLimitSpin->blockSignals(true);
    _widgets.physics.treePmDenseCellThresholdSpin->blockSignals(true);
    _widgets.physics.treePmGravityOnlyBuffersCheck->blockSignals(true);
    _widgets.physics.adaptiveTimeStepsCheck->blockSignals(true);
    _widgets.physics.adaptiveMaxLevelSpin->blockSignals(true);
    _widgets.physics.adaptiveEtaSpin->blockSignals(true);
    _widgets.physics.adaptiveCostGuardCheck->blockSignals(true);
    _widgets.run.performanceCombo->blockSignals(true);
    _widgets.scene.simulationProfileCombo->blockSignals(true);
    _widgets.scene.presetCombo->blockSignals(true);
    _widgets.physics.sphCheck->blockSignals(true);
    _widgets.physics.dtSpin->blockSignals(true);
    _widgets.physics.thetaSpin->blockSignals(true);
    _widgets.physics.softeningSpin->blockSignals(true);
    _widgets.physics.sphSmoothingSpin->blockSignals(true);
    _widgets.physics.sphRestDensitySpin->blockSignals(true);
    _widgets.physics.sphGasConstantSpin->blockSignals(true);
    _widgets.physics.sphViscositySpin->blockSignals(true);
    _widgets.render.zoomSlider->blockSignals(true);
    _widgets.render.luminositySlider->blockSignals(true);
    _widgets.render.cullingCheck->blockSignals(true);
    _widgets.render.lodCheck->blockSignals(true);
    _widgets.physics.solverCombo->setCurrentIndex(
        std::max(0, _widgets.physics.solverCombo->findText(QString::fromStdString(_config.solver))));
    _widgets.physics.integratorCombo->setCurrentIndex(
        std::max(0, _widgets.physics.integratorCombo->findText(QString::fromStdString(_config.integrator))));
    _widgets.physics.particleCountSpin->setValue(static_cast<int>(std::min<std::uint32_t>(
        _config.particleCount, static_cast<std::uint32_t>(std::numeric_limits<int>::max()))));
    _widgets.physics.treePmEnabledCheck->setChecked(_config.treePmEnabled);
    _widgets.physics.treePmPresetCombo->setCurrentIndex(std::max(
        0, _widgets.physics.treePmPresetCombo->findText(
               QString::fromStdString(_config.treePmPreset))));
    _widgets.physics.treePmModelCombo->setCurrentIndex(std::max(
        0, _widgets.physics.treePmModelCombo->findText(
               QString::fromStdString(_config.treePmModel))));
    _widgets.physics.treePmPrecisionCombo->setCurrentIndex(std::max(
        0, _widgets.physics.treePmPrecisionCombo->findText(
               QString::fromStdString(_config.treePmPrecision))));
    _widgets.physics.treePmAssignmentCombo->setCurrentIndex(std::max(
        0, _widgets.physics.treePmAssignmentCombo->findText(
               QString::fromStdString(_config.treePmAssignment))));
    _widgets.physics.treePmLocalGridCheck->setChecked(_config.treePmLocalGrid);
    _widgets.physics.treePmGridSizeSpin->setValue(static_cast<int>(_config.treePmGridSize));
    _widgets.physics.treePmJacobiIterationsSpin->setValue(
        static_cast<int>(_config.treePmJacobiIterations));
    _widgets.physics.treePmCutoffFactorSpin->setValue(_config.treePmCutoffFactor);
    _widgets.physics.treePmMaxLocalNeighborsSpin->setValue(
        static_cast<int>(_config.treePmMaxLocalNeighbors));
    _widgets.physics.treePmParticleLimitSpin->setValue(static_cast<int>(_config.treePmParticleLimit));
    _widgets.physics.treePmDenseCellThresholdSpin->setValue(
        static_cast<int>(_config.treePmDenseCellThreshold));
    _widgets.physics.treePmGravityOnlyBuffersCheck->setChecked(_config.treePmGravityOnlyBuffers);
    _widgets.physics.adaptiveTimeStepsCheck->setChecked(_config.adaptiveTimeStepsEnabled);
    _widgets.physics.adaptiveMaxLevelSpin->setValue(
        static_cast<int>(std::min<std::uint32_t>(12u, _config.adaptiveTimeStepMaxLevel)));
    _widgets.physics.adaptiveEtaSpin->setValue(std::clamp(_config.adaptiveTimeStepEta, 0.01f, 1.0f));
    _widgets.physics.adaptiveCostGuardCheck->setChecked(_config.adaptiveTimeStepCostGuard);
    _widgets.run.performanceCombo->setCurrentIndex(std::max(
        0, _widgets.run.performanceCombo->findText(QString::fromStdString(_config.performanceProfile))));
    _widgets.scene.simulationProfileCombo->setCurrentIndex(std::max(
        0, _widgets.scene.simulationProfileCombo->findText(QString::fromStdString(_config.simulationProfile))));
    const int presetIndex =
        std::max(0, _widgets.scene.presetCombo->findText(QString::fromStdString(_config.presetStructure)));
    _widgets.scene.presetCombo->setCurrentIndex(presetIndex);
    _widgets.physics.sphCheck->setChecked(_config.sphEnabled);
    _widgets.physics.dtSpin->setValue(
        std::clamp(_config.dt, kUiSimulationDtMin, kMaxStableInteractiveDt));
    _widgets.physics.thetaSpin->setValue(
        std::clamp(_config.octreeTheta, kPhysicsMinTheta, kPhysicsMaxTheta));
    _widgets.physics.softeningSpin->setValue(
        std::clamp(_config.octreeSoftening, kPhysicsMinSofteningDefault, kOctreeSofteningMax));
    _widgets.physics.sphSmoothingSpin->setValue(
        std::max(kSphSmoothingMin, _config.sphSmoothingLength));
    _widgets.physics.sphRestDensitySpin->setValue(
        std::max(kSphRestDensityMin, _config.sphRestDensity));
    _widgets.physics.sphGasConstantSpin->setValue(
        std::max(kSphGasConstantMin, _config.sphGasConstant));
    _widgets.physics.sphViscositySpin->setValue(
        std::max(kSphViscosityMin, _config.sphViscosity));
    _widgets.render.zoomSlider->setValue(zoomToSliderValue(_config.defaultZoom));
    _widgets.render.luminositySlider->setValue(
        std::clamp(_config.defaultLuminosity, kLuminosityMin, kLuminosityMax));
    _widgets.render.cullingCheck->setChecked(_config.renderCullingEnabled);
    _widgets.render.lodCheck->setChecked(_config.renderLODEnabled);
    _widgets.physics.solverCombo->blockSignals(false);
    _widgets.physics.integratorCombo->blockSignals(false);
    _widgets.physics.particleCountSpin->blockSignals(false);
    _widgets.physics.treePmEnabledCheck->blockSignals(false);
    _widgets.physics.treePmPresetCombo->blockSignals(false);
    _widgets.physics.treePmModelCombo->blockSignals(false);
    _widgets.physics.treePmPrecisionCombo->blockSignals(false);
    _widgets.physics.treePmAssignmentCombo->blockSignals(false);
    _widgets.physics.treePmLocalGridCheck->blockSignals(false);
    _widgets.physics.treePmGridSizeSpin->blockSignals(false);
    _widgets.physics.treePmJacobiIterationsSpin->blockSignals(false);
    _widgets.physics.treePmCutoffFactorSpin->blockSignals(false);
    _widgets.physics.treePmMaxLocalNeighborsSpin->blockSignals(false);
    _widgets.physics.treePmParticleLimitSpin->blockSignals(false);
    _widgets.physics.treePmDenseCellThresholdSpin->blockSignals(false);
    _widgets.physics.treePmGravityOnlyBuffersCheck->blockSignals(false);
    _widgets.physics.adaptiveTimeStepsCheck->blockSignals(false);
    _widgets.physics.adaptiveMaxLevelSpin->blockSignals(false);
    _widgets.physics.adaptiveEtaSpin->blockSignals(false);
    _widgets.physics.adaptiveCostGuardCheck->blockSignals(false);
    _widgets.run.performanceCombo->blockSignals(false);
    _widgets.scene.simulationProfileCombo->blockSignals(false);
    _widgets.scene.presetCombo->blockSignals(false);
    _widgets.physics.sphCheck->blockSignals(false);
    _widgets.physics.dtSpin->blockSignals(false);
    _widgets.physics.thetaSpin->blockSignals(false);
    _widgets.physics.softeningSpin->blockSignals(false);
    _widgets.physics.sphSmoothingSpin->blockSignals(false);
    _widgets.physics.sphRestDensitySpin->blockSignals(false);
    _widgets.physics.sphGasConstantSpin->blockSignals(false);
    _widgets.physics.sphViscositySpin->blockSignals(false);
    _widgets.render.zoomSlider->blockSignals(false);
    _widgets.render.luminositySlider->blockSignals(false);
    _widgets.render.cullingCheck->blockSignals(false);
    _widgets.render.lodCheck->blockSignals(false);
    applyViewSettings();
    if (_configurationEditor != nullptr)
        _configurationEditor->reload(_config);
    if (_sceneEditor != nullptr)
        _sceneEditor->reload(_config);
}

void Window::captureUiIntoConfig()
{
    _config.solver = _widgets.physics.solverCombo->currentText().toStdString();
    _config.integrator = _widgets.physics.integratorCombo->currentText().toStdString();
    _config.particleCount = static_cast<std::uint32_t>(_widgets.physics.particleCountSpin->value());
    _config.treePmEnabled = _widgets.physics.treePmEnabledCheck->isChecked();
    _config.treePmPreset = _widgets.physics.treePmPresetCombo->currentText().toStdString();
    _config.treePmModel = _widgets.physics.treePmModelCombo->currentText().toStdString();
    _config.treePmPrecision = _widgets.physics.treePmPrecisionCombo->currentText().toStdString();
    _config.treePmAssignment =
        _widgets.physics.treePmAssignmentCombo->currentText().toStdString();
    _config.treePmLocalGrid = _widgets.physics.treePmLocalGridCheck->isChecked();
    _config.treePmGridSize = static_cast<std::uint32_t>(_widgets.physics.treePmGridSizeSpin->value());
    _config.treePmJacobiIterations = static_cast<std::uint32_t>(
        _widgets.physics.treePmJacobiIterationsSpin->value());
    _config.treePmCutoffFactor = static_cast<float>(_widgets.physics.treePmCutoffFactorSpin->value());
    _config.treePmMaxLocalNeighbors = static_cast<std::uint32_t>(
        _widgets.physics.treePmMaxLocalNeighborsSpin->value());
    _config.treePmParticleLimit = static_cast<std::uint32_t>(
        _widgets.physics.treePmParticleLimitSpin->value());
    _config.treePmDenseCellThreshold = static_cast<std::uint32_t>(
        _widgets.physics.treePmDenseCellThresholdSpin->value());
    _config.treePmGravityOnlyBuffers = _widgets.physics.treePmGravityOnlyBuffersCheck->isChecked();
    _config.adaptiveTimeStepsEnabled = _widgets.physics.adaptiveTimeStepsCheck->isChecked();
    _config.adaptiveTimeStepMaxLevel = static_cast<std::uint32_t>(
        _widgets.physics.adaptiveMaxLevelSpin->value());
    _config.adaptiveTimeStepEta = static_cast<float>(_widgets.physics.adaptiveEtaSpin->value());
    _config.adaptiveTimeStepCostGuard = _widgets.physics.adaptiveCostGuardCheck->isChecked();
    _config.performanceProfile = _widgets.run.performanceCombo->currentText().toStdString();
    _config.simulationProfile = _widgets.scene.simulationProfileCombo->currentText().toStdString();
    _config.presetStructure = _widgets.scene.presetCombo->currentText().toStdString();
    _config.sphEnabled = _widgets.physics.sphCheck->isChecked();
    _config.dt = static_cast<float>(_widgets.physics.dtSpin->value());
    _config.octreeTheta = static_cast<float>(_widgets.physics.thetaSpin->value());
    _config.octreeSoftening = static_cast<float>(_widgets.physics.softeningSpin->value());
    _config.sphSmoothingLength = static_cast<float>(_widgets.physics.sphSmoothingSpin->value());
    _config.sphRestDensity = static_cast<float>(_widgets.physics.sphRestDensitySpin->value());
    _config.sphGasConstant = static_cast<float>(_widgets.physics.sphGasConstantSpin->value());
    _config.sphViscosity = static_cast<float>(_widgets.physics.sphViscositySpin->value());
    _config.defaultZoom = zoomFromSliderValue(_widgets.render.zoomSlider->value());
    _config.defaultLuminosity = _widgets.render.luminositySlider->value();
    _config.renderCullingEnabled = _widgets.render.cullingCheck->isChecked();
    _config.renderLODEnabled = _widgets.render.lodCheck->isChecked();
    if (_sceneEditor != nullptr)
        _sceneEditor->applyToConfig(_config);
}
} // namespace bltzr_qt
