/*
 * @file modules/qt/src/window/control/Controls.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "Constants.hpp"
#include "profile/Performance.hpp"
#include "profile/Main.hpp"
#include "window/core/Window.hpp"
#include "window/scene/SceneEditor.hpp"
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
    const auto applyTreePmParams = [this]() {
        _runtime->setTreePmParameters(
            _config.treePmEnabled, _config.treePmModel, _config.treePmLayout, _config.treePmPrecision,
            _config.treePmAssignment, _config.treePmLocalGrid, _config.treePmGridSize,
            _config.treePmJacobiIterations, _config.treePmCutoffFactor,
            _config.treePmMaxLocalNeighbors, _config.treePmParticleLimit,
            _config.treePmDenseCellThreshold, _config.treePmGravityOnlyBuffers);
        markConfigDirty();
    };
    const auto updateTreePmAvailability = [this]() {
        const bool enabled = _widgets.physics.treePmEnabledCheck->isChecked();
        for (QWidget* widget : {static_cast<QWidget*>(_widgets.physics.treePmPresetCombo),
                                static_cast<QWidget*>(_widgets.physics.treePmModelCombo),
                                static_cast<QWidget*>(_widgets.physics.treePmLayoutCombo),
                                static_cast<QWidget*>(_widgets.physics.treePmPrecisionCombo),
                                static_cast<QWidget*>(_widgets.physics.treePmAssignmentCombo),
                                static_cast<QWidget*>(_widgets.physics.treePmLocalGridCheck),
                                static_cast<QWidget*>(_widgets.physics.treePmGridSizeSpin),
                                static_cast<QWidget*>(_widgets.physics.treePmJacobiIterationsSpin),
                                static_cast<QWidget*>(_widgets.physics.treePmCutoffFactorSpin),
                                static_cast<QWidget*>(_widgets.physics.treePmMaxLocalNeighborsSpin),
                                static_cast<QWidget*>(_widgets.physics.treePmParticleLimitSpin),
                                static_cast<QWidget*>(_widgets.physics.treePmDenseCellThresholdSpin),
                                static_cast<QWidget*>(_widgets.physics.treePmGravityOnlyBuffersCheck)}) {
            widget->setEnabled(enabled);
        }
        const bool localGridRelevant = enabled &&
                                       _widgets.physics.treePmModelCombo->currentText() != "exact_tree";
        _widgets.physics.treePmLocalGridCheck->setEnabled(localGridRelevant);
    };
    const auto updateIntegratorAvailability = [this]() {
        const bool gpuOctree = _widgets.physics.solverCombo->currentText() == "octree_gpu";
        if (gpuOctree && _widgets.physics.integratorCombo->currentText() == "rk4") {
            _widgets.physics.integratorCombo->blockSignals(true);
            _widgets.physics.integratorCombo->setCurrentText("euler");
            _widgets.physics.integratorCombo->blockSignals(false);
            _config.integrator = "euler";
            _runtime->setIntegratorMode("euler");
        }
        _widgets.physics.integratorCombo->setToolTip(
            gpuOctree ? "octree_gpu supports Euler and Leapfrog; RK4 is unavailable"
                      : "Choose the time integration scheme");
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
        _config.scene.objects.clear();
        if (_sceneEditor != nullptr)
            _sceneEditor->reload(_config);
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
        _config.defaultZoom = zoomFromSliderValue(value);
        _widgets.view.multiView->setZoom(_config.defaultZoom);
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
    connect(_widgets.physics.solverCombo, &QComboBox::currentTextChanged, this,
            [this, updateIntegratorAvailability](const QString& solver) {
                _config.solver = solver.toStdString();
                _runtime->setSolverMode(_config.solver);
                updateIntegratorAvailability();
                markConfigDirty();
            });
    connect(_widgets.physics.integratorCombo, &QComboBox::currentTextChanged, this,
            [this](const QString& integrator) {
                _config.integrator = integrator.toStdString();
                _runtime->setIntegratorMode(_config.integrator);
                markConfigDirty();
            });
    connect(_widgets.physics.particleCountSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [this](int value) {
                _config.particleCount = static_cast<std::uint32_t>(value);
                _runtime->setParticleCount(_config.particleCount);
                markConfigDirty();
            });
    connect(_widgets.physics.treePmEnabledCheck, &QCheckBox::toggled, this,
            [this, applyTreePmParams, updateTreePmAvailability](bool enabled) {
                _config.treePmEnabled = enabled;
                applyTreePmParams();
                updateTreePmAvailability();
            });
    connect(_widgets.physics.treePmPresetCombo, &QComboBox::currentTextChanged, this,
            [this](const QString& preset) {
                _config.treePmPreset = preset.toStdString();
                bltzr_config::applyTreePmPreset(_config);
                applyConfigToUi();
                (void)applyConfigToServer(true);
                markConfigDirty();
                statusBar()->showMessage(QString("TreePM preset applied: %1").arg(preset), 3000);
            });
    connect(_widgets.physics.treePmModelCombo, &QComboBox::currentTextChanged, this,
            [this, applyTreePmParams, updateTreePmAvailability](const QString& model) {
                _config.treePmModel = model.toStdString();
                _config.treePmPreset = "custom";
                applyTreePmParams();
                updateTreePmAvailability();
            });
    connect(_widgets.physics.treePmLayoutCombo, &QComboBox::currentTextChanged, this,
            [this, applyTreePmParams](const QString& layout) {
                _config.treePmLayout = layout.toStdString();
                _config.treePmPreset = "custom";
                applyTreePmParams();
            });
    connect(_widgets.physics.treePmPrecisionCombo, &QComboBox::currentTextChanged, this,
            [this, applyTreePmParams](const QString& precision) {
                _config.treePmPrecision = precision.toStdString();
                _config.treePmPreset = "custom";
                applyTreePmParams();
            });
    connect(_widgets.physics.treePmAssignmentCombo, &QComboBox::currentTextChanged, this,
            [this, applyTreePmParams](const QString& assignment) {
                _config.treePmAssignment = assignment.toStdString();
                _config.treePmPreset = "custom";
                applyTreePmParams();
            });
    connect(_widgets.physics.treePmLocalGridCheck, &QCheckBox::toggled, this,
            [this, applyTreePmParams](bool enabled) {
                _config.treePmLocalGrid = enabled;
                _config.treePmPreset = "custom";
                applyTreePmParams();
            });
    connect(_widgets.physics.treePmGridSizeSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [this, applyTreePmParams](int value) {
                _config.treePmGridSize = static_cast<std::uint32_t>(value);
                _config.treePmPreset = "custom";
                applyTreePmParams();
            });
    connect(_widgets.physics.treePmJacobiIterationsSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [this, applyTreePmParams](int value) {
                _config.treePmJacobiIterations = static_cast<std::uint32_t>(value);
                _config.treePmPreset = "custom";
                applyTreePmParams();
            });
    connect(_widgets.physics.treePmCutoffFactorSpin,
            qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this, applyTreePmParams](double value) {
                _config.treePmCutoffFactor = static_cast<float>(value);
                _config.treePmPreset = "custom";
                applyTreePmParams();
            });
    connect(_widgets.physics.treePmMaxLocalNeighborsSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [this, applyTreePmParams](int value) {
                _config.treePmMaxLocalNeighbors = static_cast<std::uint32_t>(value);
                _config.treePmPreset = "custom";
                applyTreePmParams();
            });
    connect(_widgets.physics.treePmParticleLimitSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [this, applyTreePmParams](int value) {
                _config.treePmParticleLimit = static_cast<std::uint32_t>(value);
                _config.treePmPreset = "custom";
                applyTreePmParams();
            });
    connect(_widgets.physics.treePmDenseCellThresholdSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [this, applyTreePmParams](int value) {
                _config.treePmDenseCellThreshold = static_cast<std::uint32_t>(value);
                _config.treePmPreset = "custom";
                applyTreePmParams();
            });
    connect(_widgets.physics.treePmGravityOnlyBuffersCheck, &QCheckBox::toggled, this,
            [this, applyTreePmParams](bool enabled) {
                _config.treePmGravityOnlyBuffers = enabled;
                _config.treePmPreset = "custom";
                applyTreePmParams();
            });
    connect(_widgets.physics.adaptiveTimeStepsCheck, &QCheckBox::toggled, this,
            [this](bool enabled) {
                _config.adaptiveTimeStepsEnabled = enabled;
                _runtime->setAdaptiveTimeSteps(
                    enabled, _config.adaptiveTimeStepMaxLevel, _config.adaptiveTimeStepEta);
                markConfigDirty();
            });
    connect(_widgets.physics.adaptiveMaxLevelSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [this](int level) {
                _config.adaptiveTimeStepMaxLevel = static_cast<std::uint32_t>(level);
                _runtime->setAdaptiveTimeSteps(
                    _config.adaptiveTimeStepsEnabled, _config.adaptiveTimeStepMaxLevel,
                    _config.adaptiveTimeStepEta);
                markConfigDirty();
            });
    connect(_widgets.physics.adaptiveEtaSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double eta) {
                _config.adaptiveTimeStepEta = static_cast<float>(eta);
                _runtime->setAdaptiveTimeSteps(
                    _config.adaptiveTimeStepsEnabled, _config.adaptiveTimeStepMaxLevel,
                    _config.adaptiveTimeStepEta);
                markConfigDirty();
            });
    connect(_widgets.physics.adaptiveCostGuardCheck, &QCheckBox::toggled, this,
            [this](bool enabled) {
                _config.adaptiveTimeStepCostGuard = enabled;
                _runtime->setAdaptiveTimeStepCostGuard(enabled);
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
    updateTreePmAvailability();
    updateIntegratorAvailability();
}
} // namespace bltzr_qt
