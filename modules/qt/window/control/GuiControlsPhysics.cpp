/*
 * @file modules/qt/window/control/GuiControlsPhysics.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Physics, solver, TreePM, SPH, and adaptive-step controls.
 */

#include "core/constants/FndConstants.hpp"
#include "config/profile/profile/CfgMain.hpp"
#include "config/profile/profile/CfgPerformance.hpp"
#include "window/core/GuiWindow.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPointer>
#include <QSpinBox>
#include <QStatusBar>
#include <QWidget>
#include <algorithm>

namespace bltzr_qt {
void Window::applySphParameters()
{
    _config.sphSmoothingLength = static_cast<float>(_widgets.physics.sphSmoothingSpin->value());
    _config.sphRestDensity = static_cast<float>(_widgets.physics.sphRestDensitySpin->value());
    _config.sphGasConstant = static_cast<float>(_widgets.physics.sphGasConstantSpin->value());
    _config.sphViscosity = static_cast<float>(_widgets.physics.sphViscositySpin->value());
    _runtime->setSphParameters(_config.sphSmoothingLength, _config.sphRestDensity,
                               _config.sphGasConstant, _config.sphViscosity);
    markConfigDirty();
}

void Window::applyTreePmParameters()
{
    _runtime->setTreePmParameters(
        _config.treePmEnabled, _config.treePmModel, _config.treePmLayout, _config.treePmPrecision,
        _config.treePmAssignment, _config.treePmLocalGrid, _config.treePmGridSize,
        _config.treePmJacobiIterations, _config.treePmCutoffFactor,
        _config.treePmMaxLocalNeighbors, _config.treePmParticleLimit,
        _config.treePmDenseCellThreshold, _config.treePmGravityOnlyBuffers);
    markConfigDirty();
}

void Window::updateTreePmAvailability()
{
    const bool enabled = _widgets.physics.treePmEnabledCheck->isChecked();
    for (const QPointer<QWidget>& widget : {
             QPointer<QWidget>(_widgets.physics.treePmPresetCombo),
             QPointer<QWidget>(_widgets.physics.treePmModelCombo),
             QPointer<QWidget>(_widgets.physics.treePmLayoutCombo),
             QPointer<QWidget>(_widgets.physics.treePmPrecisionCombo),
             QPointer<QWidget>(_widgets.physics.treePmAssignmentCombo),
             QPointer<QWidget>(_widgets.physics.treePmLocalGridCheck),
             QPointer<QWidget>(_widgets.physics.treePmGridSizeSpin),
             QPointer<QWidget>(_widgets.physics.treePmJacobiIterationsSpin),
             QPointer<QWidget>(_widgets.physics.treePmCutoffFactorSpin),
             QPointer<QWidget>(_widgets.physics.treePmMaxLocalNeighborsSpin),
             QPointer<QWidget>(_widgets.physics.treePmParticleLimitSpin),
             QPointer<QWidget>(_widgets.physics.treePmDenseCellThresholdSpin),
             QPointer<QWidget>(_widgets.physics.treePmGravityOnlyBuffersCheck)}) {
        widget->setEnabled(enabled);
    }
    const bool localGridRelevant = enabled &&
                                   _widgets.physics.treePmModelCombo->currentText() != "exact_tree";
    _widgets.physics.treePmLocalGridCheck->setEnabled(localGridRelevant);
}

void Window::updateIntegratorAvailability()
{
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
}

void Window::connectSphControls()
{
    connect(_widgets.physics.sphCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        _config.sphEnabled = enabled;
        _runtime->setSphEnabled(enabled);
        markConfigDirty();
    });
    connect(_widgets.physics.sphSmoothingSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double) { applySphParameters(); });
    connect(_widgets.physics.sphRestDensitySpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double) { applySphParameters(); });
    connect(_widgets.physics.sphGasConstantSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double) { applySphParameters(); });
    connect(_widgets.physics.sphViscositySpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double) { applySphParameters(); });
    connect(_widgets.physics.dtSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                const float clampedDt =
                    std::clamp(static_cast<float>(value), kUiSimulationDtMin, kMaxStableInteractiveDt);
                _config.dt = clampedDt;
                _runtime->setDt(clampedDt);
                markConfigDirty();
            });
}

void Window::connectSolverControls()
{
    connect(_widgets.physics.solverCombo, &QComboBox::currentTextChanged, this,
            [this](const QString& solver) {
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
}

void Window::connectTreePmControls()
{
    connect(_widgets.physics.treePmEnabledCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        _config.treePmEnabled = enabled;
        applyTreePmParameters();
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
            [this](const QString& model) {
                _config.treePmModel = model.toStdString();
                _config.treePmPreset = "custom";
                applyTreePmParameters();
                updateTreePmAvailability();
            });
    connect(_widgets.physics.treePmLayoutCombo, &QComboBox::currentTextChanged, this,
            [this](const QString& layout) {
                _config.treePmLayout = layout.toStdString();
                _config.treePmPreset = "custom";
                applyTreePmParameters();
            });
    connect(_widgets.physics.treePmPrecisionCombo, &QComboBox::currentTextChanged, this,
            [this](const QString& precision) {
                _config.treePmPrecision = precision.toStdString();
                _config.treePmPreset = "custom";
                applyTreePmParameters();
            });
    connect(_widgets.physics.treePmAssignmentCombo, &QComboBox::currentTextChanged, this,
            [this](const QString& assignment) {
                _config.treePmAssignment = assignment.toStdString();
                _config.treePmPreset = "custom";
                applyTreePmParameters();
            });
    connect(_widgets.physics.treePmLocalGridCheck, &QCheckBox::toggled, this,
            [this](bool enabled) {
                _config.treePmLocalGrid = enabled;
                _config.treePmPreset = "custom";
                applyTreePmParameters();
            });
    connect(_widgets.physics.treePmGridSizeSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [this](int value) {
                _config.treePmGridSize = static_cast<std::uint32_t>(value);
                _config.treePmPreset = "custom";
                applyTreePmParameters();
            });
    connect(_widgets.physics.treePmJacobiIterationsSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [this](int value) {
                _config.treePmJacobiIterations = static_cast<std::uint32_t>(value);
                _config.treePmPreset = "custom";
                applyTreePmParameters();
            });
    connect(_widgets.physics.treePmCutoffFactorSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                _config.treePmCutoffFactor = static_cast<float>(value);
                _config.treePmPreset = "custom";
                applyTreePmParameters();
            });
    connect(_widgets.physics.treePmMaxLocalNeighborsSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [this](int value) {
                _config.treePmMaxLocalNeighbors = static_cast<std::uint32_t>(value);
                _config.treePmPreset = "custom";
                applyTreePmParameters();
            });
    connect(_widgets.physics.treePmParticleLimitSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [this](int value) {
                _config.treePmParticleLimit = static_cast<std::uint32_t>(value);
                _config.treePmPreset = "custom";
                applyTreePmParameters();
            });
    connect(_widgets.physics.treePmDenseCellThresholdSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [this](int value) {
                _config.treePmDenseCellThreshold = static_cast<std::uint32_t>(value);
                _config.treePmPreset = "custom";
                applyTreePmParameters();
            });
    connect(_widgets.physics.treePmGravityOnlyBuffersCheck, &QCheckBox::toggled, this,
            [this](bool enabled) {
                _config.treePmGravityOnlyBuffers = enabled;
                _config.treePmPreset = "custom";
                applyTreePmParameters();
            });
}

void Window::connectAdaptiveControls()
{
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
}

void Window::connectOctreeControls()
{
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
}

void Window::connectPhysicsControls()
{
    connectSphControls();
    connectSolverControls();
    connectTreePmControls();
    connectAdaptiveControls();
    connectOctreeControls();
    updateTreePmAvailability();
    updateIntegratorAvailability();
}
} // namespace bltzr_qt
