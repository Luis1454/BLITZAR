/*
 * @file modules/qt/src/window/actions/FileActions.cpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "Constants.hpp"
#include "client/common/ClientCommon.hpp"
#include "widgets/graphs/Graph.hpp"
#include "window/core/Window.hpp"
#include <QCheckBox>
#include <QFileDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QStatusBar>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>

namespace bltzr_qt {
std::string Window::formatFromSelectedFilter(const QString& filter)
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

void Window::configureRemoteConnectorFromUi()
{
    std::string host = _widgets.run.serverHostEdit->text().trimmed().toStdString();
    if (host.empty()) {
        host = kDefaultLoopbackHost;
        _widgets.run.serverHostEdit->setText(QString::fromStdString(host));
    }
    _runtime->configureRemoteConnector(host, static_cast<std::uint16_t>(_widgets.run.serverPortSpin->value()),
                                       _widgets.run.serverAutostartCheck->isChecked(),
                                       _widgets.run.serverBinEdit->text().trimmed().toStdString());
}

void Window::applyConnectorSettings(bool reconnectNow)
{
    configureRemoteConnectorFromUi();
    if (reconnectNow) {
        _runtime->requestReconnect();
        _widgets.view.energyGraph->clearHistory();
        _lastEnergyStep = std::numeric_limits<std::uint64_t>::max();
        statusBar()->showMessage("Connector updated and reconnect requested", 3000);
        return;
    }
    statusBar()->showMessage("Connector settings updated", 3000);
}

void Window::requestReconnectFromUi()
{
    _runtime->requestReconnect();
    _widgets.view.energyGraph->clearHistory();
    _lastEnergyStep = std::numeric_limits<std::uint64_t>::max();
    statusBar()->showMessage("Reconnect requested", 3000);
}

void Window::handleExportRequest()
{
    const SimulationStats stats = _runtime->getCachedStats();
    const std::string preferredFormat = bltzr_client::normalizeExportFormat(
        _config.exportFormat.empty() ? std::string("vtk") : _config.exportFormat);
    const QString startPath = QString::fromStdString(bltzr_client::buildSuggestedExportPath(
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
        bltzr_client::normalizeExportFormat(formatFromSelectedFilter(selectedFilter));
    std::string path = pathChosen.toStdString();
    if (format.empty()) {
        format = bltzr_client::normalizeExportFormat(bltzr_client::inferExportFormatFromPath(path));
    }
    if (format.empty()) {
        format = bltzr_client::normalizeExportFormat(_config.exportFormat);
        if (format.empty()) {
            format = "vtk";
        }
    }
    std::filesystem::path outPath(path);
    if (outPath.extension().empty()) {
        const std::string ext = bltzr_client::extensionForExportFormat(format);
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

void Window::handleSaveCheckpointRequest()
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

void Window::handleLoadCheckpointRequest()
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
    _widgets.view.energyGraph->clearHistory();
    _lastEnergyStep = std::numeric_limits<std::uint64_t>::max();
    statusBar()->showMessage("Checkpoint load requested", 3000);
}

void Window::handleLoadInputRequest()
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

void Window::handleLoadPresetRequest()
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
    _widgets.view.energyGraph->clearHistory();
    _lastEnergyStep = std::numeric_limits<std::uint64_t>::max();
    markConfigDirty(false);
}

void Window::resetSimulationFromUi()
{
    _runtime->requestReset();
    _widgets.view.energyGraph->clearHistory();
    _lastEnergyStep = std::numeric_limits<std::uint64_t>::max();
    statusBar()->showMessage("Simulation reset requested", 3000);
}
} // namespace bltzr_qt
