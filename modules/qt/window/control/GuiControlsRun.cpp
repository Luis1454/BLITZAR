/*
 * @file modules/qt/window/control/GuiControlsRun.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Run and performance control connections for the Qt window.
 */

#include "config/profile/profile/CfgMain.hpp"
#include "config/profile/profile/CfgPerformance.hpp"
#include "window/core/GuiWindow.hpp"
#include <QComboBox>
#include <QPushButton>
#include <QStatusBar>

namespace bltzr_qt {
void Window::connectRunControls()
{
    connect(_widgets.run.pauseButton, &QPushButton::clicked, this, [this](bool checked) {
        _runtime->setPaused(checked);
        _widgets.run.pauseButton->setText(checked ? "Resume" : "Pause");
    });
    connect(_widgets.run.stepButton, &QPushButton::clicked, this, [this]() {
        _runtime->stepOnce();
    });
    connect(_widgets.run.resetButton, &QPushButton::clicked, this, [this]() {
        resetSimulationFromUi();
    });
    connect(_widgets.run.recoverButton, &QPushButton::clicked, this, [this]() {
        _runtime->requestRecover();
    });
    connect(_widgets.run.applyConnectorButton, &QPushButton::clicked, this, [this]() {
        applyConnectorSettings(true);
    });
    connect(_widgets.run.performanceCombo, &QComboBox::currentTextChanged, this,
            [this](const QString& profile) {
                _config.performanceProfile = profile.toStdString();
                bltzr_config::applyPerformanceProfile(_config);
                applyConfigToUi();
                applyPerformanceProfileToRuntime();
                markConfigDirty();
                statusBar()->showMessage(QString("Run profile applied: %1").arg(profile), 3000);
            });
}
} // namespace bltzr_qt
