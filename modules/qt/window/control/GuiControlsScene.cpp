/*
 * @file modules/qt/window/control/GuiControlsScene.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Scene, input, export, and scene-profile control connections.
 */

#include "config/profile/profile/CfgMain.hpp"
#include "window/core/GuiWindow.hpp"
#include "window/scene/GuiSceneEditor.hpp"
#include <QComboBox>
#include <QPushButton>
#include <QStatusBar>

namespace bltzr_qt {
void Window::connectSceneControls()
{
    connect(_widgets.scene.exportButton, &QPushButton::clicked, this, [this]() {
        handleExportRequest();
    });
    connect(_widgets.scene.saveConfigButton, &QPushButton::clicked, this, [this]() {
        (void)saveConfigToDisk();
    });
    connect(_widgets.scene.loadInputButton, &QPushButton::clicked, this, [this]() {
        handleLoadInputRequest();
    });
    connect(_widgets.scene.applyPresetButton, &QPushButton::clicked, this, [this]() {
        _config.initConfigStyle = "preset";
        _config.presetStructure = _widgets.scene.presetCombo->currentText().toStdString();
        if (_config.presetStructure != "file") {
            _config.initMode = _config.presetStructure;
        }
        _config.scene.objects.clear();
        if (_sceneEditor != nullptr) {
            _sceneEditor->reload(_config);
        }
        applyConfigToServer(true);
        markConfigDirty();
        statusBar()->showMessage(
            QString("Scene preset applied: %1").arg(_widgets.scene.presetCombo->currentText()), 3000);
    });
    connect(_widgets.scene.loadPresetButton, &QPushButton::clicked, this, [this]() {
        handleLoadPresetRequest();
    });
    connect(_widgets.scene.simulationProfileCombo, &QComboBox::currentTextChanged, this,
            [this](const QString& profile) {
                _config.simulationProfile = profile.toStdString();
                bltzr_config::applySimulationProfile(_config);
                applyConfigToUi();
                (void)applyConfigToServer(true);
                markConfigDirty();
                statusBar()->showMessage(QString("Simulation profile applied: %1").arg(profile), 3000);
            });
}
} // namespace bltzr_qt
