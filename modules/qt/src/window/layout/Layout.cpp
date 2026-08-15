/*
 * @file modules/qt/src/window/layout/Layout.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "window/core/Window.hpp"
#include "window/config/ConfigurationEditor.hpp"
#include "window/scene/SceneEditor.hpp"
#include "panels/control/Physics.hpp"
#include "panels/control/Render.hpp"
#include "panels/control/Run.hpp"
#include "panels/control/SceneSetup.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QStatusBar>
#include <QSizePolicy>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>
#include <QWidget>
#include <QVBoxLayout>

namespace bltzr_qt {
QTabWidget* Window::buildSidebarTabs()
{
    auto* sidebarTabs = new QTabWidget(this);
    sidebarTabs->setObjectName("workspaceSidebarTabs");
    sidebarTabs->setTabPosition(QTabWidget::West);
    sidebarTabs->setDocumentMode(true);
    sidebarTabs->setMinimumWidth(320);
    sidebarTabs->setMaximumWidth(440);
    sidebarTabs->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    auto* runPage = new QWidget(sidebarTabs);
    buildRunPanel(runPage, _widgets.run.performanceCombo, _widgets.run.pauseButton, _widgets.run.stepButton, _widgets.run.resetButton,
                  _widgets.run.recoverButton, _widgets.run.serverHostEdit, _widgets.run.serverPortSpin, _widgets.run.serverBinEdit,
                  _widgets.run.serverAutostartCheck, _widgets.run.applyConnectorButton);
    auto* scenePage = new QWidget(sidebarTabs);
    auto* sceneLayout = new QVBoxLayout(scenePage);
    auto* legacySceneSetup = buildSceneSetupPanel(
        scenePage, _widgets.scene.simulationProfileCombo, _widgets.scene.presetCombo,
        _widgets.scene.applyPresetButton, _widgets.scene.loadPresetButton,
        _widgets.scene.loadInputButton, _widgets.scene.saveConfigButton);
    sceneLayout->addWidget(legacySceneSetup);
    _sceneEditor = new SceneEditor(_config, scenePage);
    sceneLayout->addWidget(_sceneEditor, 1);
    auto* physicsPage = buildPhysicsPanel(sidebarTabs, _widgets.physics);
    auto* renderPage = buildRenderPanel(
        sidebarTabs, _widgets.render.view3dCombo, _widgets.render.zoomSlider, _widgets.render.luminositySlider, _widgets.render.yawSlider, _widgets.render.pitchSlider,
        _widgets.render.rollSlider, _widgets.render.cullingCheck, _widgets.render.lodCheck, _widgets.render.octreeOverlayCheck, _widgets.render.octreeOverlayDepthSpin,
        _widgets.render.octreeOverlayOpacitySpin, _widgets.render.gpuTelemetryCheck,
        _widgets.scene.exportButton, _widgets.render.exportProgress);
    _configurationEditor = new ConfigurationEditor(_config, sidebarTabs);
    _configurationEditor->setWindowFlags(Qt::Widget);
    _configurationEditor->setObjectName("configurationEditorPanel");
    connect(_configurationEditor, &QDialog::accepted, this, [this] {
        if (_configurationEditor == nullptr)
            return;
        const SimulationConfig candidate = _configurationEditor->configuration();
        (void)applyEditedConfiguration(candidate);
        _configurationEditor->show();
    });
    connect(_configurationEditor, &QDialog::rejected, this, [this] {
        if (_configurationEditor != nullptr)
            _configurationEditor->show();
    });
    const auto applyStructuredScene = [this](const QString& message) {
        captureUiIntoConfig();
        if (!applyConfigToServer(true, false))
            return;
        if (_sceneEditor != nullptr)
            _sceneEditor->reload(_config);
        markConfigDirty();
        statusBar()->showMessage(message, 3000);
    };
    connect(_sceneEditor->applyButton(), &QPushButton::clicked, this, [applyStructuredScene] {
        applyStructuredScene("Scene applied");
    });
    sidebarTabs->addTab(runPage, "Run");
    sidebarTabs->addTab(scenePage, "Scene");
    sidebarTabs->addTab(_configurationEditor, "Config");
    sidebarTabs->addTab(physicsPage, "Physics");
    sidebarTabs->addTab(renderPage, "Render");
    connect(sidebarTabs, &QTabWidget::currentChanged, this, [this, sidebarTabs](int index) {
        // Capture pending edits before switching editors.
        captureUiIntoConfig();
        if (_sceneEditor != nullptr && index == sidebarTabs->indexOf(_sceneEditor->parentWidget()))
            _sceneEditor->reload(_config);
    });
    return sidebarTabs;
}
} // namespace bltzr_qt
