/*
 * @file modules/qt/ui/MainWindowLayout.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "ui/MainWindow.hpp"
#include "ui/panels/PhysicsControlPanel.hpp"
#include "ui/panels/RenderControlPanel.hpp"
#include "ui/panels/RunControlPanel.hpp"
#include "ui/panels/SceneSetupPanel.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSizePolicy>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>
#include <QWidget>

namespace bltzr_qt {
QTabWidget* MainWindow::buildSidebarTabs()
{
    auto* sidebarTabs = new QTabWidget(this);
    sidebarTabs->setObjectName("workspaceSidebarTabs");
    sidebarTabs->setTabPosition(QTabWidget::West);
    sidebarTabs->setDocumentMode(true);
    sidebarTabs->setMinimumWidth(220);
    sidebarTabs->setMaximumWidth(248);
    sidebarTabs->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    auto* runPage = new QWidget(sidebarTabs);
    RunControlPanel::build(runPage, _performanceCombo, _pauseButton, _stepButton, _resetButton,
                           _recoverButton, _serverHostEdit, _serverPortSpin, _serverBinEdit,
                           _serverAutostartCheck, _applyConnectorButton);
    auto* scenePage = SceneSetupPanel::build(sidebarTabs, _simulationProfileCombo, _presetCombo,
                                             _applyPresetButton, _loadPresetButton,
                                             _loadInputButton, _saveConfigButton);
    auto* physicsPage = PhysicsControlPanel::build(
        sidebarTabs, _solverCombo, _integratorCombo, _dtSpin, _thetaSpin, _softeningSpin, _sphCheck,
        _sphSmoothingSpin, _sphRestDensitySpin, _sphGasConstantSpin, _sphViscositySpin);
    auto* renderPage = RenderControlPanel::build(
        sidebarTabs, _view3dCombo, _zoomSlider, _luminositySlider, _yawSlider, _pitchSlider,
        _rollSlider, _cullingCheck, _lodCheck, _octreeOverlayCheck, _octreeOverlayDepthSpin,
        _octreeOverlayOpacitySpin, _gpuTelemetryCheck, _exportButton);
    sidebarTabs->addTab(runPage, "Run");
    sidebarTabs->addTab(scenePage, "Scene");
    sidebarTabs->addTab(physicsPage, "Physics");
    sidebarTabs->addTab(renderPage, "Render");
    return sidebarTabs;
}
} // namespace bltzr_qt
