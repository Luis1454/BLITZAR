/*
 * @file modules/qt/ui/MainWindowConfig.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "config/SimulationScenarioValidation.hpp"
#include "ui/MainWindow.hpp"
#include "ui/MultiViewWidget.hpp"
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
bool MainWindow::applyConfigToServer(bool requestReset)
{
    captureUiIntoConfig();
    const MainWindowApplyConfigResult result =
        _controller.applyConfig(_config, *_runtime, requestReset);
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
    _solverCombo->setCurrentIndex(
        std::max(0, _solverCombo->findText(QString::fromStdString(_config.solver))));
    _integratorCombo->setCurrentIndex(
        std::max(0, _integratorCombo->findText(QString::fromStdString(_config.integrator))));
    _performanceCombo->setCurrentIndex(std::max(
        0, _performanceCombo->findText(QString::fromStdString(_config.performanceProfile))));
    _simulationProfileCombo->setCurrentIndex(std::max(
        0, _simulationProfileCombo->findText(QString::fromStdString(_config.simulationProfile))));
    const int presetIndex =
        std::max(0, _presetCombo->findText(QString::fromStdString(_config.presetStructure)));
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

bool MainWindow::refreshValidationReport(bool blockOnErrors)
{
    const bltzr_config::ScenarioValidationReport report = _controller.validate(_config);
    const ThroughputAdvisory advisory = ThroughputAdvisor::evaluate(_config, _clientDrawCap);
    if (_validationLabel != nullptr) {
        _validationLabel->setText(buildValidationText(report, advisory));
    }
    if (blockOnErrors && !report.validForRun && _statusLabel != nullptr) {
        _statusLabel->setText(QString("preflight validation failed; fix config errors"));
    }
    return !blockOnErrors || report.validForRun;
}

QString MainWindow::buildValidationText(const bltzr_config::ScenarioValidationReport& report,
                                        const ThroughputAdvisory& advisory) const
{
    std::string text = bltzr_config::SimulationScenarioValidation::renderText(report);
    if (advisory.severity != ThroughputAdvisorySeverity::None) {
        text += "\n\n[" +
                std::string(advisory.severity == ThroughputAdvisorySeverity::Warning
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

void MainWindow::showThroughputAdvisory(const ThroughputAdvisory& advisory)
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
} // namespace bltzr_qt
