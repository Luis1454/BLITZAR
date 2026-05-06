/*
 * Split MainWindow config methods into a smaller translation unit.
 */
#include "ui/MainWindow.hpp"
#include "config/SimulationScenarioValidation.hpp"
#include "ui/ThroughputAdvisor.hpp"
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>
#include <QWidget>
#include <algorithm>
#include <iostream>

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

} // namespace bltzr_qt
