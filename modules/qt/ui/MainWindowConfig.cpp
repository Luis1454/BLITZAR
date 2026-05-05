/*
 * @file modules/qt/ui/MainWindowConfig.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "config/SimulationScenarioValidation.hpp"
#include "ui/MainWindow.hpp"
#include "ui/mainWindow/config/apply.hpp"
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
    return MainWindow_applyConfigToServer(this, requestReset);
}

void MainWindow::applyConfigToUi()
{
    MainWindow_applyConfigToUi(this);
}

void MainWindow::captureUiIntoConfig()
{
    MainWindow_captureUiIntoConfig(this);
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
