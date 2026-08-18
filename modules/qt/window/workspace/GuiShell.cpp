/*
 * @file modules/qt/window/workspace/GuiShell.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop workspace docks and menu composition.
 */

#include "widgets/graphs/GuiGraph.hpp"
#include "widgets/graphs/GuiSpectrumGraph.hpp"
#include "window/core/GuiWindow.hpp"
#include "widgets/viewport/GuiMultiView.hpp"
#include "support/theme/GuiTheme.hpp"
#include <QAction>
#include <QActionGroup>
#include <QCheckBox>
#include <QDockWidget>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QSpinBox>
#include <QStatusBar>

namespace bltzr_qt {
void Window::buildWorkspaceDocks(QPointer<QTabWidget> sidebarTabs, QPointer<QWidget> summaryPane,
                                 QPointer<QWidget> validationPane)
{
    _widgets.workspace.controlsDock = new QDockWidget("Controls", this);
    _widgets.workspace.controlsDock->setObjectName("controlsDock");
    _widgets.workspace.controlsDock->setFeatures(QDockWidget::DockWidgetMovable |
                                                 QDockWidget::DockWidgetFloatable);
    _widgets.workspace.controlsDock->setWidget(sidebarTabs);
    _widgets.workspace.controlsDock->setMinimumWidth(236);
    addDockWidget(Qt::LeftDockWidgetArea, _widgets.workspace.controlsDock);

    _widgets.workspace.telemetryDock = new QDockWidget("Telemetry", this);
    _widgets.workspace.telemetryDock->setObjectName("telemetryDock");
    _widgets.workspace.telemetryDock->setFeatures(QDockWidget::DockWidgetMovable |
                                                  QDockWidget::DockWidgetFloatable);
    _widgets.workspace.telemetryDock->setWidget(summaryPane);
    _widgets.workspace.telemetryDock->setMinimumHeight(164);
    addDockWidget(Qt::BottomDockWidgetArea, _widgets.workspace.telemetryDock);

    _widgets.workspace.energyDock = new QDockWidget("Energy", this);
    _widgets.workspace.energyDock->setObjectName("energyDock");
    _widgets.workspace.energyDock->setFeatures(QDockWidget::DockWidgetMovable |
                                               QDockWidget::DockWidgetFloatable);
    _widgets.workspace.energyDock->setWidget(_widgets.view.energyGraph);
    _widgets.workspace.energyDock->setMinimumHeight(136);
    addDockWidget(Qt::BottomDockWidgetArea, _widgets.workspace.energyDock);

    _widgets.workspace.spectrumDock = new QDockWidget("Structure FFT", this);
    _widgets.workspace.spectrumDock->setObjectName("spectrumDock");
    _widgets.workspace.spectrumDock->setFeatures(QDockWidget::DockWidgetMovable |
                                                 QDockWidget::DockWidgetFloatable);
    _widgets.workspace.spectrumDock->setWidget(_widgets.view.spectrumGraph);
    _widgets.workspace.spectrumDock->setMinimumHeight(196);
    addDockWidget(Qt::BottomDockWidgetArea, _widgets.workspace.spectrumDock);

    _widgets.workspace.validationDock = new QDockWidget("Validation", this);
    _widgets.workspace.validationDock->setObjectName("validationDock");
    _widgets.workspace.validationDock->setFeatures(QDockWidget::DockWidgetMovable |
                                                   QDockWidget::DockWidgetFloatable);
    _widgets.workspace.validationDock->setWidget(validationPane);
    addDockWidget(Qt::BottomDockWidgetArea, _widgets.workspace.validationDock);
    tabifyDockWidget(_widgets.workspace.telemetryDock, _widgets.workspace.validationDock);
    tabifyDockWidget(_widgets.workspace.energyDock, _widgets.workspace.spectrumDock);
    resizeDocks({_widgets.workspace.controlsDock}, {236}, Qt::Horizontal);
    resizeDocks({_widgets.workspace.energyDock}, {148}, Qt::Vertical);
    _widgets.workspace.spectrumDock->raise();
    _widgets.workspace.telemetryDock->hide();
    _widgets.workspace.validationDock->hide();
}

void Window::buildFileMenu(QMenu* menu)
{
    menu->addAction("Save Config", QKeySequence::Save, this, [this]() {
        (void)saveConfigToDisk();
    });
    menu->addAction("Load INI...", this, [this]() { handleLoadPresetRequest(); });
    menu->addAction("Load Checkpoint...", this, [this]() { handleLoadCheckpointRequest(); });
    menu->addAction("Load Input...", this, [this]() { handleLoadInputRequest(); });
    menu->addAction("Save Checkpoint...", this, [this]() { handleSaveCheckpointRequest(); });
    menu->addAction("Export Snapshot...", this, [this]() { handleExportRequest(); });
    menu->addSeparator();
    menu->addAction("Quit", QKeySequence::Quit, this, [this]() { close(); });
}

void Window::buildEditMenu(QMenu* menu)
{
    auto* configuration = menu->addAction("Edit Loaded Configuration...", this, [this]() {
        editLoadedConfiguration();
    });
    configuration->setShortcut(QKeySequence("Ctrl+Shift+E"));
    menu->addSeparator();
    menu->addAction("Validate Config", this, [this]() { (void)refreshValidationReport(false); });
    menu->addAction("Reconnect", this, [this]() { requestReconnectFromUi(); });
}

void Window::buildThemeMenu(QMenu* menu)
{
    auto* group = new QActionGroup(menu);
    group->setExclusive(true);
    auto* light = menu->addAction("Light");
    light->setObjectName("themeLightAction");
    light->setCheckable(true);
    auto* dark = menu->addAction("Dark");
    dark->setObjectName("themeDarkAction");
    dark->setCheckable(true);
    group->addAction(light);
    group->addAction(dark);
    (Theme::resolve(_config.uiTheme) == ThemeMode::Dark ? dark : light)->setChecked(true);
    connect(light, &QAction::triggered, this, [this]() {
        _config.uiTheme = "light";
        applyTheme();
        markConfigDirty();
        statusBar()->showMessage("Theme applied: light", 3000);
    });
    connect(dark, &QAction::triggered, this, [this]() {
        _config.uiTheme = "dark";
        applyTheme();
        markConfigDirty();
        statusBar()->showMessage("Theme applied: dark", 3000);
    });
}

void Window::connectOverlayControls()
{
    connect(_widgets.render.octreeOverlayCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        if (_widgets.workspace.octreeOverlayAction != nullptr &&
            _widgets.workspace.octreeOverlayAction->isChecked() != enabled) {
            _widgets.workspace.octreeOverlayAction->blockSignals(true);
            _widgets.workspace.octreeOverlayAction->setChecked(enabled);
            _widgets.workspace.octreeOverlayAction->blockSignals(false);
        }
        _widgets.view.multiView->setOctreeOverlay(enabled,
                                                   _widgets.render.octreeOverlayDepthSpin->value(),
                                                   _widgets.render.octreeOverlayOpacitySpin->value());
        statusBar()->showMessage(enabled ? "Octree overlay enabled" : "Octree overlay disabled",
                                 3000);
    });
    connect(_widgets.workspace.octreeOverlayAction, &QAction::toggled, this,
            [this](bool enabled) {
                if (_widgets.render.octreeOverlayCheck != nullptr &&
                    _widgets.render.octreeOverlayCheck->isChecked() != enabled) {
                    _widgets.render.octreeOverlayCheck->blockSignals(true);
                    _widgets.render.octreeOverlayCheck->setChecked(enabled);
                    _widgets.render.octreeOverlayCheck->blockSignals(false);
                    _widgets.view.multiView->setOctreeOverlay(
                        enabled, _widgets.render.octreeOverlayDepthSpin->value(),
                        _widgets.render.octreeOverlayOpacitySpin->value());
                    statusBar()->showMessage(
                        enabled ? "Octree overlay enabled" : "Octree overlay disabled", 3000);
                }
            });
}

void Window::connectTelemetryControls()
{
    connect(_widgets.render.gpuTelemetryCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        if (_widgets.workspace.gpuTelemetryAction != nullptr &&
            _widgets.workspace.gpuTelemetryAction->isChecked() != enabled) {
            _widgets.workspace.gpuTelemetryAction->blockSignals(true);
            _widgets.workspace.gpuTelemetryAction->setChecked(enabled);
            _widgets.workspace.gpuTelemetryAction->blockSignals(false);
        }
        _runtime->setGpuTelemetryEnabled(enabled);
        if (enabled) {
            _widgets.workspace.telemetryDock->show();
            _widgets.workspace.telemetryDock->raise();
        }
        statusBar()->showMessage(enabled ? "GPU telemetry enabled" : "GPU telemetry disabled", 3000);
    });
    connect(_widgets.workspace.gpuTelemetryAction, &QAction::toggled, this,
            [this](bool enabled) {
                if (_widgets.render.gpuTelemetryCheck != nullptr &&
                    _widgets.render.gpuTelemetryCheck->isChecked() != enabled) {
                    _widgets.render.gpuTelemetryCheck->blockSignals(true);
                    _widgets.render.gpuTelemetryCheck->setChecked(enabled);
                    _widgets.render.gpuTelemetryCheck->blockSignals(false);
                    _runtime->setGpuTelemetryEnabled(enabled);
                    statusBar()->showMessage(
                        enabled ? "GPU telemetry enabled" : "GPU telemetry disabled", 3000);
                }
            });
}

void Window::buildViewMenu(QMenu* menu)
{
    menu->addAction(_widgets.workspace.controlsDock->toggleViewAction());
    menu->addAction(_widgets.workspace.energyDock->toggleViewAction());
    menu->addAction(_widgets.workspace.spectrumDock->toggleViewAction());
    menu->addAction(_widgets.workspace.telemetryDock->toggleViewAction());
    menu->addAction(_widgets.workspace.validationDock->toggleViewAction());
    _widgets.workspace.octreeOverlayAction = menu->addAction("Octree Overlay");
    _widgets.workspace.octreeOverlayAction->setObjectName("octreeOverlayAction");
    _widgets.workspace.octreeOverlayAction->setCheckable(true);
    _widgets.workspace.gpuTelemetryAction = menu->addAction("GPU Telemetry");
    _widgets.workspace.gpuTelemetryAction->setObjectName("gpuTelemetryAction");
    _widgets.workspace.gpuTelemetryAction->setCheckable(true);
    buildThemeMenu(menu->addMenu("Theme"));
    connectOverlayControls();
    connectTelemetryControls();
}

void Window::buildSimulationMenu(QMenu* menu)
{
    menu->addAction("Pause / Resume", QKeySequence(Qt::Key_Space), this, [this]() {
        _widgets.run.pauseButton->click();
    });
    menu->addAction("Step", this, [this]() { _widgets.run.stepButton->click(); });
    menu->addAction("Reset", this, [this]() { _widgets.run.resetButton->click(); });
    menu->addAction("Recover", this, [this]() { _widgets.run.recoverButton->click(); });
}

void Window::buildWindowMenu(QMenu* menu)
{
    menu->addAction("Raise Controls", this, [this]() { _widgets.workspace.controlsDock->raise(); });
    menu->addAction("Raise Energy", this, [this]() { _widgets.workspace.energyDock->raise(); });
    menu->addAction("Raise Structure FFT", this, [this]() {
        _widgets.workspace.spectrumDock->show();
        _widgets.workspace.spectrumDock->raise();
    });
    menu->addAction("Raise Telemetry", this, [this]() { _widgets.workspace.telemetryDock->raise(); });
    menu->addAction("Raise Validation", this, [this]() { _widgets.workspace.validationDock->raise(); });
    auto* workspace = menu->addMenu("Workspace");
    workspace->addAction("Save Workspace...", this, [this]() { saveWorkspacePreset(); });
    workspace->addAction("Load Workspace...", this, [this]() { loadWorkspacePreset(); });
    workspace->addAction("Delete Workspace...", this, [this]() { deleteWorkspacePreset(); });
    workspace->addSeparator();
    workspace->addAction("Restore Default Workspace", this,
                         [this]() { restoreDefaultWorkspace(); });
}

void Window::buildHelpMenu(QMenu* menu)
{
    menu->addAction("About Workspace", this, [this]() {
        _widgets.telemetry.statusLabel->setText("Workspace shell active");
        statusBar()->showMessage("Workspace shell active", 3000);
    });
}

void Window::buildMenus()
{
    buildFileMenu(menuBar()->addMenu("&File"));
    buildEditMenu(menuBar()->addMenu("&Edit"));
    buildViewMenu(menuBar()->addMenu("&View"));
    buildSimulationMenu(menuBar()->addMenu("&Simulation"));
    buildWindowMenu(menuBar()->addMenu("&Window"));
    buildHelpMenu(menuBar()->addMenu("&Help"));
}
} // namespace bltzr_qt
