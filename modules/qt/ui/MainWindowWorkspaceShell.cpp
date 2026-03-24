#include "ui/MainWindow.hpp"

#include "ui/EnergyGraphWidget.hpp"
#include "ui/MultiViewWidget.hpp"
#include "ui/QtTheme.hpp"

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

namespace grav_qt {

void MainWindow::buildWorkspaceDocks(QTabWidget *sidebarTabs, QWidget *summaryPane, QWidget *validationPane)
{
    _controlsDock = new QDockWidget("Controls", this);
    _controlsDock->setObjectName("controlsDock");
    _controlsDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    _controlsDock->setWidget(sidebarTabs);
    _controlsDock->setMinimumWidth(236);
    addDockWidget(Qt::LeftDockWidgetArea, _controlsDock);

    _telemetryDock = new QDockWidget("Telemetry", this);
    _telemetryDock->setObjectName("telemetryDock");
    _telemetryDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    _telemetryDock->setWidget(summaryPane);
    _telemetryDock->setMinimumHeight(164);
    addDockWidget(Qt::BottomDockWidgetArea, _telemetryDock);

    _energyDock = new QDockWidget("Energy", this);
    _energyDock->setObjectName("energyDock");
    _energyDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    _energyDock->setWidget(_energyGraph);
    _energyDock->setMinimumHeight(136);
    addDockWidget(Qt::BottomDockWidgetArea, _energyDock);

    _validationDock = new QDockWidget("Validation", this);
    _validationDock->setObjectName("validationDock");
    _validationDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    _validationDock->setWidget(validationPane);
    addDockWidget(Qt::BottomDockWidgetArea, _validationDock);

    tabifyDockWidget(_telemetryDock, _validationDock);
    resizeDocks({_controlsDock}, {236}, Qt::Horizontal);
    resizeDocks({_energyDock}, {148}, Qt::Vertical);
    _energyDock->raise();
    _telemetryDock->hide();
    _validationDock->hide();
}

void MainWindow::buildMenus()
{
    auto *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("Save Config", this, [this]() { (void)saveConfigToDisk(); }, QKeySequence::Save);
    fileMenu->addAction("Load Preset...", this, [this]() { handleLoadPresetRequest(); });
    fileMenu->addAction("Load Checkpoint...", this, [this]() { handleLoadCheckpointRequest(); });
    fileMenu->addAction("Load Input...", this, [this]() { handleLoadInputRequest(); });
    fileMenu->addAction("Save Checkpoint...", this, [this]() { handleSaveCheckpointRequest(); });
    fileMenu->addAction("Export Snapshot...", this, [this]() { handleExportRequest(); });
    fileMenu->addSeparator();
    fileMenu->addAction("Quit", this, [this]() { close(); }, QKeySequence::Quit);

    auto *editMenu = menuBar()->addMenu("&Edit");
    editMenu->addAction("Validate Config", this, [this]() { (void)refreshValidationReport(false); });
    editMenu->addAction("Reconnect", this, [this]() { requestReconnectFromUi(); });

    auto *viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction(_controlsDock->toggleViewAction());
    viewMenu->addAction(_energyDock->toggleViewAction());
    viewMenu->addAction(_telemetryDock->toggleViewAction());
    viewMenu->addAction(_validationDock->toggleViewAction());

    _octreeOverlayAction = viewMenu->addAction("Octree Overlay");
    _octreeOverlayAction->setObjectName("octreeOverlayAction");
    _octreeOverlayAction->setCheckable(true);
    _gpuTelemetryAction = viewMenu->addAction("GPU Telemetry");
    _gpuTelemetryAction->setObjectName("gpuTelemetryAction");
    _gpuTelemetryAction->setCheckable(true);

    auto *themeMenu = viewMenu->addMenu("Theme");
    auto *themeGroup = new QActionGroup(themeMenu);
    themeGroup->setExclusive(true);
    auto *lightThemeAction = themeMenu->addAction("Light");
    lightThemeAction->setObjectName("themeLightAction");
    lightThemeAction->setCheckable(true);
    auto *darkThemeAction = themeMenu->addAction("Dark");
    darkThemeAction->setObjectName("themeDarkAction");
    darkThemeAction->setCheckable(true);
    themeGroup->addAction(lightThemeAction);
    themeGroup->addAction(darkThemeAction);
    if (QtTheme::resolve(_config.uiTheme) == QtThemeMode::Dark) {
        darkThemeAction->setChecked(true);
    } else {
        lightThemeAction->setChecked(true);
    }

    auto *simulationMenu = menuBar()->addMenu("&Simulation");
    simulationMenu->addAction("Pause / Resume", this, [this]() { _pauseButton->click(); }, Qt::Key_Space);
    simulationMenu->addAction("Step", this, [this]() { _stepButton->click(); });
    simulationMenu->addAction("Reset", this, [this]() { _resetButton->click(); });
    simulationMenu->addAction("Recover", this, [this]() { _recoverButton->click(); });

    auto *windowMenu = menuBar()->addMenu("&Window");
    windowMenu->addAction("Raise Controls", this, [this]() { _controlsDock->raise(); });
    windowMenu->addAction("Raise Energy", this, [this]() { _energyDock->raise(); });
    windowMenu->addAction("Raise Telemetry", this, [this]() { _telemetryDock->raise(); });
    windowMenu->addAction("Raise Validation", this, [this]() { _validationDock->raise(); });
    auto *workspaceMenu = windowMenu->addMenu("Workspace");
    workspaceMenu->addAction("Save Workspace...", this, [this]() { saveWorkspacePreset(); });
    workspaceMenu->addAction("Load Workspace...", this, [this]() { loadWorkspacePreset(); });
    workspaceMenu->addAction("Delete Workspace...", this, [this]() { deleteWorkspacePreset(); });
    workspaceMenu->addSeparator();
    workspaceMenu->addAction("Restore Default Workspace", this, [this]() { restoreDefaultWorkspace(); });

    auto *helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("About Workspace", this, [this]() {
        _statusLabel->setText("Workspace shell active");
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
    connect(_octreeOverlayCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        if (_octreeOverlayAction != nullptr && _octreeOverlayAction->isChecked() != enabled) {
            _octreeOverlayAction->blockSignals(true);
            _octreeOverlayAction->setChecked(enabled);
            _octreeOverlayAction->blockSignals(false);
        }
        _multiView->setOctreeOverlay(enabled, _octreeOverlayDepthSpin->value(), _octreeOverlayOpacitySpin->value());
        statusBar()->showMessage(enabled ? "Octree overlay enabled" : "Octree overlay disabled", 3000);
    });
    connect(_octreeOverlayAction, &QAction::toggled, this, [this](bool enabled) {
        if (_octreeOverlayCheck != nullptr && _octreeOverlayCheck->isChecked() != enabled) {
            _octreeOverlayCheck->blockSignals(true);
            _octreeOverlayCheck->setChecked(enabled);
            _octreeOverlayCheck->blockSignals(false);
            _multiView->setOctreeOverlay(enabled, _octreeOverlayDepthSpin->value(), _octreeOverlayOpacitySpin->value());
            statusBar()->showMessage(enabled ? "Octree overlay enabled" : "Octree overlay disabled", 3000);
        }
    });
    connect(_gpuTelemetryCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        if (_gpuTelemetryAction != nullptr && _gpuTelemetryAction->isChecked() != enabled) {
            _gpuTelemetryAction->blockSignals(true);
            _gpuTelemetryAction->setChecked(enabled);
            _gpuTelemetryAction->blockSignals(false);
        }
        _runtime->setGpuTelemetryEnabled(enabled);
        if (enabled) {
            _telemetryDock->show();
            _telemetryDock->raise();
        }
        statusBar()->showMessage(enabled ? "GPU telemetry enabled" : "GPU telemetry disabled", 3000);
    });
    connect(_gpuTelemetryAction, &QAction::toggled, this, [this](bool enabled) {
        if (_gpuTelemetryCheck != nullptr && _gpuTelemetryCheck->isChecked() != enabled) {
            _gpuTelemetryCheck->blockSignals(true);
            _gpuTelemetryCheck->setChecked(enabled);
            _gpuTelemetryCheck->blockSignals(false);
            _runtime->setGpuTelemetryEnabled(enabled);
            statusBar()->showMessage(enabled ? "GPU telemetry enabled" : "GPU telemetry disabled", 3000);
        }
    });
}

} // namespace grav_qt
