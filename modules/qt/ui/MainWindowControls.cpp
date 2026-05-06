/*
 * @file modules/qt/ui/MainWindowControls.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "config/SimulationPerformanceProfile.hpp"
#include "config/SimulationProfile.hpp"
#include "ui/MainWindow.hpp"
#include "ui/MultiViewWidget.hpp"
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QTimer>

namespace bltzr_qt {

void MainWindow::resetSimulationFromUi()
{
    if (_runtime != nullptr) {
        _runtime->requestReset();
    }
    (void)applyConfigToServer(true);
    if (_statusLabel != nullptr) {
        _statusLabel->setText(QString("simulation reset"));
    }
}

void MainWindow::applyConnectorSettings(bool reconnectNow)
{
    if (_runtime == nullptr || _serverHostEdit == nullptr || _serverPortSpin == nullptr ||
        _serverAutostartCheck == nullptr || _serverBinEdit == nullptr) {
        return;
    }

    const std::string host = _serverHostEdit->text().toStdString();
    const std::uint16_t port = static_cast<std::uint16_t>(_serverPortSpin->value());
    const bool autoStart = _serverAutostartCheck->isChecked();
    const std::string executable = _serverBinEdit->text().toStdString();
    _runtime->configureRemoteConnector(host, port, autoStart, executable);
    if (reconnectNow) {
        _runtime->requestReconnect();
    }
    if (_statusLabel != nullptr) {
        _statusLabel->setText(QString("remote connector updated"));
    }
}

void MainWindow::connectControls()
{
    const auto applySphParams = [this]() {
        _config.sphSmoothingLength = static_cast<float>(_sphSmoothingSpin->value());
        _config.sphRestDensity = static_cast<float>(_sphRestDensitySpin->value());
        _config.sphGasConstant = static_cast<float>(_sphGasConstantSpin->value());
        _config.sphViscosity = static_cast<float>(_sphViscositySpin->value());
        _runtime->setSphParameters(_config.sphSmoothingLength, _config.sphRestDensity,
                                   _config.sphGasConstant, _config.sphViscosity);
        markConfigDirty();
    };
    connect(_pauseButton, &QPushButton::clicked, this, [this](bool checked) {
        _runtime->setPaused(checked);
        _pauseButton->setText(checked ? "Resume" : "Pause");
    });
    connect(_stepButton, &QPushButton::clicked, this, [this]() {
        _runtime->stepOnce();
    });
    connect(_resetButton, &QPushButton::clicked, this, [this]() {
        resetSimulationFromUi();
    });
    connect(_recoverButton, &QPushButton::clicked, this, [this]() {
        _runtime->requestRecover();
    });
    connect(_applyConnectorButton, &QPushButton::clicked, this, [this]() {
        applyConnectorSettings(true);
    });
    connect(_exportButton, &QPushButton::clicked, this, [this]() {
        handleExportRequest();
    });
    connect(_saveConfigButton, &QPushButton::clicked, this, [this]() {
        (void)saveConfigToDisk();
    });
    connect(_loadInputButton, &QPushButton::clicked, this, [this]() {
        handleLoadInputRequest();
    });
    connect(_applyPresetButton, &QPushButton::clicked, this, [this]() {
        _config.initConfigStyle = "preset";
        _config.presetStructure = _presetCombo->currentText().toStdString();
        if (_config.presetStructure != "file") {
            _config.initMode = _config.presetStructure;
        }
        applyConfigToServer(true);
        markConfigDirty();
        statusBar()->showMessage(
            QString("Scene preset applied: %1").arg(_presetCombo->currentText()), 3000);
    });
    connect(_loadPresetButton, &QPushButton::clicked, this, [this]() {
        handleLoadPresetRequest();
    });
    connect(_simulationProfileCombo, &QComboBox::currentTextChanged, this,
            [this](const QString& profile) {
                _config.simulationProfile = profile.toStdString();
                bltzr_config::applySimulationProfile(_config);
                applyConfigToUi();
                (void)applyConfigToServer(true);
                markConfigDirty();
                statusBar()->showMessage(QString("Simulation profile applied: %1").arg(profile),
                                         3000);
            });
    connect(_sphCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        _config.sphEnabled = enabled;
        _runtime->setSphEnabled(enabled);
        markConfigDirty();
    });
    connect(_sphSmoothingSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [applySphParams](double) {
                applySphParams();
            });
    connect(_sphRestDensitySpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [applySphParams](double) {
                applySphParams();
            });
    connect(_sphGasConstantSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [applySphParams](double) {
                applySphParams();
            });
    connect(_sphViscositySpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [applySphParams](double) {
                applySphParams();
            });
    connect(_dtSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        _config.dt = static_cast<float>(value);
        _runtime->setDt(static_cast<float>(value));
        markConfigDirty();
    });
    connect(_zoomSlider, &QSlider::valueChanged, this, [this](int value) {
        _config.defaultZoom = static_cast<float>(value) / 10.0f;
        _multiView->setZoom(static_cast<float>(value) / 10.0f);
        markConfigDirty();
    });
    connect(_luminositySlider, &QSlider::valueChanged, this, [this](int value) {
        _config.defaultLuminosity = value;
        _multiView->setLuminosity(value);
        markConfigDirty();
    });
    connect(_octreeOverlayDepthSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [this](int value) {
                _multiView->setOctreeOverlay(_octreeOverlayCheck->isChecked(), value,
                                             _octreeOverlayOpacitySpin->value());
                statusBar()->showMessage(QString("Octree overlay depth: %1").arg(value), 2000);
            });
    connect(_octreeOverlayOpacitySpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [this](int value) {
                _multiView->setOctreeOverlay(_octreeOverlayCheck->isChecked(),
                                             _octreeOverlayDepthSpin->value(), value);
                statusBar()->showMessage(QString("Octree overlay opacity: %1").arg(value), 2000);
            });
    connect(_solverCombo, &QComboBox::currentTextChanged, this, [this](const QString& solver) {
        _config.solver = solver.toStdString();
        _runtime->setSolverMode(_config.solver);
        markConfigDirty();
    });
    connect(_integratorCombo, &QComboBox::currentTextChanged, this,
            [this](const QString& integrator) {
                _config.integrator = integrator.toStdString();
                _runtime->setIntegratorMode(_config.integrator);
                markConfigDirty();
            });
    connect(_performanceCombo, &QComboBox::currentTextChanged, this,
            [this](const QString& profile) {
                _config.performanceProfile = profile.toStdString();
                bltzr_config::applyPerformanceProfile(_config);
                applyConfigToUi();
                applyPerformanceProfileToRuntime();
                markConfigDirty();
                statusBar()->showMessage(QString("Run profile applied: %1").arg(profile), 3000);
            });
    connect(_thetaSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                _config.octreeTheta = static_cast<float>(value);
                _runtime->setOctreeParameters(_config.octreeTheta,
                                              static_cast<float>(_softeningSpin->value()));
                markConfigDirty();
            });
    connect(_softeningSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                _config.octreeSoftening = static_cast<float>(value);
                _runtime->setOctreeParameters(static_cast<float>(_thetaSpin->value()),
                                              _config.octreeSoftening);
                markConfigDirty();
            });
    connect(_view3dCombo, &QComboBox::currentTextChanged, this, [this](const QString& value) {
        _multiView->set3DMode(value == "iso" ? grav::ViewMode::Iso : grav::ViewMode::Perspective);
    });
    connect(_cullingCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        _config.renderCullingEnabled = enabled;
        _multiView->setRenderSettings(_config.renderCullingEnabled, _config.renderLODEnabled,
                                      _config.renderLODNearDistance, _config.renderLODFarDistance);
        markConfigDirty();
    });
    connect(_lodCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        _config.renderLODEnabled = enabled;
        _multiView->setRenderSettings(_config.renderCullingEnabled, _config.renderLODEnabled,
                                      _config.renderLODNearDistance, _config.renderLODFarDistance);
        markConfigDirty();
    });
    connect(_yawSlider, &QSlider::valueChanged, this, [this]() {
        update3DCameraFromSliders();
    });
    connect(_pitchSlider, &QSlider::valueChanged, this, [this]() {
        update3DCameraFromSliders();
    });
    connect(_rollSlider, &QSlider::valueChanged, this, [this]() {
        update3DCameraFromSliders();
    });
    connect(_timer, &QTimer::timeout, this, [this]() {
        tick();
    });
}
} // namespace bltzr_qt
