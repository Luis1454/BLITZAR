#include "ui/MainWindow.hpp"

#include "sim/SimulationInitConfig.hpp"
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
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <utility>
#include <vector>

namespace qtui {
namespace {

std::uint32_t getRequestedBackendParticleCount(const SimulationConfig &config)
{
    const std::uint32_t baseCount = std::max<std::uint32_t>(2u, config.particleCount);

    std::uint32_t requested = baseCount;
    if (const char *legacy = std::getenv("GRAVITY_FRONTEND_PARTICLES")) {
        (void)legacy;
        static bool warnedLegacy = false;
        if (!warnedLegacy) {
            std::cout << "[qt] ignoring deprecated GRAVITY_FRONTEND_PARTICLES; "
                         "use GRAVITY_BACKEND_PARTICLES for backend and "
                         "GRAVITY_FRONTEND_DRAW_CAP for frontend draw cap\n";
            warnedLegacy = true;
        }
    }
    if (const char *envValue = std::getenv("GRAVITY_BACKEND_PARTICLES")) {
        char *end = nullptr;
        const long parsed = std::strtol(envValue, &end, 10);
        if (end != envValue && parsed > 1) {
            requested = static_cast<std::uint32_t>(parsed);
        }
    }

    return requested;
}

std::uint32_t getFrontendDrawCap(const SimulationConfig &config)
{
    const std::uint32_t cap = std::max<std::uint32_t>(2u, config.frontendParticleCap);
    if (const char *envValue = std::getenv("GRAVITY_FRONTEND_DRAW_CAP")) {
        char *end = nullptr;
        const long parsed = std::strtol(envValue, &end, 10);
        if (end != envValue && parsed > 1) {
            return static_cast<std::uint32_t>(parsed);
        }
    }
    return cap;
}

std::string toLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string normalizeExportFormat(const std::string &raw)
{
    const std::string format = toLower(raw);
    if (format == "binary") {
        return "bin";
    }
    if (format == "vtkb" || format == "vtk-bin") {
        return "vtk_binary";
    }
    if (format == "vtk_ascii") {
        return "vtk";
    }
    return format;
}

std::string extensionForExportFormat(const std::string &raw)
{
    const std::string format = normalizeExportFormat(raw);
    if (format == "vtk" || format == "vtk_binary") {
        return ".vtk";
    }
    if (format == "xyz") {
        return ".xyz";
    }
    if (format == "bin") {
        return ".bin";
    }
    return {};
}

std::string inferExportFormatFromPath(const std::string &path)
{
    std::string ext = toLower(std::filesystem::path(path).extension().string());
    if (!ext.empty() && ext.front() == '.') {
        ext.erase(ext.begin());
    }
    if (ext == "vtk") {
        return "vtk";
    }
    if (ext == "xyz") {
        return "xyz";
    }
    if (ext == "bin" || ext == "binary" || ext == "nbin") {
        return "bin";
    }
    return {};
}

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

std::string buildSuggestedExportPath(const std::string &directory, const std::string &format, std::uint64_t step)
{
    const std::string normalizedFormat = normalizeExportFormat(format.empty() ? std::string("vtk") : format);
    const std::string extension = extensionForExportFormat(normalizedFormat);

    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &nowTime);
#else
    localtime_r(&nowTime, &tm);
#endif

    std::ostringstream fileName;
    fileName << "sim_" << std::put_time(&tm, "%Y%m%d_%H%M%S") << "_s" << step << extension;

    std::filesystem::path baseDir = directory.empty() ? std::filesystem::path("exports") : std::filesystem::path(directory);
    return (baseDir / fileName.str()).string();
}

} // namespace

MainWindow::MainWindow(SimulationConfig config, std::string configPath, QWidget *parent)
    : QMainWindow(parent),
      _config(std::move(config)),
      _configPath(std::move(configPath)),
      _backend(getRequestedBackendParticleCount(_config), _config.dt),
      _multiView(new MultiViewWidget(this)),
      _energyGraph(new EnergyGraphWidget(this)),
      _statusLabel(new QLabel(this)),
      _pauseButton(new QPushButton("Pause", this)),
      _stepButton(new QPushButton("Step", this)),
      _resetButton(new QPushButton("Reset", this)),
      _exportButton(new QPushButton("Export", this)),
      _loadInputButton(new QPushButton("Load input", this)),
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
      _presetCombo(new QComboBox(this)),
      _view3dCombo(new QComboBox(this)),
      _thetaSpin(new QDoubleSpinBox(this)),
      _softeningSpin(new QDoubleSpinBox(this)),
      _applyPresetButton(new QPushButton("Apply preset", this)),
      _loadPresetButton(new QPushButton("Load preset file", this)),
      _yawSlider(new QSlider(Qt::Horizontal, this)),
      _pitchSlider(new QSlider(Qt::Horizontal, this)),
      _rollSlider(new QSlider(Qt::Horizontal, this)),
      _timer(new QTimer(this)),
      _lastEnergyStep(std::numeric_limits<std::uint64_t>::max()),
      _frontendDrawCap(getFrontendDrawCap(_config)),
      _uiTickFps(0.0f),
      _lastUiTickAt()
{
    setWindowTitle("N-Body Qt Frontend");

    QWidget *central = new QWidget(this);
    QVBoxLayout *root = new QVBoxLayout(central);
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
    _presetCombo->addItem("disk_orbit");
    _presetCombo->addItem("random_cloud");
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

    QWidget *sectionsWidget = new QWidget(this);
    QHBoxLayout *sectionsLayout = new QHBoxLayout(sectionsWidget);
    sectionsLayout->setContentsMargins(4, 4, 4, 4);
    sectionsLayout->setSpacing(10);

    QGroupBox *simulationBox = new QGroupBox("Simulation", sectionsWidget);
    QVBoxLayout *simulationLayout = new QVBoxLayout(simulationBox);
    QFormLayout *simulationForm = new QFormLayout();
    simulationForm->addRow("solver", _solverCombo);
    simulationForm->addRow("integrator", _integratorCombo);
    simulationForm->addRow("preset", _presetCombo);
    simulationLayout->addLayout(simulationForm);
    simulationLayout->addWidget(_applyPresetButton);
    simulationLayout->addWidget(_pauseButton);
    simulationLayout->addWidget(_stepButton);
    simulationLayout->addWidget(_resetButton);

    QGroupBox *timeBox = new QGroupBox("Time", sectionsWidget);
    QVBoxLayout *timeLayout = new QVBoxLayout(timeBox);
    QFormLayout *timeForm = new QFormLayout();
    timeForm->addRow("dt", _dtSpin);
    timeForm->addRow("theta", _thetaSpin);
    timeForm->addRow("softening", _softeningSpin);
    timeLayout->addLayout(timeForm);

    QGroupBox *sphBox = new QGroupBox("SPH", sectionsWidget);
    QVBoxLayout *sphLayout = new QVBoxLayout(sphBox);
    sphLayout->addWidget(_sphCheck);
    QFormLayout *sphForm = new QFormLayout();
    sphForm->addRow("h", _sphSmoothingSpin);
    sphForm->addRow("rest density", _sphRestDensitySpin);
    sphForm->addRow("gas K", _sphGasConstantSpin);
    sphForm->addRow("viscosity", _sphViscositySpin);
    sphLayout->addLayout(sphForm);

    QGroupBox *ioBox = new QGroupBox("I/O", sectionsWidget);
    QVBoxLayout *ioLayout = new QVBoxLayout(ioBox);
    ioLayout->addWidget(_loadPresetButton);
    ioLayout->addWidget(_loadInputButton);
    ioLayout->addWidget(_exportButton);
    ioLayout->addStretch(1);

    sectionsLayout->addWidget(simulationBox);
    sectionsLayout->addWidget(timeBox);
    sectionsLayout->addWidget(sphBox);
    sectionsLayout->addWidget(ioBox);
    sectionsLayout->addStretch(1);

    QScrollArea *controlsScroll = new QScrollArea(this);
    controlsScroll->setWidgetResizable(true);
    controlsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    controlsScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    controlsScroll->setWidget(sectionsWidget);
    controlsScroll->setMaximumHeight(240);

    QGroupBox *cameraBox = new QGroupBox("View & Camera", this);
    QGridLayout *cameraLayout = new QGridLayout(cameraBox);
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

    root->addWidget(controlsScroll, 0);
    root->addWidget(_multiView, 4);
    root->addWidget(cameraBox, 0);
    root->addWidget(_energyGraph, 2);
    root->addWidget(_statusLabel, 0);

    setCentralWidget(central);
    resize(1500, 980);

    applyConfigToBackend(false);
    _multiView->setMaxDrawParticles(_frontendDrawCap);

    _multiView->setZoom(static_cast<float>(_zoomSlider->value()) / 10.0f);
    _multiView->setLuminosity(_luminositySlider->value());
    update3DCameraFromSliders();

    const auto applySphParams = [this]() {
        _backend.setSphParameters(
            static_cast<float>(_sphSmoothingSpin->value()),
            static_cast<float>(_sphRestDensitySpin->value()),
            static_cast<float>(_sphGasConstantSpin->value()),
            static_cast<float>(_sphViscositySpin->value())
        );
    };

    connect(_pauseButton, &QPushButton::clicked, this, [this](bool checked) {
        _backend.setPaused(checked);
        _pauseButton->setText(checked ? "Resume" : "Pause");
    });
    connect(_stepButton, &QPushButton::clicked, this, [this]() { _backend.stepOnce(); });
    connect(_resetButton, &QPushButton::clicked, this, [this]() {
        _config = SimulationConfig::loadOrCreate(_configPath);
        applyConfigToUi();
        applyConfigToBackend(true);
        _lastEnergyStep = std::numeric_limits<std::uint64_t>::max();
    });
    connect(_exportButton, &QPushButton::clicked, this, [this]() {
        const SimulationStats stats = _backend.getStats();
        const std::string preferredFormat = normalizeExportFormat(_config.exportFormat.empty() ? std::string("vtk") : _config.exportFormat);
        const QString startPath = QString::fromStdString(
            buildSuggestedExportPath(_config.exportDirectory, preferredFormat, stats.steps)
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

        std::string format = normalizeExportFormat(formatFromSelectedFilter(selectedFilter));
        std::string path = pathChosen.toStdString();
        if (format.empty()) {
            format = normalizeExportFormat(inferExportFormatFromPath(path));
        }
        if (format.empty()) {
            format = normalizeExportFormat(_config.exportFormat);
            if (format.empty()) {
                format = "vtk";
            }
        }

        std::filesystem::path outPath(path);
        if (outPath.extension().empty()) {
            const std::string ext = extensionForExportFormat(format);
            if (!ext.empty()) {
                outPath += ext;
                path = outPath.string();
            }
        }

        _backend.requestExportSnapshot(path, format);
        _config.exportFormat = format;
        if (outPath.has_parent_path()) {
            _config.exportDirectory = outPath.parent_path().string();
        }
        _config.save(_configPath);
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
        _config.initMode = "file";
        _backend.setInitialStateFile(_config.inputFile, _config.inputFormat);
        _backend.setInitialStateConfig(buildInitialStateConfig(_config));
        _backend.requestReset();
        _config.save(_configPath);
    });
    connect(_applyPresetButton, &QPushButton::clicked, this, [this]() {
        _config.initConfigStyle = "preset";
        _config.presetStructure = _presetCombo->currentText().toStdString();
        applyConfigToBackend(true);
        _config.save(_configPath);
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
        applyConfigToBackend(true);
        _lastEnergyStep = std::numeric_limits<std::uint64_t>::max();
        _config.save(_configPath);
    });
    connect(_sphCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        _config.sphEnabled = enabled;
        _backend.setSphEnabled(enabled);
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
        _backend.setDt(static_cast<float>(value));
    });
    connect(_zoomSlider, &QSlider::valueChanged, this, [this](int value) {
        _multiView->setZoom(static_cast<float>(value) / 10.0f);
    });
    connect(_luminositySlider, &QSlider::valueChanged, this, [this](int value) {
        _multiView->setLuminosity(value);
    });
    connect(_solverCombo, &QComboBox::currentTextChanged, this, [this](const QString &solver) {
        _backend.setSolverMode(solver.toStdString());
    });
    connect(_integratorCombo, &QComboBox::currentTextChanged, this, [this](const QString &integrator) {
        _backend.setIntegratorMode(integrator.toStdString());
    });
    connect(_thetaSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        _backend.setOctreeParameters(static_cast<float>(value), static_cast<float>(_softeningSpin->value()));
    });
    connect(_softeningSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        _backend.setOctreeParameters(static_cast<float>(_thetaSpin->value()), static_cast<float>(value));
    });
    connect(_view3dCombo, &QComboBox::currentTextChanged, this, [this](const QString &value) {
        _multiView->set3DMode(value == "iso" ? ViewMode::Iso : ViewMode::Perspective);
    });
    connect(_yawSlider, &QSlider::valueChanged, this, [this]() { update3DCameraFromSliders(); });
    connect(_pitchSlider, &QSlider::valueChanged, this, [this]() { update3DCameraFromSliders(); });
    connect(_rollSlider, &QSlider::valueChanged, this, [this]() { update3DCameraFromSliders(); });

    connect(_timer, &QTimer::timeout, this, [this]() { tick(); });
    const std::uint32_t fps = std::max<std::uint32_t>(1u, _config.uiFpsLimit);
    _timer->start(std::max(1, static_cast<int>(1000 / fps)));

    _backend.start();
}

MainWindow::~MainWindow()
{
    _backend.stop();
}

void MainWindow::applyConfigToBackend(bool requestReset)
{
    _backend.setParticleCount(getRequestedBackendParticleCount(_config));
    _backend.setDt(_config.dt);
    _backend.setSolverMode(_config.solver);
    _backend.setIntegratorMode(_config.integrator);
    _backend.setOctreeParameters(_config.octreeTheta, _config.octreeSoftening);
    _backend.setSphEnabled(_config.sphEnabled);
    _backend.setSphParameters(
        _config.sphSmoothingLength,
        _config.sphRestDensity,
        _config.sphGasConstant,
        _config.sphViscosity
    );
    _backend.setEnergyMeasurementConfig(_config.energyMeasureEverySteps, _config.energySampleLimit);
    _backend.setExportDefaults(_config.exportDirectory, _config.exportFormat);
    _backend.setInitialStateFile(_config.inputFile, _config.inputFormat);
    _backend.setInitialStateConfig(buildInitialStateConfig(_config));
    if (requestReset) {
        _backend.requestReset();
    }
}

void MainWindow::applyConfigToUi()
{
    _solverCombo->blockSignals(true);
    _integratorCombo->blockSignals(true);
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

    _solverCombo->setCurrentIndex(std::max(0, _solverCombo->findText(QString::fromStdString(_config.solver))));
    _integratorCombo->setCurrentIndex(std::max(0, _integratorCombo->findText(QString::fromStdString(_config.integrator))));
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

    _solverCombo->blockSignals(false);
    _integratorCombo->blockSignals(false);
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

    _multiView->setZoom(static_cast<float>(_zoomSlider->value()) / 10.0f);
    _multiView->setLuminosity(_luminositySlider->value());
    _frontendDrawCap = getFrontendDrawCap(_config);
    _multiView->setMaxDrawParticles(_frontendDrawCap);
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
    const bool gotSnapshot = _backend.tryConsumeSnapshot(snapshot);
    const size_t snapshotSize = snapshot.size();
    if (gotSnapshot) {
        _multiView->setSnapshot(std::move(snapshot));
    }

    const SimulationStats stats = _backend.getStats();
    const size_t displayedParticles = _multiView->displayedParticleCount();
    if (stats.steps != _lastEnergyStep) {
        _energyGraph->pushSample(stats);
        _lastEnergyStep = stats.steps;
    }

    _statusLabel->setText(
        QString("state=%1 | solver=%2 | sph=%3 | dt=%4 | backend=%5 step/s | ui=%6 fps | steps=%7 | particles=%8 draw=%9 cap=%10 | Ekin=%11 Epot=%12 Eth=%13 Erad=%14 Etot=%15 | dE=%16%% %17")
            .arg(stats.paused ? "PAUSED" : "RUNNING")
            .arg(stats.solverName)
            .arg(stats.sphEnabled ? "on" : "off")
            .arg(stats.dt, 0, 'f', 5)
            .arg(stats.backendFps, 0, 'f', 1)
            .arg(_uiTickFps, 0, 'f', 1)
            .arg(static_cast<qulonglong>(stats.steps))
            .arg(stats.particleCount)
            .arg(static_cast<qulonglong>(displayedParticles))
            .arg(static_cast<qulonglong>(_frontendDrawCap))
            .arg(stats.kineticEnergy, 0, 'g', 6)
            .arg(stats.potentialEnergy, 0, 'g', 6)
            .arg(stats.thermalEnergy, 0, 'g', 6)
            .arg(stats.radiatedEnergy, 0, 'g', 6)
            .arg(stats.totalEnergy, 0, 'g', 6)
            .arg(stats.energyDriftPct, 0, 'f', 4)
            .arg(stats.energyEstimated ? "(estimated)" : "")
    );

    _pauseButton->blockSignals(true);
    _pauseButton->setChecked(stats.paused);
    _pauseButton->setText(stats.paused ? "Resume" : "Pause");
    _pauseButton->blockSignals(false);

    static auto lastConsoleTrace = std::chrono::steady_clock::now();
    const auto now = std::chrono::steady_clock::now();
    if (now - lastConsoleTrace >= std::chrono::seconds(1)) {
        std::cout << "[qt] step=" << stats.steps
                  << " solver=" << stats.solverName
                  << " sph=" << (stats.sphEnabled ? "on" : "off")
                  << " step_s=" << stats.backendFps
                  << " ui_fps=" << _uiTickFps
                  << " snapshot=" << snapshotSize
                  << " draw=" << displayedParticles
                  << " draw_cap=" << _frontendDrawCap
                  << " energy=" << stats.totalEnergy
                  << " thermal=" << stats.thermalEnergy
                  << " radiated=" << stats.radiatedEnergy
                  << " drift_pct=" << stats.energyDriftPct
                  << (stats.energyEstimated ? " est" : "")
                  << "\n";
        lastConsoleTrace = now;
    }
}

} // namespace qtui
