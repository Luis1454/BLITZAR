/*
 * @file modules/qt/src/window/config/WindowConfig.cpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "config/validation/Scenario.hpp"
#include "window/core/Window.hpp"
#include "widgets/viewport/MultiView.hpp"
#include "Constants.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QSlider>
#include <QStatusBar>
#include <algorithm>
#include <iostream>
#include <string>

namespace bltzr_qt {
bool Window::applyConfigToServer(bool requestReset)
{
    captureUiIntoConfig();
    const ApplyConfigResult result =
        _controller.applyConfig(_config, *_runtime, requestReset);
    const Advisory advisory = Throughput::evaluate(_config, result.clientDrawCap);
    if (_widgets.telemetry.validationLabel != nullptr) {
        _widgets.telemetry.validationLabel->setText(buildValidationText(result.report, advisory));
    }
    showThroughputAdvisory(advisory);
    if (!result.report.validForRun) {
        if (_widgets.telemetry.statusLabel != nullptr) {
            _widgets.telemetry.statusLabel->setText(QString("preflight validation failed; fix config errors"));
        }
        return false;
    }
    _clientDrawCap = result.clientDrawCap;
    applyViewSettings();
    return true;
}

void Window::applyConfigToUi()
{
    _widgets.physics.solverCombo->blockSignals(true);
    _widgets.physics.integratorCombo->blockSignals(true);
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
    _widgets.render.zoomSlider->setValue(static_cast<int>(std::clamp(
        _config.defaultZoom * kZoomSliderDivisor, static_cast<float>(kZoomSliderMin),
        static_cast<float>(kZoomSliderMax))));
    _widgets.render.luminositySlider->setValue(
        std::clamp(_config.defaultLuminosity, kLuminosityMin, kLuminosityMax));
    _widgets.render.cullingCheck->setChecked(_config.renderCullingEnabled);
    _widgets.render.lodCheck->setChecked(_config.renderLODEnabled);
    _widgets.physics.solverCombo->blockSignals(false);
    _widgets.physics.integratorCombo->blockSignals(false);
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
}

void Window::captureUiIntoConfig()
{
    _config.solver = _widgets.physics.solverCombo->currentText().toStdString();
    _config.integrator = _widgets.physics.integratorCombo->currentText().toStdString();
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
    _config.defaultZoom = static_cast<float>(_widgets.render.zoomSlider->value()) / kZoomSliderDivisor;
    _config.defaultLuminosity = _widgets.render.luminositySlider->value();
    _config.renderCullingEnabled = _widgets.render.cullingCheck->isChecked();
    _config.renderLODEnabled = _widgets.render.lodCheck->isChecked();
}

void Window::applyPerformanceProfileToRuntime()
{
    _clientDrawCap = _controller.applyPerformanceProfile(_config, *_runtime);
    applyViewSettings();
}

void Window::markConfigDirty(bool dirty)
{
    _configDirty = dirty;
    setWindowTitle(_configDirty ? "N-Body Qt Client *" : "N-Body Qt Client");
}

bool Window::saveConfigToDisk()
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

bool Window::refreshValidationReport(bool blockOnErrors)
{
    const bltzr_config::ScenarioValidationReport report = _controller.validate(_config);
    const Advisory advisory = Throughput::evaluate(_config, _clientDrawCap);
    if (_widgets.telemetry.validationLabel != nullptr) {
        _widgets.telemetry.validationLabel->setText(buildValidationText(report, advisory));
    }
    if (blockOnErrors && !report.validForRun && _widgets.telemetry.statusLabel != nullptr) {
        _widgets.telemetry.statusLabel->setText(QString("preflight validation failed; fix config errors"));
    }
    return !blockOnErrors || report.validForRun;
}

QString Window::buildValidationText(const bltzr_config::ScenarioValidationReport& report,
                                        const Advisory& advisory) const
{
    std::string text = bltzr_config::SimulationScenarioValidation::renderText(report);
    if (advisory.severity != Severity::None) {
        text += "\n\n[" +
                std::string(advisory.severity == Severity::Warning
                                ? "throughput-warning"
                                : "throughput-advisory") +
                "] ";
        text += advisory.summary;
        if (!advisory.action.empty()) {
            text += "\nAction: " + advisory.action;
        }
    }
    return QString::fromStdString(text);
}

void Window::showThroughputAdvisory(const Advisory& advisory)
{
    if (advisory.severity == Severity::None) {
        return;
    }
    std::cout << "[qt] " << advisory.statusBarText << "\n";
    statusBar()->showMessage(QString::fromStdString(advisory.statusBarText), 6000);
}

void Window::update3DCameraFromSliders()
{
    const float yaw = static_cast<float>(_widgets.render.yawSlider->value()) * kDegreesToRadians;
    const float pitch = static_cast<float>(_widgets.render.pitchSlider->value()) * kDegreesToRadians;
    const float roll = static_cast<float>(_widgets.render.rollSlider->value()) * kDegreesToRadians;
    _widgets.view.multiView->set3DCameraAngles(yaw, pitch, roll);
}
} // namespace bltzr_qt
