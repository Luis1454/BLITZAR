// File: modules/qt/ui/MainWindowControls.cpp
// Purpose: Client module implementation for BLITZAR extension workflows.

#include "config/SimulationPerformanceProfile.hpp"
#include "config/SimulationProfile.hpp"
#include "ui/MainWindow.hpp"
#include "ui/MultiViewWidget.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QTimer>
namespace grav_qt {
/// Description: Executes the connectControls operation.
void MainWindow::connectControls()
{
    const auto applySphParams = [this]() {
        _config.sphSmoothingLength = static_cast<float>(_sphSmoothingSpin->value());
        _config.sphRestDensity = static_cast<float>(_sphRestDensitySpin->value());
        _config.sphGasConstant = static_cast<float>(_sphGasConstantSpin->value());
        _config.sphViscosity = static_cast<float>(_sphViscositySpin->value());
        _runtime->setSphParameters(_config.sphSmoothingLength, _config.sphRestDensity,
                                   _config.sphGasConstant, _config.sphViscosity);
        /// Description: Executes the markConfigDirty operation.
        markConfigDirty();
    };
    /// Description: Executes the connect operation.
    connect(_pauseButton, &QPushButton::clicked, this, [this](bool checked) {
        _runtime->setPaused(checked);
        _pauseButton->setText(checked ? "Resume" : "Pause");
    });
    /// Description: Executes the connect operation.
    connect(_stepButton, &QPushButton::clicked, this, [this]() { _runtime->stepOnce(); });
    /// Description: Executes the connect operation.
    connect(_resetButton, &QPushButton::clicked, this, [this]() { resetSimulationFromUi(); });
    /// Description: Executes the connect operation.
    connect(_recoverButton, &QPushButton::clicked, this, [this]() { _runtime->requestRecover(); });
    connect(_applyConnectorButton, &QPushButton::clicked, this,
            [this]() { applyConnectorSettings(true); });
    /// Description: Executes the connect operation.
    connect(_exportButton, &QPushButton::clicked, this, [this]() { handleExportRequest(); });
    /// Description: Executes the connect operation.
    connect(_saveConfigButton, &QPushButton::clicked, this, [this]() { (void)saveConfigToDisk(); });
    /// Description: Executes the connect operation.
    connect(_loadInputButton, &QPushButton::clicked, this, [this]() { handleLoadInputRequest(); });
    /// Description: Executes the connect operation.
    connect(_applyPresetButton, &QPushButton::clicked, this, [this]() {
        _config.initConfigStyle = "preset";
        _config.presetStructure = _presetCombo->currentText().toStdString();
        if (_config.presetStructure != "file") {
            _config.initMode = _config.presetStructure;
        }
        /// Description: Executes the applyConfigToServer operation.
        applyConfigToServer(true);
        /// Description: Executes the markConfigDirty operation.
        markConfigDirty();
        statusBar()->showMessage(
            /// Description: Executes the QString operation.
            QString("Scene preset applied: %1").arg(_presetCombo->currentText()), 3000);
    });
    connect(_loadPresetButton, &QPushButton::clicked, this,
            [this]() { handleLoadPresetRequest(); });
    connect(_simulationProfileCombo, &QComboBox::currentTextChanged, this,
            [this](const QString& profile) {
                _config.simulationProfile = profile.toStdString();
                /// Description: Executes the applySimulationProfile operation.
                grav_config::applySimulationProfile(_config);
                /// Description: Executes the applyConfigToUi operation.
                applyConfigToUi();
                (void)applyConfigToServer(true);
                /// Description: Executes the markConfigDirty operation.
                markConfigDirty();
                statusBar()->showMessage(QString("Simulation profile applied: %1").arg(profile),
                                         3000);
            });
    /// Description: Executes the connect operation.
    connect(_sphCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        _config.sphEnabled = enabled;
        _runtime->setSphEnabled(enabled);
        /// Description: Executes the markConfigDirty operation.
        markConfigDirty();
    });
    connect(_sphSmoothingSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [applySphParams](double) { applySphParams(); });
    connect(_sphRestDensitySpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [applySphParams](double) { applySphParams(); });
    connect(_sphGasConstantSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [applySphParams](double) { applySphParams(); });
    connect(_sphViscositySpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [applySphParams](double) { applySphParams(); });
    /// Description: Executes the connect operation.
    connect(_dtSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        _config.dt = static_cast<float>(value);
        _runtime->setDt(static_cast<float>(value));
        /// Description: Executes the markConfigDirty operation.
        markConfigDirty();
    });
    /// Description: Executes the connect operation.
    connect(_zoomSlider, &QSlider::valueChanged, this, [this](int value) {
        _config.defaultZoom = static_cast<float>(value) / 10.0f;
        _multiView->setZoom(static_cast<float>(value) / 10.0f);
        /// Description: Executes the markConfigDirty operation.
        markConfigDirty();
    });
    /// Description: Executes the connect operation.
    connect(_luminositySlider, &QSlider::valueChanged, this, [this](int value) {
        _config.defaultLuminosity = value;
        _multiView->setLuminosity(value);
        /// Description: Executes the markConfigDirty operation.
        markConfigDirty();
    });
    connect(_octreeOverlayDepthSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [this](int value) {
                _multiView->setOctreeOverlay(_octreeOverlayCheck->isChecked(), value,
                                             _octreeOverlayOpacitySpin->value());
                /// Description: Executes the statusBar operation.
                statusBar()->showMessage(QString("Octree overlay depth: %1").arg(value), 2000);
            });
    connect(_octreeOverlayOpacitySpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [this](int value) {
                _multiView->setOctreeOverlay(_octreeOverlayCheck->isChecked(),
                                             _octreeOverlayDepthSpin->value(), value);
                /// Description: Executes the statusBar operation.
                statusBar()->showMessage(QString("Octree overlay opacity: %1").arg(value), 2000);
            });
    /// Description: Executes the connect operation.
    connect(_solverCombo, &QComboBox::currentTextChanged, this, [this](const QString& solver) {
        _config.solver = solver.toStdString();
        _runtime->setSolverMode(_config.solver);
        /// Description: Executes the markConfigDirty operation.
        markConfigDirty();
    });
    connect(_integratorCombo, &QComboBox::currentTextChanged, this,
            [this](const QString& integrator) {
                _config.integrator = integrator.toStdString();
                _runtime->setIntegratorMode(_config.integrator);
                /// Description: Executes the markConfigDirty operation.
                markConfigDirty();
            });
    connect(_performanceCombo, &QComboBox::currentTextChanged, this,
            [this](const QString& profile) {
                _config.performanceProfile = profile.toStdString();
                /// Description: Executes the applyPerformanceProfile operation.
                grav_config::applyPerformanceProfile(_config);
                /// Description: Executes the applyConfigToUi operation.
                applyConfigToUi();
                /// Description: Executes the applyPerformanceProfileToRuntime operation.
                applyPerformanceProfileToRuntime();
                /// Description: Executes the markConfigDirty operation.
                markConfigDirty();
                /// Description: Executes the statusBar operation.
                statusBar()->showMessage(QString("Run profile applied: %1").arg(profile), 3000);
            });
    connect(_thetaSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                _config.octreeTheta = static_cast<float>(value);
                _runtime->setOctreeParameters(_config.octreeTheta,
                                              static_cast<float>(_softeningSpin->value()));
                /// Description: Executes the markConfigDirty operation.
                markConfigDirty();
            });
    connect(_softeningSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                _config.octreeSoftening = static_cast<float>(value);
                _runtime->setOctreeParameters(static_cast<float>(_thetaSpin->value()),
                                              _config.octreeSoftening);
                /// Description: Executes the markConfigDirty operation.
                markConfigDirty();
            });
    /// Description: Executes the connect operation.
    connect(_view3dCombo, &QComboBox::currentTextChanged, this, [this](const QString& value) {
        _multiView->set3DMode(value == "iso" ? grav::ViewMode::Iso : grav::ViewMode::Perspective);
    });
    /// Description: Executes the connect operation.
    connect(_cullingCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        _config.renderCullingEnabled = enabled;
        _multiView->setRenderSettings(_config.renderCullingEnabled, _config.renderLODEnabled,
                                      _config.renderLODNearDistance, _config.renderLODFarDistance);
        /// Description: Executes the markConfigDirty operation.
        markConfigDirty();
    });
    /// Description: Executes the connect operation.
    connect(_lodCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        _config.renderLODEnabled = enabled;
        _multiView->setRenderSettings(_config.renderCullingEnabled, _config.renderLODEnabled,
                                      _config.renderLODNearDistance, _config.renderLODFarDistance);
        /// Description: Executes the markConfigDirty operation.
        markConfigDirty();
    });
    /// Description: Executes the connect operation.
    connect(_yawSlider, &QSlider::valueChanged, this, [this]() { update3DCameraFromSliders(); });
    /// Description: Executes the connect operation.
    connect(_pitchSlider, &QSlider::valueChanged, this, [this]() { update3DCameraFromSliders(); });
    /// Description: Executes the connect operation.
    connect(_rollSlider, &QSlider::valueChanged, this, [this]() { update3DCameraFromSliders(); });
    /// Description: Executes the connect operation.
    connect(_timer, &QTimer::timeout, this, [this]() { tick(); });
}
} // namespace grav_qt
