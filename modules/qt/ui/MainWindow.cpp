#include "ui/MainWindow.hpp"

#include "client/ClientCommon.hpp"
#include "config/SimulationPerformanceProfile.hpp"
#include "server/SimulationInitConfig.hpp"
#include "ui/EnergyGraphWidget.hpp"
#include "ui/MultiViewWidget.hpp"
#include "ui/QtViewMath.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace grav_qt {

std::string formatFromSelectedFilter(const QString &filter)
{
    if (filter.startsWith("VTK ASCII")) {
        return "vtk";
    }
    if (filter.startsWith("VTK BINARY")) {
        return "vtk_binary";
    }
    if (filter.startsWith("XYZ")) {
        return "xyz";
    }
    if (filter.startsWith("Native binary")) {
        return "bin";
    }
    return {};
}
MainWindow::MainWindow(
    SimulationConfig config,
    std::string configPath,
    std::unique_ptr<grav_client::IClientRuntime> runtime)
    : QMainWindow(nullptr),
      _config(std::move(config)),
      _configPath(std::move(configPath)),
      _runtime(std::move(runtime)),
      _multiView(new MultiViewWidget()),
      _energyGraph(new EnergyGraphWidget()),
      _statusLabel(new QLabel(this)),
      _pauseButton(new QPushButton("Pause", this)),
      _stepButton(new QPushButton("Step", this)),
      _resetButton(new QPushButton("Reset", this)),
      _recoverButton(new QPushButton("Recover", this)),
      _reconnectButton(new QPushButton("Reconnect", this)),
      _applyConnectorButton(new QPushButton("Apply connector", this)),
      _exportButton(new QPushButton("Export", this)),
      _saveConfigButton(new QPushButton("Save config", this)),
      _loadInputButton(new QPushButton("Load input", this)),
      _serverAutostartCheck(new QCheckBox("autostart server", this)),
      _serverHostEdit(new QLineEdit(this)),
      _serverBinEdit(new QLineEdit(this)),
      _serverPortSpin(new QSpinBox(this)),
      _sphCheck(new QCheckBox("SPH", this)),
      _sphSmoothingSpin(new QDoubleSpinBox(this)),
      _sphRestDensitySpin(new QDoubleSpinBox(this)),
      _sphGasConstantSpin(new QDoubleSpinBox(this)),
      _sphViscositySpin(new QDoubleSpinBox(this)),
      _dtSpin(new QDoubleSpinBox(this)),
      _zoomSlider(new QSlider(Qt::Horizontal, this)),
      _luminositySlider(new QSlider(Qt::Horizontal, this)),
      _solverCombo(new QComboBox(this)),
      _integratorCombo(new QComboBox(this)),
      _performanceCombo(new QComboBox(this)),
      _presetCombo(new QComboBox(this)),
      _view3dCombo(new QComboBox(this)),
      _thetaSpin(new QDoubleSpinBox(this)),
      _softeningSpin(new QDoubleSpinBox(this)),
      _applyPresetButton(new QPushButton("Apply preset", this)),
      _loadPresetButton(new QPushButton("Load preset file", this)),
      _yawSlider(new QSlider(Qt::Horizontal, this)),
      _pitchSlider(new QSlider(Qt::Horizontal, this)),
      _rollSlider(new QSlider(Qt::Horizontal, this)),
      _cullingCheck(new QCheckBox("Culling", this)),
      _lodCheck(new QCheckBox("LOD", this)),
      _timer(new QTimer(this)),
      _lastEnergyStep(std::numeric_limits<std::uint64_t>::max()),
      _clientDrawCap(grav_client::resolveClientDrawCap(_config)),
      _uiTickFps(0.0f),
      _configDirty(false),
      _lastUiTickAt()
{
    if (!_runtime) {
        throw std::invalid_argument("grav_qt::MainWindow requires a valid client runtime");
    }

    setWindowTitle("N-Body Qt Client");

    auto *central = new QWidget(this);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(8);

    _pauseButton->setCheckable(true);
    _solverCombo->addItem("pairwise_cuda");
    _solverCombo->addItem("octree_gpu");
    _solverCombo->addItem("octree_cpu");
    const int solverIndex = std::max(0, _solverCombo->findText(QString::fromStdString(_config.solver)));
    _solverCombo->setCurrentIndex(solverIndex);
    _integratorCombo->addItem("euler");
    _integratorCombo->addItem("rk4");
    const int integratorIndex = std::max(0, _integratorCombo->findText(QString::fromStdString(_config.integrator)));
    _integratorCombo->setCurrentIndex(integratorIndex);
    _performanceCombo->addItem("interactive");
    _performanceCombo->addItem("balanced");
    _performanceCombo->addItem("quality");
    _performanceCombo->addItem("custom");
    _performanceCombo->setCurrentIndex(std::max(0, _performanceCombo->findText(QString::fromStdString(_config.performanceProfile))));
    _presetCombo->addItem("disk_orbit");
    _presetCombo->addItem("random_cloud");
    _presetCombo->addItem("two_body");
    _presetCombo->addItem("three_body");
    _presetCombo->addItem("plummer_sphere");
    _presetCombo->addItem("file");
    _presetCombo->setCurrentIndex(std::max(0, _presetCombo->findText(QString::fromStdString(_config.presetStructure))));
    _sphCheck->setChecked(_config.sphEnabled);

    _dtSpin->setDecimals(5);
    _dtSpin->setRange(0.00001, 100.0);
    _dtSpin->setSingleStep(0.001);
    _dtSpin->setValue(std::max(0.00001f, _config.dt));
    _thetaSpin->setDecimals(3);
    _thetaSpin->setRange(0.05, 4.0);
    _thetaSpin->setSingleStep(0.05);
    _thetaSpin->setValue(std::clamp(_config.octreeTheta, 0.05f, 4.0f));
    _softeningSpin->setDecimals(4);
    _softeningSpin->setRange(0.0001, 5.0);
    _softeningSpin->setSingleStep(0.01);
    _softeningSpin->setValue(std::clamp(_config.octreeSoftening, 0.0001f, 5.0f));

    _sphSmoothingSpin->setDecimals(3);
    _sphSmoothingSpin->setRange(0.05, 10.0);
    _sphSmoothingSpin->setSingleStep(0.05);
    _sphSmoothingSpin->setValue(std::max(0.05f, _config.sphSmoothingLength));
    _sphRestDensitySpin->setDecimals(3);
    _sphRestDensitySpin->setRange(0.05, 1000.0);
    _sphRestDensitySpin->setSingleStep(0.05);
    _sphRestDensitySpin->setValue(std::max(0.05f, _config.sphRestDensity));
    _sphGasConstantSpin->setDecimals(3);
    _sphGasConstantSpin->setRange(0.01, 1000.0);
    _sphGasConstantSpin->setSingleStep(0.1);
    _sphGasConstantSpin->setValue(std::max(0.01f, _config.sphGasConstant));
    _sphViscositySpin->setDecimals(4);
    _sphViscositySpin->setRange(0.0, 100.0);
    _sphViscositySpin->setSingleStep(0.01);
    _sphViscositySpin->setValue(std::max(0.0f, _config.sphViscosity));

    _zoomSlider->setRange(1, 400);
    _zoomSlider->setValue(static_cast<int>(std::clamp(_config.defaultZoom * 10.0f, 1.0f, 400.0f)));
    _luminositySlider->setRange(0, 255);
    _luminositySlider->setValue(std::clamp(_config.defaultLuminosity, 0, 255));
    _view3dCombo->addItem("perspective");
    _view3dCombo->addItem("iso");
    _yawSlider->setRange(-180, 180);
    _yawSlider->setValue(0);
    _pitchSlider->setRange(-90, 90);
    _pitchSlider->setValue(0);
    _rollSlider->setRange(-180, 180);
    _rollSlider->setValue(0);
    _cullingCheck->setChecked(_config.renderCullingEnabled);
    _lodCheck->setChecked(_config.renderLODEnabled);
    _serverHostEdit->setText("127.0.0.1");
    _serverPortSpin->setRange(1, 65535);
    _serverPortSpin->setValue(4545);
    _serverAutostartCheck->setChecked(false);
    _serverBinEdit->setPlaceholderText("blitzar-server(.exe)");

    auto *sectionsWidget = new QWidget(this);
    auto *sectionsLayout = new QHBoxLayout(sectionsWidget);
    sectionsLayout->setContentsMargins(4, 4, 4, 4);
    sectionsLayout->setSpacing(10);

    auto *simulationBox = new QGroupBox("Simulation", sectionsWidget);
    auto *simulationLayout = new QVBoxLayout(simulationBox);
    auto *simulationForm = new QFormLayout();
    simulationForm->addRow("solver", _solverCombo);
    simulationForm->addRow("integrator", _integratorCombo);
    simulationForm->addRow("performance", _performanceCombo);
    simulationForm->addRow("preset", _presetCombo);
    simulationLayout->addLayout(simulationForm);
    simulationLayout->addWidget(_applyPresetButton);
    simulationLayout->addWidget(_pauseButton);
    simulationLayout->addWidget(_stepButton);
    simulationLayout->addWidget(_resetButton);
    simulationLayout->addWidget(_recoverButton);
    simulationLayout->addWidget(_reconnectButton);

    auto *timeBox = new QGroupBox("Time", sectionsWidget);
    auto *timeLayout = new QVBoxLayout(timeBox);
    auto *timeForm = new QFormLayout();
    timeForm->addRow("dt", _dtSpin);
    timeForm->addRow("theta", _thetaSpin);
    timeForm->addRow("softening", _softeningSpin);
    timeLayout->addLayout(timeForm);

    auto *sphBox = new QGroupBox("SPH", sectionsWidget);
    auto *sphLayout = new QVBoxLayout(sphBox);
    sphLayout->addWidget(_sphCheck);
    auto *sphForm = new QFormLayout();
    sphForm->addRow("h", _sphSmoothingSpin);
    sphForm->addRow("rest density", _sphRestDensitySpin);
    sphForm->addRow("gas K", _sphGasConstantSpin);
    sphForm->addRow("viscosity", _sphViscositySpin);
    sphLayout->addLayout(sphForm);

    auto *ioBox = new QGroupBox("I/O", sectionsWidget);
    auto *ioLayout = new QVBoxLayout(ioBox);
    ioLayout->addWidget(_saveConfigButton);
    ioLayout->addWidget(_loadPresetButton);
    ioLayout->addWidget(_loadInputButton);
    ioLayout->addWidget(_exportButton);
    ioLayout->addStretch(1);

    auto *connectorBox = new QGroupBox("Connector", sectionsWidget);
    auto *connectorLayout = new QVBoxLayout(connectorBox);
    auto *connectorForm = new QFormLayout();
    connectorForm->addRow("host", _serverHostEdit);
    connectorForm->addRow("port", _serverPortSpin);
    connectorForm->addRow("server bin", _serverBinEdit);
    connectorLayout->addLayout(connectorForm);
    connectorLayout->addWidget(_serverAutostartCheck);
    connectorLayout->addWidget(_applyConnectorButton);
    connectorLayout->addStretch(1);

    sectionsLayout->addWidget(simulationBox);
    sectionsLayout->addWidget(timeBox);
    sectionsLayout->addWidget(sphBox);
    sectionsLayout->addWidget(ioBox);
    sectionsLayout->addWidget(connectorBox);
    sectionsLayout->addStretch(1);

    auto *controlsScroll = new QScrollArea(this);
    controlsScroll->setWidgetResizable(true);
    controlsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    controlsScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    controlsScroll->setWidget(sectionsWidget);
    controlsScroll->setMaximumHeight(240);

    auto *cameraBox = new QGroupBox("View & Camera", this);
    auto *cameraLayout = new QGridLayout(cameraBox);
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

    root->addWidget(controlsScroll, 0);
    root->addWidget(_multiView, 4);
    root->addWidget(cameraBox, 0);
    root->addWidget(_energyGraph, 2);
    root->addWidget(_statusLabel, 0);

    setCentralWidget(central);
    resize(1500, 980);

    applyConfigToServer(false);
    markConfigDirty(false);
    _multiView->setMaxDrawParticles(_clientDrawCap);
    _runtime->setRemoteSnapshotCap(_clientDrawCap);

    _multiView->setZoom(static_cast<float>(_zoomSlider->value()) / 10.0f);
    _multiView->setLuminosity(_luminositySlider->value());
    _multiView->setRenderSettings(
        _config.renderCullingEnabled,
        _config.renderLODEnabled,
        _config.renderLODNearDistance,
        _config.renderLODFarDistance
    );
    update3DCameraFromSliders();
    _reconnectButton->setEnabled(true);
    _applyConnectorButton->setEnabled(true);
    _serverAutostartCheck->setEnabled(true);
    _serverHostEdit->setEnabled(true);
    _serverPortSpin->setEnabled(true);
    _serverBinEdit->setEnabled(true);

    const auto applySphParams = [this]() {
        _config.sphSmoothingLength = static_cast<float>(_sphSmoothingSpin->value());
        _config.sphRestDensity = static_cast<float>(_sphRestDensitySpin->value());
        _config.sphGasConstant = static_cast<float>(_sphGasConstantSpin->value());
        _config.sphViscosity = static_cast<float>(_sphViscositySpin->value());
        _runtime->setSphParameters(
            _config.sphSmoothingLength,
            _config.sphRestDensity,
            _config.sphGasConstant,
            _config.sphViscosity
        );
        markConfigDirty();
    };
    const auto applyConnector = [this]() {
        std::string host = _serverHostEdit->text().trimmed().toStdString();
        if (host.empty()) {
            host = "127.0.0.1";
            _serverHostEdit->setText(QString::fromStdString(host));
        }
        const auto port = static_cast<std::uint16_t>(_serverPortSpin->value());
        const bool autostart = _serverAutostartCheck->isChecked();
        const std::string serverBin = _serverBinEdit->text().trimmed().toStdString();
        _runtime->configureRemoteConnector(host, port, autostart, serverBin);
    };

    connect(_pauseButton, &QPushButton::clicked, this, [this](bool checked) {
        _runtime->setPaused(checked);
        _pauseButton->setText(checked ? "Resume" : "Pause");
    });
    connect(_stepButton, &QPushButton::clicked, this, [this]() { _runtime->stepOnce(); });
    connect(_resetButton, &QPushButton::clicked, this, [this]() {
        _config = SimulationConfig::loadOrCreate(_configPath);
        applyConfigToUi();
        applyConfigToServer(true);
        markConfigDirty(false);
        _lastEnergyStep = std::numeric_limits<std::uint64_t>::max();
    });
    connect(_recoverButton, &QPushButton::clicked, this, [this]() {
        _runtime->requestRecover();
    });
    connect(_reconnectButton, &QPushButton::clicked, this, [applyConnector]() { applyConnector(); });
    connect(_applyConnectorButton, &QPushButton::clicked, this, [applyConnector]() { applyConnector(); });
    connect(_exportButton, &QPushButton::clicked, this, [this]() {
        const SimulationStats stats = _runtime->getCachedStats();
        const std::string preferredFormat = grav_client::normalizeExportFormat(
            _config.exportFormat.empty() ? std::string("vtk") : _config.exportFormat);
        const QString startPath = QString::fromStdString(
            grav_client::buildSuggestedExportPath(_config.exportDirectory, preferredFormat, stats.steps)
        );
        QString selectedFilter = "VTK ASCII (*.vtk)";
        if (preferredFormat == "vtk_binary") {
            selectedFilter = "VTK BINARY (*.vtk)";
        } else if (preferredFormat == "xyz") {
            selectedFilter = "XYZ (*.xyz)";
        } else if (preferredFormat == "bin") {
            selectedFilter = "Native binary (*.bin)";
        }
        const QString pathChosen = QFileDialog::getSaveFileName(
            this,
            "Export Snapshot",
            startPath,
            "VTK ASCII (*.vtk);;VTK BINARY (*.vtk);;XYZ (*.xyz);;Native binary (*.bin);;All files (*.*)",
            &selectedFilter
        );
        if (pathChosen.isEmpty()) {
            return;
        }

        std::string format = grav_client::normalizeExportFormat(formatFromSelectedFilter(selectedFilter));
        std::string path = pathChosen.toStdString();
        if (format.empty()) {
            format = grav_client::normalizeExportFormat(grav_client::inferExportFormatFromPath(path));
        }
        if (format.empty()) {
            format = grav_client::normalizeExportFormat(_config.exportFormat);
            if (format.empty()) {
                format = "vtk";
            }
        }

        std::filesystem::path outPath(path);
        if (outPath.extension().empty()) {
            const std::string ext = grav_client::extensionForExportFormat(format);
            if (!ext.empty()) {
                outPath += ext;
                path = outPath.string();
            }
        }

        _runtime->requestExportSnapshot(path, format);
        _config.exportFormat = format;
        if (outPath.has_parent_path()) {
            _config.exportDirectory = outPath.parent_path().string();
        }
        markConfigDirty();
    });
    connect(_saveConfigButton, &QPushButton::clicked, this, [this]() {
        (void)saveConfigToDisk();
    });
    connect(_loadInputButton, &QPushButton::clicked, this, [this]() {
        const QString startPath = _config.inputFile.empty()
            ? QString::fromStdString(_config.exportDirectory)
            : QString::fromStdString(_config.inputFile);
        const QString path = QFileDialog::getOpenFileName(
            this,
            "Load Initial State",
            startPath,
            "Simulation files (*.vtk *.xyz *.bin);;All files (*.*)"
        );
        if (path.isEmpty()) {
            return;
        }
        _config.inputFile = path.toStdString();
        _config.inputFormat = "auto";
        _config.presetStructure = "file";
        _config.initMode = "file";
        const ResolvedInitialStatePlan initPlan = resolveInitialStatePlan(_config, std::cerr);
        _runtime->setInitialStateFile(initPlan.inputFile, initPlan.inputFormat);
        _runtime->setInitialStateConfig(initPlan.config);
        _runtime->requestReset();
        markConfigDirty();
    });
    connect(_applyPresetButton, &QPushButton::clicked, this, [this]() {
        _config.initConfigStyle = "preset";
        _config.presetStructure = _presetCombo->currentText().toStdString();
        applyConfigToServer(true);
        markConfigDirty();
    });
    connect(_loadPresetButton, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(
            this,
            "Load Preset Config",
            QString::fromStdString(_configPath),
            "Ini files (*.ini);;All files (*.*)"
        );
        if (path.isEmpty()) {
            return;
        }
        SimulationConfig loaded = SimulationConfig::loadOrCreate(path.toStdString());
        _config = loaded;
        _configPath = path.toStdString();
        applyConfigToUi();
        applyConfigToServer(true);
        _lastEnergyStep = std::numeric_limits<std::uint64_t>::max();
        markConfigDirty(false);
    });
    connect(_sphCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        _config.sphEnabled = enabled;
        _runtime->setSphEnabled(enabled);
        markConfigDirty();
    });
    connect(_sphSmoothingSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [applySphParams](double) {
        applySphParams();
    });
    connect(_sphRestDensitySpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [applySphParams](double) {
        applySphParams();
    });
    connect(_sphGasConstantSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [applySphParams](double) {
        applySphParams();
    });
    connect(_sphViscositySpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [applySphParams](double) {
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
    connect(_solverCombo, &QComboBox::currentTextChanged, this, [this](const QString &solver) {
        _config.solver = solver.toStdString();
        _runtime->setSolverMode(solver.toStdString());
        markConfigDirty();
    });
    connect(_integratorCombo, &QComboBox::currentTextChanged, this, [this](const QString &integrator) {
        _config.integrator = integrator.toStdString();
        _runtime->setIntegratorMode(integrator.toStdString());
        markConfigDirty();
    });
    connect(_performanceCombo, &QComboBox::currentTextChanged, this, [this](const QString &profile) {
        _config.performanceProfile = profile.toStdString();
        grav_config::applyPerformanceProfile(_config);
        applyPerformanceProfileToRuntime();
        markConfigDirty();
    });
    connect(_thetaSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        _config.octreeTheta = static_cast<float>(value);
        _runtime->setOctreeParameters(static_cast<float>(value), static_cast<float>(_softeningSpin->value()));
        markConfigDirty();
    });
    connect(_softeningSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        _config.octreeSoftening = static_cast<float>(value);
        _runtime->setOctreeParameters(static_cast<float>(_thetaSpin->value()), static_cast<float>(value));
        markConfigDirty();
    });
    connect(_view3dCombo, &QComboBox::currentTextChanged, this, [this](const QString &value) {
        _multiView->set3DMode(value == "iso" ? grav::ViewMode::Iso : grav::ViewMode::Perspective);
    });
    connect(_cullingCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        _config.renderCullingEnabled = enabled;
        _multiView->setRenderSettings(
            _config.renderCullingEnabled,
            _config.renderLODEnabled,
            _config.renderLODNearDistance,
            _config.renderLODFarDistance
        );
        markConfigDirty();
    });
    connect(_lodCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        _config.renderLODEnabled = enabled;
        _multiView->setRenderSettings(
            _config.renderCullingEnabled,
            _config.renderLODEnabled,
            _config.renderLODNearDistance,
            _config.renderLODFarDistance
        );
        markConfigDirty();
    });
    connect(_yawSlider, &QSlider::valueChanged, this, [this]() { update3DCameraFromSliders(); });
    connect(_pitchSlider, &QSlider::valueChanged, this, [this]() { update3DCameraFromSliders(); });
    connect(_rollSlider, &QSlider::valueChanged, this, [this]() { update3DCameraFromSliders(); });

    connect(_timer, &QTimer::timeout, this, [this]() { tick(); });
    const std::uint32_t fps = std::max<std::uint32_t>(1u, _config.uiFpsLimit);
    if (_runtime->start()) {
        _timer->start(std::max(1, static_cast<int>(1000 / fps)));
    } else {
        _statusLabel->setText(
            QString("server connection failed (service)")
        );
    }
}

MainWindow::~MainWindow()
{
    _runtime->stop();
}

void MainWindow::applyConfigToServer(bool requestReset)
{
    const ResolvedInitialStatePlan initPlan = resolveInitialStatePlan(_config, std::cerr);
    _runtime->setParticleCount(grav_client::resolveServerParticleCount(_config));
    _runtime->setDt(_config.dt);
    _runtime->setSolverMode(_config.solver);
    _runtime->setIntegratorMode(_config.integrator);
    _runtime->setPerformanceProfile(_config.performanceProfile);
    _runtime->setOctreeParameters(_config.octreeTheta, _config.octreeSoftening);
    _runtime->setSphEnabled(_config.sphEnabled);
    _runtime->setSphParameters(
        _config.sphSmoothingLength,
        _config.sphRestDensity,
        _config.sphGasConstant,
        _config.sphViscosity
    );
    _runtime->setSubstepPolicy(_config.substepTargetDt, _config.maxSubsteps);
    _runtime->setSnapshotPublishPeriodMs(_config.snapshotPublishPeriodMs);
    _runtime->setEnergyMeasurementConfig(_config.energyMeasureEverySteps, _config.energySampleLimit);
    _runtime->setExportDefaults(_config.exportDirectory, _config.exportFormat);
    _runtime->setInitialStateFile(initPlan.inputFile, initPlan.inputFormat);
    _runtime->setInitialStateConfig(initPlan.config);
    _clientDrawCap = grav_client::resolveClientDrawCap(_config);
    _multiView->setMaxDrawParticles(_clientDrawCap);
    _runtime->setRemoteSnapshotCap(_clientDrawCap);
    if (requestReset) {
        _runtime->requestReset();
    }
}

void MainWindow::applyConfigToUi()
{
    _solverCombo->blockSignals(true);
    _integratorCombo->blockSignals(true);
    _performanceCombo->blockSignals(true);
    _presetCombo->blockSignals(true);
    _sphCheck->blockSignals(true);
    _dtSpin->blockSignals(true);
    _thetaSpin->blockSignals(true);
    _softeningSpin->blockSignals(true);
    _sphSmoothingSpin->blockSignals(true);
    _sphRestDensitySpin->blockSignals(true);
    _sphGasConstantSpin->blockSignals(true);
    _sphViscositySpin->blockSignals(true);
    _zoomSlider->blockSignals(true);
    _luminositySlider->blockSignals(true);
    _cullingCheck->blockSignals(true);
    _lodCheck->blockSignals(true);

    _solverCombo->setCurrentIndex(std::max(0, _solverCombo->findText(QString::fromStdString(_config.solver))));
    _integratorCombo->setCurrentIndex(std::max(0, _integratorCombo->findText(QString::fromStdString(_config.integrator))));
    _performanceCombo->setCurrentIndex(std::max(0, _performanceCombo->findText(QString::fromStdString(_config.performanceProfile))));
    const int presetIndex = std::max(0, _presetCombo->findText(QString::fromStdString(_config.presetStructure)));
    _presetCombo->setCurrentIndex(presetIndex);
    _sphCheck->setChecked(_config.sphEnabled);
    _dtSpin->setValue(std::max(0.00001f, _config.dt));
    _thetaSpin->setValue(std::clamp(_config.octreeTheta, 0.05f, 4.0f));
    _softeningSpin->setValue(std::clamp(_config.octreeSoftening, 0.0001f, 5.0f));
    _sphSmoothingSpin->setValue(std::max(0.05f, _config.sphSmoothingLength));
    _sphRestDensitySpin->setValue(std::max(0.05f, _config.sphRestDensity));
    _sphGasConstantSpin->setValue(std::max(0.01f, _config.sphGasConstant));
    _sphViscositySpin->setValue(std::max(0.0f, _config.sphViscosity));
    _zoomSlider->setValue(static_cast<int>(std::clamp(_config.defaultZoom * 10.0f, 1.0f, 400.0f)));
    _luminositySlider->setValue(std::clamp(_config.defaultLuminosity, 0, 255));
    _cullingCheck->setChecked(_config.renderCullingEnabled);
    _lodCheck->setChecked(_config.renderLODEnabled);

    _solverCombo->blockSignals(false);
    _integratorCombo->blockSignals(false);
    _performanceCombo->blockSignals(false);
    _presetCombo->blockSignals(false);
    _sphCheck->blockSignals(false);
    _dtSpin->blockSignals(false);
    _thetaSpin->blockSignals(false);
    _softeningSpin->blockSignals(false);
    _sphSmoothingSpin->blockSignals(false);
    _sphRestDensitySpin->blockSignals(false);
    _sphGasConstantSpin->blockSignals(false);
    _sphViscositySpin->blockSignals(false);
    _zoomSlider->blockSignals(false);
    _luminositySlider->blockSignals(false);
    _cullingCheck->blockSignals(false);
    _lodCheck->blockSignals(false);

    _multiView->setZoom(static_cast<float>(_zoomSlider->value()) / 10.0f);
    _multiView->setLuminosity(_luminositySlider->value());
    _multiView->setRenderSettings(
        _config.renderCullingEnabled,
        _config.renderLODEnabled,
        _config.renderLODNearDistance,
        _config.renderLODFarDistance
    );
    _clientDrawCap = grav_client::resolveClientDrawCap(_config);
    _multiView->setMaxDrawParticles(_clientDrawCap);
    _runtime->setRemoteSnapshotCap(_clientDrawCap);
}

void MainWindow::captureUiIntoConfig()
{
    _config.solver = _solverCombo->currentText().toStdString();
    _config.integrator = _integratorCombo->currentText().toStdString();
    _config.performanceProfile = _performanceCombo->currentText().toStdString();
    _config.presetStructure = _presetCombo->currentText().toStdString();
    _config.sphEnabled = _sphCheck->isChecked();
    _config.dt = static_cast<float>(_dtSpin->value());
    _config.octreeTheta = static_cast<float>(_thetaSpin->value());
    _config.octreeSoftening = static_cast<float>(_softeningSpin->value());
    _config.sphSmoothingLength = static_cast<float>(_sphSmoothingSpin->value());
    _config.sphRestDensity = static_cast<float>(_sphRestDensitySpin->value());
    _config.sphGasConstant = static_cast<float>(_sphGasConstantSpin->value());
    _config.sphViscosity = static_cast<float>(_sphViscositySpin->value());
    _config.defaultZoom = static_cast<float>(_zoomSlider->value()) / 10.0f;
    _config.defaultLuminosity = _luminositySlider->value();
    _config.renderCullingEnabled = _cullingCheck->isChecked();
    _config.renderLODEnabled = _lodCheck->isChecked();
}

void MainWindow::applyPerformanceProfileToRuntime()
{
    _runtime->setPerformanceProfile(_config.performanceProfile);
    _runtime->setSubstepPolicy(_config.substepTargetDt, _config.maxSubsteps);
    _runtime->setSnapshotPublishPeriodMs(_config.snapshotPublishPeriodMs);
    _runtime->setEnergyMeasurementConfig(_config.energyMeasureEverySteps, _config.energySampleLimit);
    _clientDrawCap = grav_client::resolveClientDrawCap(_config);
    _multiView->setMaxDrawParticles(_clientDrawCap);
    _runtime->setRemoteSnapshotCap(_clientDrawCap);
}

void MainWindow::markConfigDirty(bool dirty)
{
    _configDirty = dirty;
    if (_saveConfigButton != nullptr) {
        _saveConfigButton->setEnabled(_configDirty);
    }
    setWindowTitle(_configDirty ? "N-Body Qt Client *" : "N-Body Qt Client");
}

bool MainWindow::saveConfigToDisk()
{
    captureUiIntoConfig();
    if (_configPath.empty()) {
        _configPath = "simulation.ini";
    }
    if (!_config.save(_configPath)) {
        std::cerr << "[qt] failed to save config: " << _configPath << "\n";
        return false;
    }
    markConfigDirty(false);
    std::cout << "[qt] config saved: " << _configPath << "\n";
    return true;
}

void MainWindow::update3DCameraFromSliders()
{
    constexpr float pi = 3.14159265359f;
    const float yaw = static_cast<float>(_yawSlider->value()) * pi / 180.0f;
    const float pitch = static_cast<float>(_pitchSlider->value()) * pi / 180.0f;
    const float roll = static_cast<float>(_rollSlider->value()) * pi / 180.0f;
    _multiView->set3DCameraAngles(yaw, pitch, roll);
}

void MainWindow::tick()
{
    const auto uiNow = std::chrono::steady_clock::now();
    if (_lastUiTickAt.time_since_epoch().count() != 0) {
        const std::chrono::duration<float> uiDt = uiNow - _lastUiTickAt;
        if (uiDt.count() > 1e-6f) {
            const float inst = 1.0f / uiDt.count();
            _uiTickFps = (_uiTickFps <= 0.0f) ? inst : (0.9f * _uiTickFps + 0.1f * inst);
        }
    }
    _lastUiTickAt = uiNow;

    std::vector<RenderParticle> snapshot;
    const SimulationStats stats = _runtime->getCachedStats();
    if (_solverCombo && !stats.solverName.empty()) {
        const QString solverText = QString::fromStdString(stats.solverName);
        const int solverIndex = _solverCombo->findText(solverText);
        if (solverIndex >= 0 && _solverCombo->currentIndex() != solverIndex) {
            _solverCombo->blockSignals(true);
            _solverCombo->setCurrentIndex(solverIndex);
            _solverCombo->blockSignals(false);
            _config.solver = stats.solverName;
        }
    }
    if (_integratorCombo && !stats.integratorName.empty()) {
        const QString integratorText = QString::fromStdString(stats.integratorName);
        const int integratorIndex = _integratorCombo->findText(integratorText);
        if (integratorIndex >= 0 && _integratorCombo->currentIndex() != integratorIndex) {
            _integratorCombo->blockSignals(true);
            _integratorCombo->setCurrentIndex(integratorIndex);
            _integratorCombo->blockSignals(false);
            _config.integrator = stats.integratorName;
        }
    }
    std::size_t snapshotSize = 0u;
    std::optional<grav_client::ConsumedSnapshot> consumedSnapshot = _runtime->consumeLatestSnapshot();
    const bool gotSnapshot = consumedSnapshot.has_value();
    if (gotSnapshot) {
        snapshotSize = consumedSnapshot->sourceSize;
        snapshot = std::move(consumedSnapshot->particles);
    }
    const std::string linkLabel = _runtime->linkStateLabel();
    const std::string ownerLabel = _runtime->serverOwnerLabel();
    const std::uint32_t statsAgeMs = _runtime->statsAgeMs();
    const std::uint32_t snapshotAgeMs = _runtime->snapshotAgeMs();
    const bool hasStatsAge = statsAgeMs != std::numeric_limits<std::uint32_t>::max();
    const bool hasSnapshotAge = snapshotAgeMs != std::numeric_limits<std::uint32_t>::max();
    const bool staleStats = hasStatsAge && statsAgeMs > 1000u;
    const bool staleSnapshot = hasSnapshotAge && snapshotAgeMs > 1000u;
    const bool stale = (linkLabel != "connected" || staleStats || staleSnapshot);
    if (gotSnapshot) {
        _multiView->setSnapshot(std::move(snapshot));
    }
    const size_t displayedParticles = _multiView->displayedParticleCount();
    if (stats.steps != _lastEnergyStep) {
        _energyGraph->pushSample(stats);
        _lastEnergyStep = stats.steps;
    }

    _statusLabel->setText(
        QString("state=%1 | link=%2 owner=%3 | solver=%4 integrator=%5 perf=%6 | sph=%7 | dt=%8 | server=%9 step/s | sub=%10x@%11 (target=%12 max=%13) snap=%14ms | ui=%15 fps | steps=%16 | particles=%17 draw=%18 cap=%19 | data=stats:%20 snap:%21 %22 | Ekin=%23 Epot=%24 Eth=%25 Erad=%26 Etot=%27 | dE=%28%% %29")
            .arg(stats.faulted ? "FAULT" : (stats.paused ? "PAUSED" : "RUNNING"))
            .arg(QString::fromStdString(linkLabel))
            .arg(QString::fromStdString(ownerLabel))
            .arg(QString::fromStdString(stats.solverName))
            .arg(QString::fromStdString(stats.integratorName))
            .arg(QString::fromStdString(stats.performanceProfile.empty() ? _config.performanceProfile : stats.performanceProfile))
            .arg(stats.sphEnabled ? "on" : "off")
            .arg(stats.dt, 0, 'f', 5)
            .arg(stats.serverFps, 0, 'f', 1)
            .arg(stats.substeps)
            .arg(stats.substepDt, 0, 'f', 5)
            .arg(stats.substepTargetDt, 0, 'f', 5)
            .arg(stats.maxSubsteps)
            .arg(stats.snapshotPublishPeriodMs)
            .arg(_uiTickFps, 0, 'f', 1)
            .arg(static_cast<qulonglong>(stats.steps))
            .arg(stats.particleCount)
            .arg(static_cast<qulonglong>(displayedParticles))
            .arg(static_cast<qulonglong>(_clientDrawCap))
            .arg(hasStatsAge ? QString::number(statsAgeMs) + "ms" : QString("n/a"))
            .arg(hasSnapshotAge ? QString::number(snapshotAgeMs) + "ms" : QString("n/a"))
            .arg(stale ? "[stale]" : "")
            .arg(stats.kineticEnergy, 0, 'g', 6)
            .arg(stats.potentialEnergy, 0, 'g', 6)
            .arg(stats.thermalEnergy, 0, 'g', 6)
            .arg(stats.radiatedEnergy, 0, 'g', 6)
            .arg(stats.totalEnergy, 0, 'g', 6)
            .arg(stats.energyDriftPct, 0, 'f', 4)
            .arg(stats.faulted
                ? QString("[fault@%1 %2]").arg(static_cast<qulonglong>(stats.faultStep)).arg(QString::fromStdString(stats.faultReason))
                : (stats.energyEstimated ? QString("(estimated)") : QString()))
    );

    _pauseButton->blockSignals(true);
    _pauseButton->setChecked(stats.paused);
    _pauseButton->setText(stats.paused ? "Resume" : "Pause");
    _pauseButton->blockSignals(false);
    _recoverButton->setEnabled(stats.faulted);

    static auto lastConsoleTrace = std::chrono::steady_clock::now();
    const auto now = std::chrono::steady_clock::now();
    if (now - lastConsoleTrace >= std::chrono::seconds(1)) {
        std::cout << "[qt] step=" << stats.steps
                  << " link=" << linkLabel
                  << " owner=" << ownerLabel
                  << " solver=" << stats.solverName
                  << " integrator=" << stats.integratorName
                  << " perf=" << (stats.performanceProfile.empty() ? _config.performanceProfile : stats.performanceProfile)
                  << " sph=" << (stats.sphEnabled ? "on" : "off")
                  << " step_s=" << stats.serverFps
                  << " substeps=" << stats.substeps
                  << " substep_dt=" << stats.substepDt
                  << " substep_target_dt=" << stats.substepTargetDt
                  << " max_substeps=" << stats.maxSubsteps
                  << " snapshot_publish_ms=" << stats.snapshotPublishPeriodMs
                  << " ui_fps=" << _uiTickFps
                  << " snapshot=" << snapshotSize
                  << " draw=" << displayedParticles
                  << " draw_cap=" << _clientDrawCap
                  << " stats_age_ms=" << (hasStatsAge ? std::to_string(statsAgeMs) : std::string("na"))
                  << " snap_age_ms=" << (hasSnapshotAge ? std::to_string(snapshotAgeMs) : std::string("na"))
                  << " stale=" << (stale ? "1" : "0")
                  << " faulted=" << (stats.faulted ? "1" : "0")
                  << " fault_step=" << stats.faultStep
                  << " fault_reason=\"" << stats.faultReason << "\""
                  << " energy=" << stats.totalEnergy
                  << " thermal=" << stats.thermalEnergy
                  << " radiated=" << stats.radiatedEnergy
                  << " drift_pct=" << stats.energyDriftPct
                  << (stats.energyEstimated ? " est" : "")
                  << "\n";
        lastConsoleTrace = now;
    }
}

} // namespace grav_qt
