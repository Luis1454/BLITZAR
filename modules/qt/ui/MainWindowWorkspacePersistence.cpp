// File: modules/qt/ui/MainWindowWorkspacePersistence.cpp
// Purpose: Client module implementation for BLITZAR extension workflows.

#include "ui/MainWindow.hpp"
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QStatusBar>
#include <string>
#include <vector>

namespace grav_qt {
/// Description: Executes the saveWorkspacePreset operation.
void MainWindow::saveWorkspacePreset()
{
    bool accepted = false;
    const QString rawName = QInputDialog::getText(this, "Save Workspace", "Preset name",
                                                  QLineEdit::Normal, "default", &accepted);
    if (!accepted || rawName.trimmed().isEmpty()) {
        return;
    }
    const bool saved = _workspaceLayouts.savePreset(rawName.trimmed().toStdString(),
                                                    saveState().toBase64().toStdString(),
                                                    saveGeometry().toBase64().toStdString());
    _statusLabel->setText(saved ? QString("workspace saved: %1").arg(rawName.trimmed())
                                : QString("failed to save workspace"));
    statusBar()->showMessage(saved ? QString("Workspace saved: %1").arg(rawName.trimmed())
                                   : QString("Failed to save workspace"),
                             3000);
}

/// Description: Executes the loadWorkspacePreset operation.
void MainWindow::loadWorkspacePreset()
{
    const std::vector<std::string> presets = _workspaceLayouts.listPresets();
    if (presets.empty()) {
        _statusLabel->setText("no saved workspace preset");
        statusBar()->showMessage("No saved workspace preset", 3000);
        return;
    }
    QStringList items;
    for (const std::string& preset : presets)
        items.push_back(QString::fromStdString(preset));
    bool accepted = false;
    const QString selected =
        QInputDialog::getItem(this, "Load Workspace", "Preset", items, 0, false, &accepted);
    if (!accepted || selected.isEmpty()) {
        return;
    }
    std::string state;
    std::string geometry;
    if (!_workspaceLayouts.loadPreset(selected.toStdString(), state, geometry)) {
        _statusLabel->setText("failed to load workspace");
        statusBar()->showMessage("Failed to load workspace", 3000);
        return;
    }
    const bool geometryRestored =
        restoreGeometry(QByteArray::fromBase64(QByteArray::fromStdString(geometry)));
    const bool stateRestored =
        restoreState(QByteArray::fromBase64(QByteArray::fromStdString(state)));
    _statusLabel->setText((geometryRestored && stateRestored)
                              ? QString("workspace loaded: %1").arg(selected)
                              : QString("workspace preset invalid"));
    statusBar()->showMessage((geometryRestored && stateRestored)
                                 ? QString("Workspace loaded: %1").arg(selected)
                                 : QString("Workspace preset invalid"),
                             3000);
}

/// Description: Executes the deleteWorkspacePreset operation.
void MainWindow::deleteWorkspacePreset()
{
    const std::vector<std::string> presets = _workspaceLayouts.listPresets();
    if (presets.empty()) {
        _statusLabel->setText("no workspace preset to delete");
        statusBar()->showMessage("No workspace preset to delete", 3000);
        return;
    }
    QStringList items;
    for (const std::string& preset : presets)
        items.push_back(QString::fromStdString(preset));
    bool accepted = false;
    const QString selected =
        QInputDialog::getItem(this, "Delete Workspace", "Preset", items, 0, false, &accepted);
    if (!accepted || selected.isEmpty()) {
        return;
    }
    const bool deleted = _workspaceLayouts.deletePreset(selected.toStdString());
    _statusLabel->setText(deleted ? QString("workspace deleted: %1").arg(selected)
                                  : QString("failed to delete workspace"));
    statusBar()->showMessage(deleted ? QString("Workspace deleted: %1").arg(selected)
                                     : QString("Failed to delete workspace"),
                             3000);
}

/// Description: Executes the restoreDefaultWorkspace operation.
void MainWindow::restoreDefaultWorkspace()
{
    const bool geometryRestored = restoreGeometry(_defaultWorkspaceGeometry);
    const bool stateRestored = restoreState(_defaultWorkspaceState);
    _statusLabel->setText((geometryRestored && stateRestored)
                              ? QString("default workspace restored")
                              : QString("failed to restore default workspace"));
    statusBar()->showMessage((geometryRestored && stateRestored)
                                 ? QString("Default workspace restored")
                                 : QString("Failed to restore default workspace"),
                             3000);
}
} // namespace grav_qt
