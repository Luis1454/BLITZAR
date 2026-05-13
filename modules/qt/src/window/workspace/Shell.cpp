/*
 * @file modules/qt/src/window/workspace/Shell.cpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "widgets/graphs/Graph.hpp"
#include "window/core/Window.hpp"
#include "widgets/viewport/MultiView.hpp"
#include "support/theme/Theme.hpp"
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
void Window::buildWorkspaceDocks(QTabWidget* sidebarTabs, QWidget* summaryPane,
                                     QWidget* validationPane)
{
    _widgets.workspace.controlsDock = new QDockWidget("Controls", this);
    _widgets.workspace.controlsDock->setObjectName("controlsDock");
    _widgets.workspace.controlsDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    _widgets.workspace.controlsDock->setWidget(sidebarTabs);
    _widgets.workspace.controlsDock->setMinimumWidth(236);
    addDockWidget(Qt::LeftDockWidgetArea, _widgets.workspace.controlsDock);
    _widgets.workspace.telemetryDock = new QDockWidget("Telemetry", this);
    _widgets.workspace.telemetryDock->setObjectName("telemetryDock");
    _widgets.workspace.telemetryDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    _widgets.workspace.telemetryDock->setWidget(summaryPane);
    _widgets.workspace.telemetryDock->setMinimumHeight(164);
    addDockWidget(Qt::BottomDockWidgetArea, _widgets.workspace.telemetryDock);
    _widgets.workspace.energyDock = new QDockWidget("Energy", this);
    _widgets.workspace.energyDock->setObjectName("energyDock");
    _widgets.workspace.energyDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    _widgets.workspace.energyDock->setWidget(_widgets.view.energyGraph);
    _widgets.workspace.energyDock->setMinimumHeight(136);
    addDockWidget(Qt::BottomDockWidgetArea, _widgets.workspace.energyDock);
    _widgets.workspace.validationDock = new QDockWidget("Validation", this);
    _widgets.workspace.validationDock->setObjectName("validationDock");
    _widgets.workspace.validationDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    _widgets.workspace.validationDock->setWidget(validationPane);
    addDockWidget(Qt::BottomDockWidgetArea, _widgets.workspace.validationDock);
    tabifyDockWidget(_widgets.workspace.telemetryDock, _widgets.workspace.validationDock);
    resizeDocks({_widgets.workspace.controlsDock}, {236}, Qt::Horizontal);
    resizeDocks({_widgets.workspace.energyDock}, {148}, Qt::Vertical);
    _widgets.workspace.energyDock->raise();
    _widgets.workspace.telemetryDock->hide();
    _widgets.workspace.validationDock->hide();
}

void Window::buildMenus()
{
    auto* fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction(
        "Save Config", QKeySequence::Save, this,
        [this]() {
            (void)saveConfigToDisk();
        });
    fileMenu->addAction("Load Preset...", this, [this]() {
        handleLoadPresetRequest();
    });
    fileMenu->addAction("Load Checkpoint...", this, [this]() {
        handleLoadCheckpointRequest();
    });
    fileMenu->addAction("Load Input...", this, [this]() {
        handleLoadInputRequest();
    });
    fileMenu->addAction("Save Checkpoint...", this, [this]() {
        handleSaveCheckpointRequest();
    });
    fileMenu->addAction("Export Snapshot...", this, [this]() {
        handleExportRequest();
    });
    fileMenu->addSeparator();
    fileMenu->addAction(
        "Quit", QKeySequence::Quit, this,
        [this]() {
            close();
        });
    auto* editMenu = menuBar()->addMenu("&Edit");
    editMenu->addAction("Validate Config", this, [this]() {
        (void)refreshValidationReport(false);
    });
    editMenu->addAction("Reconnect", this, [this]() {
        requestReconnectFromUi();
    });
    auto* viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction(_widgets.workspace.controlsDock->toggleViewAction());
    viewMenu->addAction(_widgets.workspace.energyDock->toggleViewAction());
    viewMenu->addAction(_widgets.workspace.telemetryDock->toggleViewAction());
    viewMenu->addAction(_widgets.workspace.validationDock->toggleViewAction());
    _widgets.workspace.octreeOverlayAction = viewMenu->addAction("Octree Overlay");
    _widgets.workspace.octreeOverlayAction->setObjectName("octreeOverlayAction");
    _widgets.workspace.octreeOverlayAction->setCheckable(true);
    _widgets.workspace.gpuTelemetryAction = viewMenu->addAction("GPU Telemetry");
    _widgets.workspace.gpuTelemetryAction->setObjectName("gpuTelemetryAction");
    _widgets.workspace.gpuTelemetryAction->setCheckable(true);
    auto* themeMenu = viewMenu->addMenu("Theme");
    auto* themeGroup = new QActionGroup(themeMenu);
    themeGroup->setExclusive(true);
    auto* lightThemeAction = themeMenu->addAction("Light");
    lightThemeAction->setObjectName("themeLightAction");
    lightThemeAction->setCheckable(true);
    auto* darkThemeAction = themeMenu->addAction("Dark");
    darkThemeAction->setObjectName("themeDarkAction");
    darkThemeAction->setCheckable(true);
    themeGroup->addAction(lightThemeAction);
    themeGroup->addAction(darkThemeAction);
    if (Theme::resolve(_config.uiTheme) == ThemeMode::Dark) {
        darkThemeAction->setChecked(true);
    }
    else {
        lightThemeAction->setChecked(true);
    }
    auto* simulationMenu = menuBar()->addMenu("&Simulation");
    simulationMenu->addAction(
        "Pause / Resume", QKeySequence(Qt::Key_Space), this,
        [this]() {
            _widgets.run.pauseButton->click();
        });
    simulationMenu->addAction("Step", this, [this]() {
        _widgets.run.stepButton->click();
    });
    simulationMenu->addAction("Reset", this, [this]() {
        _widgets.run.resetButton->click();
    });
    simulationMenu->addAction("Recover", this, [this]() {
        _widgets.run.recoverButton->click();
    });
    auto* windowMenu = menuBar()->addMenu("&Window");
    windowMenu->addAction("Raise Controls", this, [this]() {
        _widgets.workspace.controlsDock->raise();
    });
    windowMenu->addAction("Raise Energy", this, [this]() {
        _widgets.workspace.energyDock->raise();
    });
    windowMenu->addAction("Raise Telemetry", this, [this]() {
        _widgets.workspace.telemetryDock->raise();
    });
    windowMenu->addAction("Raise Validation", this, [this]() {
        _widgets.workspace.validationDock->raise();
    });
    auto* workspaceMenu = windowMenu->addMenu("Workspace");
    workspaceMenu->addAction("Save Workspace...", this, [this]() {
        saveWorkspacePreset();
    });
    workspaceMenu->addAction("Load Workspace...", this, [this]() {
        loadWorkspacePreset();
    });
    workspaceMenu->addAction("Delete Workspace...", this, [this]() {
        deleteWorkspacePreset();
    });
    workspaceMenu->addSeparator();
    workspaceMenu->addAction("Restore Default Workspace", this, [this]() {
        restoreDefaultWorkspace();
    });
    auto* helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("About Workspace", this, [this]() {
        _widgets.telemetry.statusLabel->setText("Workspace shell active");
        statusBar()->showMessage("Workspace shell active", 3000);
    });
    connect(lightThemeAction, &QAction::triggered, this, [this]() {
        _config.uiTheme = "light";
        applyTheme();
        markConfigDirty();
        statusBar()->showMessage("Theme applied: light", 3000);
    });
    connect(darkThemeAction, &QAction::triggered, this, [this]() {
        _config.uiTheme = "dark";
        applyTheme();
        markConfigDirty();
        statusBar()->showMessage("Theme applied: dark", 3000);
    });
    connect(_widgets.render.octreeOverlayCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        if (_widgets.workspace.octreeOverlayAction != nullptr && _widgets.workspace.octreeOverlayAction->isChecked() != enabled) {
            _widgets.workspace.octreeOverlayAction->blockSignals(true);
            _widgets.workspace.octreeOverlayAction->setChecked(enabled);
            _widgets.workspace.octreeOverlayAction->blockSignals(false);
        }
        _widgets.view.multiView->setOctreeOverlay(enabled, _widgets.render.octreeOverlayDepthSpin->value(),
                                     _widgets.render.octreeOverlayOpacitySpin->value());
        statusBar()->showMessage(enabled ? "Octree overlay enabled" : "Octree overlay disabled",
                                 3000);
    });
    connect(_widgets.workspace.octreeOverlayAction, &QAction::toggled, this, [this](bool enabled) {
        if (_widgets.render.octreeOverlayCheck != nullptr && _widgets.render.octreeOverlayCheck->isChecked() != enabled) {
            _widgets.render.octreeOverlayCheck->blockSignals(true);
            _widgets.render.octreeOverlayCheck->setChecked(enabled);
            _widgets.render.octreeOverlayCheck->blockSignals(false);
            _widgets.view.multiView->setOctreeOverlay(enabled, _widgets.render.octreeOverlayDepthSpin->value(),
                                         _widgets.render.octreeOverlayOpacitySpin->value());
            statusBar()->showMessage(enabled ? "Octree overlay enabled" : "Octree overlay disabled",
                                     3000);
        }
    });
    connect(_widgets.render.gpuTelemetryCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        if (_widgets.workspace.gpuTelemetryAction != nullptr && _widgets.workspace.gpuTelemetryAction->isChecked() != enabled) {
            _widgets.workspace.gpuTelemetryAction->blockSignals(true);
            _widgets.workspace.gpuTelemetryAction->setChecked(enabled);
            _widgets.workspace.gpuTelemetryAction->blockSignals(false);
        }
        _runtime->setGpuTelemetryEnabled(enabled);
        if (enabled) {
            _widgets.workspace.telemetryDock->show();
            _widgets.workspace.telemetryDock->raise();
        }
        statusBar()->showMessage(enabled ? "GPU telemetry enabled" : "GPU telemetry disabled",
                                 3000);
    });
    connect(_widgets.workspace.gpuTelemetryAction, &QAction::toggled, this, [this](bool enabled) {
        if (_widgets.render.gpuTelemetryCheck != nullptr && _widgets.render.gpuTelemetryCheck->isChecked() != enabled) {
            _widgets.render.gpuTelemetryCheck->blockSignals(true);
            _widgets.render.gpuTelemetryCheck->setChecked(enabled);
            _widgets.render.gpuTelemetryCheck->blockSignals(false);
            _runtime->setGpuTelemetryEnabled(enabled);
            statusBar()->showMessage(enabled ? "GPU telemetry enabled" : "GPU telemetry disabled",
                                     3000);
        }
    });
}
} // namespace bltzr_qt
