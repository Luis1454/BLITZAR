// File: modules/qt/ui/MainWindowFileActions.cpp
// Purpose: Client module implementation for BLITZAR extension workflows.

#include "client/ClientCommon.hpp"
#include "ui/EnergyGraphWidget.hpp"
#include "ui/MainWindow.hpp"
#include <QCheckBox>
#include <QFileDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QStatusBar>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>
namespace grav_qt {
std::string MainWindow::formatFromSelectedFilter(const QString& filter)
{
    if (filter.startsWith("VTK ASCII")) {
        return "vtk";
    }
    if (filter.startsWith("VTK BINARY")) {
        return "vtk_binary";
    }
    if (filter.startsWith("XYZ")) {
        return "xyz";
    }
    if (filter.startsWith("Native binary")) {
        return "bin";
    }
    return {};
}
void MainWindow::configureRemoteConnectorFromUi()
{
    std::string host = _serverHostEdit->text().trimmed().toStdString();
    if (host.empty()) {
        host = "127.0.0.1";
        _serverHostEdit->setText(QString::fromStdString(host));
    }
    _runtime->configureRemoteConnector(host, static_cast<std::uint16_t>(_serverPortSpin->value()),
                                       _serverAutostartCheck->isChecked(),
                                       _serverBinEdit->text().trimmed().toStdString());
}
void MainWindow::applyConnectorSettings(bool reconnectNow)
{
    configureRemoteConnectorFromUi();
    if (reconnectNow) {
        _runtime->requestReconnect();
        _energyGraph->clearHistory();
        _lastEnergyStep = std::numeric_limits<std::uint64_t>::max();
        statusBar()->showMessage("Connector updated and reconnect requested", 3000);
        return;
    }
    statusBar()->showMessage("Connector settings updated", 3000);
}
void MainWindow::requestReconnectFromUi()
{
    _runtime->requestReconnect();
    _energyGraph->clearHistory();
    _lastEnergyStep = std::numeric_limits<std::uint64_t>::max();
    statusBar()->showMessage("Reconnect requested", 3000);
}
void MainWindow::handleExportRequest()
{
    const SimulationStats stats = _runtime->getCachedStats();
    const std::string preferredFormat = grav_client::normalizeExportFormat(
        _config.exportFormat.empty() ? std::string("vtk") : _config.exportFormat);
    const QString startPath = QString::fromStdString(grav_client::buildSuggestedExportPath(
        _config.exportDirectory, preferredFormat, stats.steps));
    QString selectedFilter = "VTK ASCII (*.vtk)";
    if (preferredFormat == "vtk_binary")
        selectedFilter = "VTK BINARY (*.vtk)";
    else if (preferredFormat == "xyz")
        selectedFilter = "XYZ (*.xyz)";
    else if (preferredFormat == "bin")
        selectedFilter = "Native binary (*.bin)";
    const QString pathChosen =
        QFileDialog::getSaveFileName(this, "Export Snapshot", startPath,
                                     "VTK ASCII (*.vtk);;VTK BINARY (*.vtk);;XYZ (*.xyz);;Native "
                                     "binary (*.bin);;All files (*.*)",
                                     &selectedFilter);
    if (pathChosen.isEmpty()) {
        return;
    }
    std::string format =
        grav_client::normalizeExportFormat(formatFromSelectedFilter(selectedFilter));
    std::string path = pathChosen.toStdString();
    if (format.empty()) {
        format = grav_client::normalizeExportFormat(grav_client::inferExportFormatFromPath(path));
    }
    if (format.empty()) {
        format = grav_client::normalizeExportFormat(_config.exportFormat);
        if (format.empty()) {
            format = "vtk";
        }
    }
    std::filesystem::path outPath(path);
    if (outPath.extension().empty()) {
        const std::string ext = grav_client::extensionForExportFormat(format);
        if (!ext.empty()) {
            outPath += ext;
            path = outPath.string();
        }
    }
    _runtime->requestExportSnapshot(path, format);
    _config.exportFormat = format;
    if (outPath.has_parent_path()) {
        _config.exportDirectory = outPath.parent_path().string();
    }
    markConfigDirty();
}
void MainWindow::handleSaveCheckpointRequest()
{
    const SimulationStats stats = _runtime->getCachedStats();
    std::filesystem::path startPath =
        std::filesystem::path(_config.exportDirectory.empty() ? "exports"
                                                              : _config.exportDirectory) /
        ("checkpoint_s" + std::to_string(stats.steps) + ".chk");
    const QString pathChosen = QFileDialog::getSaveFileName(
        this, "Save Checkpoint", QString::fromStdString(startPath.string()),
        "Checkpoint (*.chk);;All files (*.*)");
    if (pathChosen.isEmpty()) {
        return;
    }
    std::filesystem::path outputPath(pathChosen.toStdString());
    if (outputPath.extension().empty()) {
        outputPath += ".chk";
    }
    _runtime->requestSaveCheckpoint(outputPath.string());
    if (outputPath.has_parent_path()) {
        _config.exportDirectory = outputPath.parent_path().string();
    }
    statusBar()->showMessage("Checkpoint save requested", 3000);
    markConfigDirty();
}
void MainWindow::handleLoadCheckpointRequest()
{
    const QString startPath = _config.inputFile.empty()
                                  ? QString::fromStdString(_config.exportDirectory)
                                  : QString::fromStdString(_config.inputFile);
    const QString path = QFileDialog::getOpenFileName(this, "Load Checkpoint", startPath,
                                                      "Checkpoint (*.chk);;All files (*.*)");
    if (path.isEmpty()) {
        return;
    }
    _runtime->requestLoadCheckpoint(path.toStdString());
    _energyGraph->clearHistory();
    _lastEnergyStep = std::numeric_limits<std::uint64_t>::max();
    statusBar()->showMessage("Checkpoint load requested", 3000);
}
void MainWindow::handleLoadInputRequest()
{
    const QString startPath = _config.inputFile.empty()
                                  ? QString::fromStdString(_config.exportDirectory)
                                  : QString::fromStdString(_config.inputFile);
    const QString path =
        QFileDialog::getOpenFileName(this, "Load Initial State", startPath,
                                     "Simulation files (*.vtk *.xyz *.bin);;All files (*.*)");
    if (path.isEmpty()) {
        return;
    }
    _config.inputFile = path.toStdString();
    _config.inputFormat = "auto";
    _config.presetStructure = "file";
    _config.initMode = "file";
    (void)applyConfigToServer(true);
    markConfigDirty();
}
void MainWindow::handleLoadPresetRequest()
{
    const QString path = QFileDialog::getOpenFileName(this, "Load Preset Config",
                                                      QString::fromStdString(_configPath),
                                                      "Ini files (*.ini);;All files (*.*)");
    if (path.isEmpty()) {
        return;
    }
    _config = SimulationConfig::loadOrCreate(path.toStdString());
    _configPath = path.toStdString();
    applyConfigToUi();
    applyConfigToServer(true);
    _energyGraph->clearHistory();
    _lastEnergyStep = std::numeric_limits<std::uint64_t>::max();
    markConfigDirty(false);
}
void MainWindow::resetSimulationFromUi()
{
    _runtime->requestReset();
    _energyGraph->clearHistory();
    _lastEnergyStep = std::numeric_limits<std::uint64_t>::max();
    statusBar()->showMessage("Simulation reset requested", 3000);
}
} // namespace grav_qt
