/*
 * @file modules/qt/src/window/layout/Layout.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "window/core/Window.hpp"
#include "panels/control/Physics.hpp"
#include "panels/control/Render.hpp"
#include "panels/control/Run.hpp"
#include "panels/control/SceneSetup.hpp"
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
QTabWidget* Window::buildSidebarTabs()
{
    auto* sidebarTabs = new QTabWidget(this);
    sidebarTabs->setObjectName("workspaceSidebarTabs");
    sidebarTabs->setTabPosition(QTabWidget::West);
    sidebarTabs->setDocumentMode(true);
    sidebarTabs->setMinimumWidth(220);
    sidebarTabs->setMaximumWidth(248);
    sidebarTabs->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    auto* runPage = new QWidget(sidebarTabs);
    buildRunPanel(runPage, _widgets.run.performanceCombo, _widgets.run.pauseButton, _widgets.run.stepButton, _widgets.run.resetButton,
                  _widgets.run.recoverButton, _widgets.run.serverHostEdit, _widgets.run.serverPortSpin, _widgets.run.serverBinEdit,
                  _widgets.run.serverAutostartCheck, _widgets.run.applyConnectorButton);
    auto* scenePage = buildSceneSetupPanel(sidebarTabs, _widgets.scene.simulationProfileCombo, _widgets.scene.presetCombo,
                                           _widgets.scene.applyPresetButton, _widgets.scene.loadPresetButton,
                                           _widgets.scene.loadInputButton, _widgets.scene.saveConfigButton);
    auto* physicsPage = buildPhysicsPanel(
        sidebarTabs, _widgets.physics.solverCombo, _widgets.physics.integratorCombo, _widgets.physics.dtSpin, _widgets.physics.thetaSpin, _widgets.physics.softeningSpin, _widgets.physics.sphCheck,
        _widgets.physics.sphSmoothingSpin, _widgets.physics.sphRestDensitySpin, _widgets.physics.sphGasConstantSpin, _widgets.physics.sphViscositySpin);
    auto* renderPage = buildRenderPanel(
        sidebarTabs, _widgets.render.view3dCombo, _widgets.render.zoomSlider, _widgets.render.luminositySlider, _widgets.render.yawSlider, _widgets.render.pitchSlider,
        _widgets.render.rollSlider, _widgets.render.cullingCheck, _widgets.render.lodCheck, _widgets.render.octreeOverlayCheck, _widgets.render.octreeOverlayDepthSpin,
        _widgets.render.octreeOverlayOpacitySpin, _widgets.render.gpuTelemetryCheck, _widgets.scene.exportButton);
    sidebarTabs->addTab(runPage, "Run");
    sidebarTabs->addTab(scenePage, "Scene");
    sidebarTabs->addTab(physicsPage, "Physics");
    sidebarTabs->addTab(renderPage, "Render");
    return sidebarTabs;
}
} // namespace bltzr_qt
