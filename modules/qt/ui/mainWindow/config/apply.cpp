/*
 * Moved implementations for MainWindow config-related methods to reduce file size.
 */
#include "ui/MainWindow.hpp"
#include "mainWindow/config/apply.hpp"
#include "config/SimulationScenarioValidation.hpp"
#include "ui/ThroughputAdvisor.hpp"
#include <algorithm>
#include <iostream>

namespace bltzr_qt {

bool MainWindow_applyConfigToServer(MainWindow* self, bool requestReset)
{
    self->captureUiIntoConfig();
    const MainWindowApplyConfigResult result = self->_controller.applyConfig(self->_config, *self->_runtime, requestReset);
    const ThroughputAdvisory advisory = ThroughputAdvisor::evaluate(self->_config, result.clientDrawCap);
    if (self->_validationLabel != nullptr) {
        self->_validationLabel->setText(self->buildValidationText(result.report, advisory));
    }
    self->showThroughputAdvisory(advisory);
    if (!result.report.validForRun) {
        if (self->_statusLabel != nullptr) {
            self->_statusLabel->setText(QString("preflight validation failed; fix config errors"));
        }
        return false;
    }
    self->_clientDrawCap = result.clientDrawCap;
    self->applyViewSettings();
    return true;
}

void MainWindow_applyConfigToUi(MainWindow* self)
{
    self->_solverCombo->blockSignals(true);
    self->_integratorCombo->blockSignals(true);
    self->_performanceCombo->blockSignals(true);
    self->_simulationProfileCombo->blockSignals(true);
    self->_presetCombo->blockSignals(true);
    self->_sphCheck->blockSignals(true);
    self->_dtSpin->blockSignals(true);
    self->_thetaSpin->blockSignals(true);
    self->_softeningSpin->blockSignals(true);
    self->_sphSmoothingSpin->blockSignals(true);
    self->_sphRestDensitySpin->blockSignals(true);
    self->_sphGasConstantSpin->blockSignals(true);
    self->_sphViscositySpin->blockSignals(true);
    self->_zoomSlider->blockSignals(true);
    self->_luminositySlider->blockSignals(true);
    self->_cullingCheck->blockSignals(true);
    self->_lodCheck->blockSignals(true);
    self->_solverCombo->setCurrentIndex(
        std::max(0, self->_solverCombo->findText(QString::fromStdString(self->_config.solver))));
    self->_integratorCombo->setCurrentIndex(
        std::max(0, self->_integratorCombo->findText(QString::fromStdString(self->_config.integrator))));
    self->_performanceCombo->setCurrentIndex(std::max(
        0, self->_performanceCombo->findText(QString::fromStdString(self->_config.performanceProfile))));
    self->_simulationProfileCombo->setCurrentIndex(std::max(
        0, self->_simulationProfileCombo->findText(QString::fromStdString(self->_config.simulationProfile))));
    const int presetIndex =
        std::max(0, self->_presetCombo->findText(QString::fromStdString(self->_config.presetStructure)));
    self->_presetCombo->setCurrentIndex(presetIndex);
    self->_sphCheck->setChecked(self->_config.sphEnabled);
    self->_dtSpin->setValue(std::max(0.00001f, self->_config.dt));
    self->_thetaSpin->setValue(std::clamp(self->_config.octreeTheta, 0.05f, 4.0f));
    self->_softeningSpin->setValue(std::clamp(self->_config.octreeSoftening, 0.0001f, 5.0f));
    self->_sphSmoothingSpin->setValue(std::max(0.05f, self->_config.sphSmoothingLength));
    self->_sphRestDensitySpin->setValue(std::max(0.05f, self->_config.sphRestDensity));
    self->_sphGasConstantSpin->setValue(std::max(0.01f, self->_config.sphGasConstant));
    self->_sphViscositySpin->setValue(std::max(0.0f, self->_config.sphViscosity));
    self->_zoomSlider->setValue(static_cast<int>(std::clamp(self->_config.defaultZoom * 10.0f, 1.0f, 400.0f)));
    self->_luminositySlider->setValue(std::clamp(self->_config.defaultLuminosity, 0, 255));
    self->_cullingCheck->setChecked(self->_config.renderCullingEnabled);
    self->_lodCheck->setChecked(self->_config.renderLODEnabled);
    self->_solverCombo->blockSignals(false);
    self->_integratorCombo->blockSignals(false);
    self->_performanceCombo->blockSignals(false);
    self->_simulationProfileCombo->blockSignals(false);
    self->_presetCombo->blockSignals(false);
    self->_sphCheck->blockSignals(false);
    self->_dtSpin->blockSignals(false);
    self->_thetaSpin->blockSignals(false);
    self->_softeningSpin->blockSignals(false);
    self->_sphSmoothingSpin->blockSignals(false);
    self->_sphRestDensitySpin->blockSignals(false);
    self->_sphGasConstantSpin->blockSignals(false);
    self->_sphViscositySpin->blockSignals(false);
    self->_zoomSlider->blockSignals(false);
    self->_luminositySlider->blockSignals(false);
    self->_cullingCheck->blockSignals(false);
    self->_lodCheck->blockSignals(false);
    self->applyViewSettings();
}

void MainWindow_captureUiIntoConfig(MainWindow* self)
{
    self->_config.solver = self->_solverCombo->currentText().toStdString();
    self->_config.integrator = self->_integratorCombo->currentText().toStdString();
    self->_config.performanceProfile = self->_performanceCombo->currentText().toStdString();
    self->_config.simulationProfile = self->_simulationProfileCombo->currentText().toStdString();
    self->_config.presetStructure = self->_presetCombo->currentText().toStdString();
    self->_config.sphEnabled = self->_sphCheck->isChecked();
    self->_config.dt = static_cast<float>(self->_dtSpin->value());
    self->_config.octreeTheta = static_cast<float>(self->_thetaSpin->value());
    self->_config.octreeSoftening = static_cast<float>(self->_softeningSpin->value());
    self->_config.sphSmoothingLength = static_cast<float>(self->_sphSmoothingSpin->value());
    self->_config.sphRestDensity = static_cast<float>(self->_sphRestDensitySpin->value());
    self->_config.sphGasConstant = static_cast<float>(self->_sphGasConstantSpin->value());
    self->_config.sphViscosity = static_cast<float>(self->_sphViscositySpin->value());
    self->_config.defaultZoom = static_cast<float>(self->_zoomSlider->value()) / 10.0f;
    self->_config.defaultLuminosity = self->_luminositySlider->value();
    self->_config.renderCullingEnabled = self->_cullingCheck->isChecked();
    self->_config.renderLODEnabled = self->_lodCheck->isChecked();
}

} // namespace bltzr_qt
