/*
 * @file modules/qt/src/window/config/GuiWindowConfig.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for configuration application and persistence.
 */

#include "config/validation/CfgScenario.hpp"
#include "window/config/GuiConfigurationEditor.hpp"
#include "window/core/GuiWindow.hpp"
#include "src/widgets/viewport/GuiMultiView.hpp"
#include "FndConstants.hpp"
#include <QLabel>
#include <QMessageBox>
#include <QSlider>
#include <QStatusBar>
#include <iostream>
#include <string>

namespace bltzr_qt {
void Window::editLoadedConfiguration()
{
    ConfigurationEditor editor(_config, this);
    if (editor.exec() != QDialog::Accepted) {
        return;
    }
    (void)applyEditedConfiguration(editor.configuration());
}

bool Window::applyEditedConfiguration(const SimulationConfig& candidate)
{
    const SimulationConfig previous = _config;
    _config = candidate;
    applyConfigToUi();
    if (!applyConfigToServer(true, false)) {
        const QString candidateValidation = _widgets.telemetry.validationLabel == nullptr
                                                ? QString()
                                                : _widgets.telemetry.validationLabel->text();
        _config = previous;
        applyConfigToUi();
        (void)applyConfigToServer(false, false);
        QMessageBox details(this);
        details.setIcon(QMessageBox::Critical);
        details.setWindowTitle("Invalid configuration");
        details.setText("The edited configuration was rejected. Previous values were restored.\n\n" +
                        candidateValidation);
        details.exec();
        statusBar()->showMessage("Configuration rejected; previous values restored", 5000);
        return false;
    }
    markConfigDirty(true);
    statusBar()->showMessage("Structured configuration applied; use Save Config to persist it", 5000);
    return true;
}

bool Window::applyConfigToServer(bool requestReset, bool captureUi)
{
    if (captureUi) {
        captureUiIntoConfig();
    }
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
