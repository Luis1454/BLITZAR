/*
 * @file modules/qt/src/window/control/Controls.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "Constants.hpp"
#include "config/profile/Performance.hpp"
#include "config/profile/Main.hpp"
#include "window/core/Window.hpp"
#include "widgets/viewport/MultiView.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QTimer>
#include <algorithm>

namespace bltzr_qt {
void Window::connectControls()
{
    const auto applySphParams = [this]() {
        _config.sphSmoothingLength = static_cast<float>(_widgets.physics.sphSmoothingSpin->value());
        _config.sphRestDensity = static_cast<float>(_widgets.physics.sphRestDensitySpin->value());
        _config.sphGasConstant = static_cast<float>(_widgets.physics.sphGasConstantSpin->value());
        _config.sphViscosity = static_cast<float>(_widgets.physics.sphViscositySpin->value());
        _runtime->setSphParameters(_config.sphSmoothingLength, _config.sphRestDensity,
                                   _config.sphGasConstant, _config.sphViscosity);
        markConfigDirty();
    };
    connect(_widgets.run.pauseButton, &QPushButton::clicked, this, [this](bool checked) {
        _runtime->setPaused(checked);
        _widgets.run.pauseButton->setText(checked ? "Resume" : "Pause");
    });
    connect(_widgets.run.stepButton, &QPushButton::clicked, this, [this]() {
        _runtime->stepOnce();
    });
    connect(_widgets.run.resetButton, &QPushButton::clicked, this, [this]() {
        resetSimulationFromUi();
    });
    connect(_widgets.run.recoverButton, &QPushButton::clicked, this, [this]() {
        _runtime->requestRecover();
    });
    connect(_widgets.run.applyConnectorButton, &QPushButton::clicked, this, [this]() {
        applyConnectorSettings(true);
    });
    connect(_widgets.scene.exportButton, &QPushButton::clicked, this, [this]() {
        handleExportRequest();
    });
    connect(_widgets.scene.saveConfigButton, &QPushButton::clicked, this, [this]() {
        (void)saveConfigToDisk();
    });
    connect(_widgets.scene.loadInputButton, &QPushButton::clicked, this, [this]() {
        handleLoadInputRequest();
    });
    connect(_widgets.scene.applyPresetButton, &QPushButton::clicked, this, [this]() {
        _config.initConfigStyle = "preset";
        _config.presetStructure = _widgets.scene.presetCombo->currentText().toStdString();
        if (_config.presetStructure != "file") {
            _config.initMode = _config.presetStructure;
        }
        applyConfigToServer(true);
        markConfigDirty();
        statusBar()->showMessage(
            QString("Scene preset applied: %1").arg(_widgets.scene.presetCombo->currentText()), 3000);
    });
    connect(_widgets.scene.loadPresetButton, &QPushButton::clicked, this, [this]() {
        handleLoadPresetRequest();
    });
    connect(_widgets.scene.simulationProfileCombo, &QComboBox::currentTextChanged, this,
            [this](const QString& profile) {
                _config.simulationProfile = profile.toStdString();
                bltzr_config::applySimulationProfile(_config);
                applyConfigToUi();
                (void)applyConfigToServer(true);
                markConfigDirty();
                statusBar()->showMessage(QString("Simulation profile applied: %1").arg(profile),
                                         3000);
            });
    connect(_widgets.physics.sphCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        _config.sphEnabled = enabled;
        _runtime->setSphEnabled(enabled);
        markConfigDirty();
    });
    connect(_widgets.physics.sphSmoothingSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [applySphParams](double) {
                applySphParams();
            });
    connect(_widgets.physics.sphRestDensitySpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [applySphParams](double) {
                applySphParams();
            });
    connect(_widgets.physics.sphGasConstantSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [applySphParams](double) {
                applySphParams();
            });
    connect(_widgets.physics.sphViscositySpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [applySphParams](double) {
                applySphParams();
            });
    connect(_widgets.physics.dtSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        const float clampedDt =
            std::clamp(static_cast<float>(value), kUiSimulationDtMin, kMaxStableInteractiveDt);
        _config.dt = clampedDt;
        _runtime->setDt(clampedDt);
        markConfigDirty();
    });
    connect(_widgets.render.zoomSlider, &QSlider::valueChanged, this, [this](int value) {
        _config.defaultZoom = static_cast<float>(value) / kZoomSliderDivisor;
        _widgets.view.multiView->setZoom(static_cast<float>(value) / kZoomSliderDivisor);
        markConfigDirty();
    });
    connect(_widgets.render.luminositySlider, &QSlider::valueChanged, this, [this](int value) {
        _config.defaultLuminosity = value;
        _widgets.view.multiView->setLuminosity(value);
        markConfigDirty();
    });
    connect(_widgets.render.octreeOverlayDepthSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [this](int value) {
                _widgets.view.multiView->setOctreeOverlay(_widgets.render.octreeOverlayCheck->isChecked(), value,
                                             _widgets.render.octreeOverlayOpacitySpin->value());
                statusBar()->showMessage(QString("Octree overlay depth: %1").arg(value), 2000);
            });
    connect(_widgets.render.octreeOverlayOpacitySpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [this](int value) {
                _widgets.view.multiView->setOctreeOverlay(_widgets.render.octreeOverlayCheck->isChecked(),
                                             _widgets.render.octreeOverlayDepthSpin->value(), value);
                statusBar()->showMessage(QString("Octree overlay opacity: %1").arg(value), 2000);
            });
    connect(_widgets.physics.solverCombo, &QComboBox::currentTextChanged, this, [this](const QString& solver) {
        _config.solver = solver.toStdString();
        _runtime->setSolverMode(_config.solver);
        markConfigDirty();
    });
    connect(_widgets.physics.integratorCombo, &QComboBox::currentTextChanged, this,
            [this](const QString& integrator) {
                _config.integrator = integrator.toStdString();
                _runtime->setIntegratorMode(_config.integrator);
                markConfigDirty();
            });
    connect(_widgets.run.performanceCombo, &QComboBox::currentTextChanged, this,
            [this](const QString& profile) {
                _config.performanceProfile = profile.toStdString();
                bltzr_config::applyPerformanceProfile(_config);
                applyConfigToUi();
                applyPerformanceProfileToRuntime();
                markConfigDirty();
                statusBar()->showMessage(QString("Run profile applied: %1").arg(profile), 3000);
            });
    connect(_widgets.physics.thetaSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                _config.octreeTheta = static_cast<float>(value);
                _runtime->setOctreeParameters(_config.octreeTheta,
                                              static_cast<float>(_widgets.physics.softeningSpin->value()));
                markConfigDirty();
            });
    connect(_widgets.physics.softeningSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                _config.octreeSoftening = static_cast<float>(value);
                _runtime->setOctreeParameters(static_cast<float>(_widgets.physics.thetaSpin->value()),
                                              _config.octreeSoftening);
                markConfigDirty();
            });
    connect(_widgets.render.view3dCombo, &QComboBox::currentTextChanged, this, [this](const QString& value) {
        _widgets.view.multiView->set3DMode(value == "iso" ? grav::ViewMode::Iso : grav::ViewMode::Perspective);
    });
    connect(_widgets.render.cullingCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        _config.renderCullingEnabled = enabled;
        _widgets.view.multiView->setRenderSettings(_config.renderCullingEnabled, _config.renderLODEnabled,
                                      _config.renderLODNearDistance, _config.renderLODFarDistance);
        markConfigDirty();
    });
    connect(_widgets.render.lodCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        _config.renderLODEnabled = enabled;
        _widgets.view.multiView->setRenderSettings(_config.renderCullingEnabled, _config.renderLODEnabled,
                                      _config.renderLODNearDistance, _config.renderLODFarDistance);
        markConfigDirty();
    });
    connect(_widgets.render.yawSlider, &QSlider::valueChanged, this, [this]() {
        update3DCameraFromSliders();
    });
    connect(_widgets.render.pitchSlider, &QSlider::valueChanged, this, [this]() {
        update3DCameraFromSliders();
    });
    connect(_widgets.render.rollSlider, &QSlider::valueChanged, this, [this]() {
        update3DCameraFromSliders();
    });
    connect(_widgets.workspace.timer, &QTimer::timeout, this, [this]() {
        tick();
    });
}
} // namespace bltzr_qt
