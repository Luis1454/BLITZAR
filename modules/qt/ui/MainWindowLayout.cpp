/*
 * @file modules/qt/ui/MainWindowLayout.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "ui/EnergyGraphWidget.hpp"
#include "ui/MainWindow.hpp"
#include "ui/MultiViewWidget.hpp"
#include "ui/UiEnums.hpp"
#include "ui/panels/RunControlPanel.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSizePolicy>
#include <QSlider>
#include <QSpinBox>
#include <QStringList>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

namespace grav_qt {
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
    auto* runLayout = new QVBoxLayout(runPage);
    auto* scenePage = new QWidget(sidebarTabs);
    auto* sceneLayout = new QVBoxLayout(scenePage);
    auto* physicsPage = new QWidget(sidebarTabs);
    auto* physicsLayout = new QVBoxLayout(physicsPage);
    auto* renderPage = new QWidget(sidebarTabs);
    auto* renderLayout = new QVBoxLayout(renderPage);
    for (QVBoxLayout* layout : {runLayout, sceneLayout, physicsLayout, renderLayout}) {
        layout->setContentsMargins(4, 4, 4, 4);
        layout->setSpacing(8);
    }
    RunControlPanel::build(runPage, _performanceCombo, _pauseButton, _stepButton, _resetButton,
                           _recoverButton, _serverHostEdit, _serverPortSpin, _serverBinEdit,
                           _serverAutostartCheck, _applyConnectorButton);
    auto* sceneBox = new QGroupBox("Scene Setup", scenePage);
    auto* sceneBoxLayout = new QVBoxLayout(sceneBox);
    auto* sceneForm = new QFormLayout();
    sceneForm->addRow("profile", _simulationProfileCombo);
    sceneForm->addRow("preset", _presetCombo);
    sceneBoxLayout->addLayout(sceneForm);
    sceneBoxLayout->addWidget(_applyPresetButton);
    sceneBoxLayout->addWidget(_loadPresetButton);
    sceneBoxLayout->addWidget(_loadInputButton);
    auto* projectBox = new QGroupBox("Project", scenePage);
    auto* projectLayout = new QVBoxLayout(projectBox);
    projectLayout->addWidget(_saveConfigButton);
    projectLayout->addStretch(1);
    auto* physicsCoreBox = new QGroupBox("Physics Core", physicsPage);
    auto* physicsCoreLayout = new QVBoxLayout(physicsCoreBox);
    auto* physicsForm = new QFormLayout();
    physicsForm->addRow("solver", _solverCombo);
    physicsForm->addRow("integrator", _integratorCombo);
    physicsForm->addRow("dt", _dtSpin);
    physicsForm->addRow("theta", _thetaSpin);
    physicsForm->addRow("softening", _softeningSpin);
    physicsCoreLayout->addLayout(physicsForm);
    auto* sphBox = new QGroupBox("SPH", physicsPage);
    auto* sphLayout = new QVBoxLayout(sphBox);
    auto* sphForm = new QFormLayout();
    sphForm->addRow("h", _sphSmoothingSpin);
    sphForm->addRow("rest density", _sphRestDensitySpin);
    sphForm->addRow("gas K", _sphGasConstantSpin);
    sphForm->addRow("viscosity", _sphViscositySpin);
    sphLayout->addWidget(_sphCheck);
    sphLayout->addLayout(sphForm);
    auto* cameraBox = new QGroupBox("View & Camera", renderPage);
    auto* cameraLayout = new QGridLayout(cameraBox);
    cameraLayout->addWidget(new QLabel("zoom", this), 0, 0);
    cameraLayout->addWidget(_zoomSlider, 0, 1);
    cameraLayout->addWidget(new QLabel("luminosity", this), 0, 2);
    cameraLayout->addWidget(_luminositySlider, 0, 3);
    cameraLayout->addWidget(new QLabel("3D view", this), 1, 0);
    cameraLayout->addWidget(_view3dCombo, 1, 1);
    cameraLayout->addWidget(new QLabel("yaw", this), 1, 2);
    cameraLayout->addWidget(_yawSlider, 1, 3);
    cameraLayout->addWidget(new QLabel("pitch", this), 2, 0);
    cameraLayout->addWidget(_pitchSlider, 2, 1);
    cameraLayout->addWidget(new QLabel("roll", this), 2, 2);
    cameraLayout->addWidget(_rollSlider, 2, 3);
    cameraLayout->addWidget(_cullingCheck, 3, 0);
    cameraLayout->addWidget(_lodCheck, 3, 1);
    auto* overlayBox = new QGroupBox("Octree Overlay", renderPage);
    auto* overlayLayout = new QFormLayout(overlayBox);
    overlayLayout->addRow(_octreeOverlayCheck);
    overlayLayout->addRow("depth", _octreeOverlayDepthSpin);
    overlayLayout->addRow("opacity", _octreeOverlayOpacitySpin);
    auto* exportBox = new QGroupBox("Export", renderPage);
    auto* exportLayout = new QVBoxLayout(exportBox);
    exportLayout->addWidget(_exportButton);
    exportLayout->addWidget(_gpuTelemetryCheck);
    exportLayout->addStretch(1);
    // runPage populated by RunControlPanel::build
    sceneLayout->addWidget(sceneBox);
    sceneLayout->addWidget(projectBox);
    sceneLayout->addStretch(1);
    physicsLayout->addWidget(physicsCoreBox);
    physicsLayout->addWidget(sphBox);
    physicsLayout->addStretch(1);
    renderLayout->addWidget(cameraBox);
    renderLayout->addWidget(overlayBox);
    renderLayout->addWidget(exportBox);
    renderLayout->addStretch(1);
    sidebarTabs->addTab(runPage, "Run");
    sidebarTabs->addTab(scenePage, "Scene");
    sidebarTabs->addTab(physicsPage, "Physics");
    sidebarTabs->addTab(renderPage, "Render");
    return sidebarTabs;
}
} // namespace grav_qt
