/*
 * @file modules/qt/src/window/config/ConfigurationEditor.cpp
 * @brief Structured tile editor for all persisted simulation settings.
 */

#include "window/config/ConfigurationEditor.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStringList>
#include <QVBoxLayout>
#include <algorithm>
#include <climits>
#include <limits>

namespace bltzr_qt {
QWidget* addInt(QFormLayout* form, QHash<QString, QWidget*>& fields, const char* key, int value,
                int minimum = 0, int maximum = INT_MAX)
{
    auto* editor = new QSpinBox(form->parentWidget());
    editor->setObjectName(QString::fromUtf8(key));
    editor->setRange(minimum, maximum);
    editor->setValue(value);
    fields.insert(QString::fromUtf8(key), editor);
    form->addRow(QString::fromUtf8(key), editor);
    return editor;
}

QWidget* addFloat(QFormLayout* form, QHash<QString, QWidget*>& fields, const char* key,
                  double value, double minimum = -1.0e12, double maximum = 1.0e12, int decimals = 6)
{
    auto* editor = new QDoubleSpinBox(form->parentWidget());
    editor->setObjectName(QString::fromUtf8(key));
    editor->setDecimals(decimals);
    editor->setRange(minimum, maximum);
    editor->setSingleStep(0.01);
    editor->setValue(value);
    fields.insert(QString::fromUtf8(key), editor);
    form->addRow(QString::fromUtf8(key), editor);
    return editor;
}

QWidget* addBool(QFormLayout* form, QHash<QString, QWidget*>& fields, const char* key, bool value)
{
    auto* editor = new QCheckBox(form->parentWidget());
    editor->setObjectName(QString::fromUtf8(key));
    editor->setChecked(value);
    fields.insert(QString::fromUtf8(key), editor);
    form->addRow(QString::fromUtf8(key), editor);
    return editor;
}

QWidget* addText(QFormLayout* form, QHash<QString, QWidget*>& fields, const char* key,
                 const std::string& value)
{
    auto* editor = new QLineEdit(QString::fromStdString(value), form->parentWidget());
    editor->setObjectName(QString::fromUtf8(key));
    fields.insert(QString::fromUtf8(key), editor);
    form->addRow(QString::fromUtf8(key), editor);
    return editor;
}

QWidget* addCombo(QFormLayout* form, QHash<QString, QWidget*>& fields, const char* key,
                  const std::string& value, const QStringList& choices)
{
    auto* editor = new QComboBox(form->parentWidget());
    editor->setObjectName(QString::fromUtf8(key));
    editor->addItems(choices);
    const int index = editor->findText(QString::fromStdString(value));
    editor->setCurrentIndex(index < 0 ? 0 : index);
    fields.insert(QString::fromUtf8(key), editor);
    form->addRow(QString::fromUtf8(key), editor);
    return editor;
}

QWidget* field(const QHash<QString, QWidget*>& fields, const char* key)
{
    return fields.value(QString::fromUtf8(key), nullptr);
}

bool fieldIsActive(QWidget* editor)
{
    if (editor == nullptr)
        return false;
    for (auto* parent = editor; parent != nullptr; parent = parent->parentWidget()) {
        if (parent->property("configSection").isValid())
            return parent->isVisible();
    }
    return true;
}

void readInt(const QHash<QString, QWidget*>& fields, const char* key, std::uint32_t& target)
{
    if (auto* widget = field(fields, key); fieldIsActive(widget)) {
        auto* editor = qobject_cast<QSpinBox*>(widget);
        if (editor != nullptr)
            target = static_cast<std::uint32_t>(editor->value());
    }
}

void readInt(const QHash<QString, QWidget*>& fields, const char* key, int& target)
{
    if (auto* widget = field(fields, key); fieldIsActive(widget)) {
        auto* editor = qobject_cast<QSpinBox*>(widget);
        if (editor != nullptr)
            target = editor->value();
    }
}

void readFloat(const QHash<QString, QWidget*>& fields, const char* key, float& target)
{
    if (auto* widget = field(fields, key); fieldIsActive(widget)) {
        auto* editor = qobject_cast<QDoubleSpinBox*>(widget);
        if (editor != nullptr)
            target = static_cast<float>(editor->value());
    }
}

void readBool(const QHash<QString, QWidget*>& fields, const char* key, bool& target)
{
    if (auto* widget = field(fields, key); fieldIsActive(widget)) {
        auto* editor = qobject_cast<QCheckBox*>(widget);
        if (editor != nullptr)
            target = editor->isChecked();
    }
}

void readString(const QHash<QString, QWidget*>& fields, const char* key, std::string& target)
{
    auto* widget = field(fields, key);
    if (!fieldIsActive(widget))
        return;
    if (auto* editor = qobject_cast<QLineEdit*>(widget))
        target = editor->text().toStdString();
    else if (auto* combo = qobject_cast<QComboBox*>(widget))
        target = combo->currentText().toStdString();
}

ConfigurationEditor::ConfigurationEditor(const SimulationConfig& config, QWidget* parent)
    : QDialog(parent), _configuration(config)
{
    setWindowTitle("Configuration");
    setObjectName("configurationEditorDialog");
    resize(760, 820);
    auto* root = new QVBoxLayout(this);
    auto* intro = new QLabel("Advanced input/output settings only. Solver, physics, rendering, "
                             "scene objects and properties are managed in their dedicated menus.",
                             this);
    intro->setWordWrap(true);
    root->addWidget(intro);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setObjectName("configurationEditorScroll");
    auto* content = new QWidget(scroll);
    content->setMinimumWidth(0);
    content->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    auto* stack = new QVBoxLayout(content);
    stack->setContentsMargins(8, 8, 8, 8);
    stack->setSpacing(10);
    content->setLayout(stack);
    scroll->setWidget(content);
    _content = content;
    _stack = stack;
    root->addWidget(scroll, 1);
    buildTiles(config);
    if (auto* firstField = findChild<QSpinBox*>("particle_count")) {
        firstField->setFocus();
    }

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    buttons->addButton("Apply", QDialogButtonBox::AcceptRole);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    root->addWidget(buttons);
}

SimulationConfig ConfigurationEditor::configuration() const
{
    SimulationConfig result = _configuration;
    readValues(result);
    return result;
}

void ConfigurationEditor::reload(const SimulationConfig& config)
{
    _configuration = config;
    _fields.clear();
    if (_stack == nullptr)
        return;
    while (QLayoutItem* item = _stack->takeAt(0)) {
        if (auto* widget = item->widget())
            delete widget;
        delete item;
    }
    buildTiles(config);
    if (auto* firstField = findChild<QSpinBox*>("particle_count"))
        firstField->setFocus();
}

QFormLayout* ConfigurationEditor::addTile(QVBoxLayout* stack, const QString& section,
                                          const QString& title, const QString& description)
{
    auto* tile = new QGroupBox(title, stack->parentWidget());
    tile->setMinimumWidth(0);
    tile->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    tile->setProperty("configTile", true);
    tile->setProperty("configSection", section);
    auto* layout = new QVBoxLayout(tile);
    auto* hint = new QLabel(description, tile);
    hint->setWordWrap(true);
    hint->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    hint->setObjectName("configurationTileHint");
    layout->addWidget(hint);
    auto* form = new QFormLayout();
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    layout->addLayout(form);
    stack->addWidget(tile);
    return form;
}

void ConfigurationEditor::buildTiles(const SimulationConfig& config)
{
    if (_stack == nullptr)
        return;
    addRuntimeTiles(config);
    _stack->addStretch(1);
}

void ConfigurationEditor::addSimulationTile(const SimulationConfig& config)
{
    auto* form = addTile(_stack, "simulation", "Simulation and Time",
                         "Solver, integrator, particle count and time control.");
    addInt(form, _fields, "particle_count",
           static_cast<int>(std::min<std::uint32_t>(config.particleCount, INT_MAX)), 2);
    addFloat(form, _fields, "dt", config.dt, 0.000001, 1000.0);
    addCombo(form, _fields, "solver", config.solver,
             {"pairwise_cuda", "octree_gpu", "octree_cpu", "fmm_cpu"});
    addCombo(form, _fields, "integrator", config.integrator, {"euler", "rk4", "leapfrog"});
    addCombo(form, _fields, "performance_profile", config.performanceProfile,
             {"interactive", "balanced", "quality", "custom"});
    addCombo(form, _fields, "simulation_profile", config.simulationProfile,
             {"", "disk_orbit", "galaxy_collision", "plummer_sphere", "binary_star", "solar_system",
              "sph_collapse"});
    addFloat(form, _fields, "substep_target_dt", config.substepTargetDt, 0.0, 1000.0);
    addInt(form, _fields, "max_substeps", static_cast<int>(config.maxSubsteps), 1, 4096);
    addInt(form, _fields, "snapshot_publish_period_ms",
           static_cast<int>(config.snapshotPublishPeriodMs), 1, 60000);
    addBool(form, _fields, "adaptive_time_steps", config.adaptiveTimeStepsEnabled);
    addInt(form, _fields, "adaptive_max_level", static_cast<int>(config.adaptiveTimeStepMaxLevel),
           0, 32);
    addFloat(form, _fields, "adaptive_eta", config.adaptiveTimeStepEta, 0.001, 1.0);
    addBool(form, _fields, "adaptive_cost_guard", config.adaptiveTimeStepCostGuard);
}

void ConfigurationEditor::addSceneTile(const SimulationConfig& config)
{
    auto* form = addTile(_stack, "scene", "Scene Selection",
                         "Choose preset, detailed mode, input file and generation seed.");
    addCombo(form, _fields, "init_config_style", config.initConfigStyle, {"preset", "detailed"});
    addCombo(form, _fields, "preset_structure", config.presetStructure,
             {"disk_orbit", "random_cloud", "cube_random", "sphere_random", "cosmology", "two_body",
              "three_body", "plummer_sphere", "galaxy_collision", "binary_star", "solar_system",
              "sph_collapse", "file"});
    addCombo(form, _fields, "init_mode", config.initMode,
             {"disk_orbit", "random_cloud", "cube_random", "sphere_random", "cosmology", "two_body",
              "three_body", "plummer_sphere", "galaxy_collision", "binary_star", "solar_system",
              "sph_collapse", "file"});
    addText(form, _fields, "input_file", config.inputFile);
    addCombo(form, _fields, "input_format", config.inputFormat,
             {"auto", "bin", "vtk", "vtk_binary", "xyz"});
    addFloat(form, _fields, "preset_size", config.presetSize, 0.01, 1.0e9);
    addFloat(form, _fields, "velocity_temperature", config.velocityTemperature);
    addFloat(form, _fields, "particle_temperature", config.particleTemperature);
    addInt(form, _fields, "init_seed",
           static_cast<int>(std::min<std::uint32_t>(config.initSeed, INT_MAX)), 0);
    addBool(form, _fields, "init_include_central_body", config.initIncludeCentralBody);
    addBool(form, _fields, "deterministic_mode", config.deterministicMode);
    addBool(form, _fields, "cosmology_enabled", config.cosmologyEnabled);
    addCombo(form, _fields, "cosmology_mode", config.cosmologyMode,
             {"expanding_preview", "comoving"});
    addCombo(form, _fields, "cosmology_geometry", config.cosmologyGeometry, {"sphere", "cube"});
    addFloat(form, _fields, "cosmology_box_half_extent", config.cosmologyBoxHalfExtent, 0.000001,
             1.0e9);
    addFloat(form, _fields, "cosmology_sphere_radius", config.cosmologySphereRadius, 0.000001,
             1.0e9);
    addFloat(form, _fields, "cosmology_h0", config.cosmologyHubbleH0, 0.000001, 1.0e6);
    addFloat(form, _fields, "cosmology_omega_m", config.cosmologyOmegaMatter, 0.0, 10.0);
    addFloat(form, _fields, "cosmology_omega_lambda", config.cosmologyOmegaLambda, 0.0, 10.0);
    addFloat(form, _fields, "cosmology_omega_radiation", config.cosmologyOmegaRadiation, 0.0, 10.0);
    addFloat(form, _fields, "cosmology_initial_scale_factor", config.cosmologyInitialScaleFactor,
             0.000001, 1.0e12);
    addFloat(form, _fields, "cosmology_perturbation_amplitude",
             config.cosmologyPerturbationAmplitude, 0.0, 1.0);
    addFloat(form, _fields, "cosmology_peculiar_velocity_scale",
             config.cosmologyPeculiarVelocityScale, 0.0, 1.0e6);
}

void ConfigurationEditor::addSceneTransformTile(const SimulationConfig& config)
{
    auto* form = addTile(_stack, "transform", "Scene Transform",
                         "Translate, rotate, mirror and repeat the loaded model.");
    addFloat(form, _fields, "scene_offset_x", config.sceneOffsetX, -1.0e9, 1.0e9, 3);
    addFloat(form, _fields, "scene_offset_y", config.sceneOffsetY, -1.0e9, 1.0e9, 3);
    addFloat(form, _fields, "scene_offset_z", config.sceneOffsetZ, -1.0e9, 1.0e9, 3);
    addFloat(form, _fields, "scene_rotation_x", config.sceneRotationX, -360.0, 360.0, 3);
    addFloat(form, _fields, "scene_rotation_y", config.sceneRotationY, -360.0, 360.0, 3);
    addFloat(form, _fields, "scene_rotation_z", config.sceneRotationZ, -360.0, 360.0, 3);
    addCombo(form, _fields, "scene_copy_axis", config.sceneCopyAxis, {"x", "y", "z"});
    addInt(form, _fields, "scene_rotation_copies",
           static_cast<int>(std::clamp(config.sceneRotationCopies, 1u, 256u)), 1, 256);
    addBool(form, _fields, "scene_mirror_x", config.sceneMirrorX);
    addBool(form, _fields, "scene_mirror_y", config.sceneMirrorY);
    addBool(form, _fields, "scene_mirror_z", config.sceneMirrorZ);
}

void ConfigurationEditor::addBodyTiles(const SimulationConfig& config)
{
    auto* layout = _stack;
    auto* central = addTile(layout, "central_body", "Central Body",
                            "Mass, position and velocity of the primary body.");
    addFloat(central, _fields, "init_central_mass", config.initCentralMass, 0.0, 1.0e15);
    addFloat(central, _fields, "init_central_x", config.initCentralX);
    addFloat(central, _fields, "init_central_y", config.initCentralY);
    addFloat(central, _fields, "init_central_z", config.initCentralZ);
    addFloat(central, _fields, "init_central_vx", config.initCentralVx);
    addFloat(central, _fields, "init_central_vy", config.initCentralVy);
    addFloat(central, _fields, "init_central_vz", config.initCentralVz);
    auto* disk = addTile(layout, "disk", "Disk, Rings and Orbits",
                         "Configure circular structures and orbital velocity.");
    addFloat(disk, _fields, "init_disk_mass", config.initDiskMass, 0.0, 1.0e15);
    addFloat(disk, _fields, "init_disk_radius_min", config.initDiskRadiusMin, 0.0, 1.0e9);
    addFloat(disk, _fields, "init_disk_radius_max", config.initDiskRadiusMax, 0.0, 1.0e9);
    addFloat(disk, _fields, "init_disk_thickness", config.initDiskThickness, 0.0, 1.0e9);
    addFloat(disk, _fields, "init_velocity_scale", config.initVelocityScale, 0.0, 1.0e6);
    auto* cloud =
        addTile(layout, "cloud", "Cloud, Plummer and Bodies",
                "Shared spatial controls for clouds, Plummer spheres and multi-body presets.");
    addFloat(cloud, _fields, "init_cloud_half_extent", config.initCloudHalfExtent, 0.0, 1.0e9);
    addFloat(cloud, _fields, "init_cube_half_extent", config.initCubeHalfExtent, 0.0, 1.0e9);
    addFloat(cloud, _fields, "init_sphere_radius", config.initSphereRadius, 0.0, 1.0e9);
    addFloat(cloud, _fields, "init_cloud_speed", config.initCloudSpeed, 0.0, 1.0e9);
    addFloat(cloud, _fields, "init_particle_mass", config.initParticleMass, 0.0, 1.0e15);
}

void ConfigurationEditor::addPhysicsTiles(const SimulationConfig& config)
{
    auto* layout = _stack;
    auto* octree = addTile(layout, "physics", "Octree and TreePM",
                           "Approximation, mesh, precision and local correction controls.");
    addFloat(octree, _fields, "octree_theta", config.octreeTheta, 0.01, 10.0);
    addFloat(octree, _fields, "octree_softening", config.octreeSoftening, 0.000001, 1.0e9);
    addCombo(octree, _fields, "octree_opening_criterion", config.octreeOpeningCriterion,
             {"com", "bounds"});
    addBool(octree, _fields, "octree_theta_auto_tune", config.octreeThetaAutoTune);
    addFloat(octree, _fields, "octree_theta_auto_min", config.octreeThetaAutoMin, 0.01, 10.0);
    addFloat(octree, _fields, "octree_theta_auto_max", config.octreeThetaAutoMax, 0.01, 10.0);
    addInt(octree, _fields, "linear_octree_leaf_capacity",
           static_cast<int>(config.linearOctreeLeafCapacity), 1, 4096);
    addCombo(octree, _fields, "cuda_cache_preference", config.cudaCachePreference,
             {"default", "l1", "shared"});
    addBool(octree, _fields, "treepm_enabled", config.treePmEnabled);
    addCombo(octree, _fields, "treepm_preset", config.treePmPreset,
             {"custom", "pm_only", "local_grid_fast", "hybrid_balanced", "hybrid_quality",
              "tree_quality"});
    addCombo(octree, _fields, "treepm_model", config.treePmModel,
             {"auto", "pm_only", "local_grid", "tree", "hybrid", "exact_tree"});
    addCombo(octree, _fields, "treepm_layout", config.treePmLayout,
             {"auto", "linear", "gather_linear", "gather_morton"});
    addCombo(octree, _fields, "treepm_precision", config.treePmPrecision, {"fp32", "fp64"});
    addCombo(octree, _fields, "treepm_assignment", config.treePmAssignment, {"cic", "tsc", "pcs"});
    addBool(octree, _fields, "treepm_local_grid", config.treePmLocalGrid);
    addInt(octree, _fields, "treepm_grid_size", static_cast<int>(config.treePmGridSize), 8, 1024);
    addInt(octree, _fields, "treepm_jacobi_iterations",
           static_cast<int>(config.treePmJacobiIterations), 0, 4096);
    addFloat(octree, _fields, "treepm_cutoff_factor", config.treePmCutoffFactor, 0.0, 100.0);
    addInt(octree, _fields, "treepm_max_local_neighbors",
           static_cast<int>(config.treePmMaxLocalNeighbors), 0, 4096);
    addInt(octree, _fields, "treepm_particle_limit",
           static_cast<int>(std::min<std::uint32_t>(config.treePmParticleLimit, INT_MAX)), 0);
    addInt(octree, _fields, "treepm_dense_cell_threshold",
           static_cast<int>(config.treePmDenseCellThreshold), 1, 65536);
    addBool(octree, _fields, "treepm_gravity_only_buffers", config.treePmGravityOnlyBuffers);
    auto* thermal = addTile(layout, "thermal_sph", "Thermal and SPH",
                            "Gas, heating, radiation and hydrodynamic stabilization.");
    addFloat(thermal, _fields, "thermal_ambient_temperature", config.thermalAmbientTemperature);
    addFloat(thermal, _fields, "thermal_specific_heat", config.thermalSpecificHeat, 0.000001,
             1.0e9);
    addFloat(thermal, _fields, "thermal_heating_coeff", config.thermalHeatingCoeff, 0.0, 1.0e9);
    addFloat(thermal, _fields, "thermal_radiation_coeff", config.thermalRadiationCoeff, 0.0, 1.0e9);
    addBool(thermal, _fields, "sph_enabled", config.sphEnabled);
    addFloat(thermal, _fields, "sph_smoothing_length", config.sphSmoothingLength, 0.000001, 1.0e9);
    addFloat(thermal, _fields, "sph_rest_density", config.sphRestDensity, 0.000001, 1.0e12);
    addFloat(thermal, _fields, "sph_gas_constant", config.sphGasConstant, 0.0, 1.0e12);
    addFloat(thermal, _fields, "sph_viscosity", config.sphViscosity, 0.0, 1.0e12);
    addFloat(thermal, _fields, "sph_max_acceleration", config.sphMaxAcceleration, 0.0, 1.0e12);
    addFloat(thermal, _fields, "sph_max_speed", config.sphMaxSpeed, 0.0, 1.0e12);
    auto* numerical = addTile(layout, "numerical", "Numerical Limits and Diagnostics",
                              "Safety clamps and energy sampling controls.");
    addFloat(numerical, _fields, "physics_max_acceleration", config.physicsMaxAcceleration, 0.0,
             1.0e12);
    addFloat(numerical, _fields, "physics_min_softening", config.physicsMinSoftening, 0.000001,
             1.0e12);
    addFloat(numerical, _fields, "physics_min_distance2", config.physicsMinDistance2,
             0.000000000001, 1.0e12, 12);
    addFloat(numerical, _fields, "physics_min_theta", config.physicsMinTheta, 0.000001, 10.0);
    addInt(numerical, _fields, "energy_measure_every_steps",
           static_cast<int>(config.energyMeasureEverySteps), 1, INT_MAX);
    addInt(numerical, _fields, "energy_sample_limit", static_cast<int>(config.energySampleLimit), 1,
           INT_MAX);
}

void ConfigurationEditor::addRuntimeTiles(const SimulationConfig& config)
{
    auto* advanced = addTile(_stack, "advanced", "Advanced I/O",
                             "Only settings not exposed by Physics, Scene or Render.");
    addText(advanced, _fields, "input_file", config.inputFile);
    addCombo(advanced, _fields, "input_format", config.inputFormat,
             {"auto", "bin", "vtk", "vtk_binary", "xyz"});
    addText(advanced, _fields, "export_directory", config.exportDirectory);
    addCombo(advanced, _fields, "export_format", config.exportFormat,
             {"vtk", "vtk_binary", "xyz", "bin"});
    addInt(advanced, _fields, "client_remote_command_timeout_ms",
           static_cast<int>(config.clientRemoteCommandTimeoutMs), 1, 600000);
    addInt(advanced, _fields, "client_remote_status_timeout_ms",
           static_cast<int>(config.clientRemoteStatusTimeoutMs), 1, 600000);
    addInt(advanced, _fields, "client_remote_snapshot_timeout_ms",
           static_cast<int>(config.clientRemoteSnapshotTimeoutMs), 1, 600000);
    addInt(advanced, _fields, "client_snapshot_queue_capacity",
           static_cast<int>(config.clientSnapshotQueueCapacity), 1, 1024);
    addCombo(advanced, _fields, "client_snapshot_drop_policy", config.clientSnapshotDropPolicy,
             {"latest-only", "paced"});
}

void ConfigurationEditor::readValues(SimulationConfig& config) const
{
    readInt(_fields, "particle_count", config.particleCount);
    readFloat(_fields, "dt", config.dt);
    readString(_fields, "solver", config.solver);
    readString(_fields, "integrator", config.integrator);
    readString(_fields, "performance_profile", config.performanceProfile);
    readString(_fields, "simulation_profile", config.simulationProfile);
    readFloat(_fields, "substep_target_dt", config.substepTargetDt);
    readInt(_fields, "max_substeps", config.maxSubsteps);
    readInt(_fields, "snapshot_publish_period_ms", config.snapshotPublishPeriodMs);
    readBool(_fields, "adaptive_time_steps", config.adaptiveTimeStepsEnabled);
    readInt(_fields, "adaptive_max_level", config.adaptiveTimeStepMaxLevel);
    readFloat(_fields, "adaptive_eta", config.adaptiveTimeStepEta);
    readBool(_fields, "adaptive_cost_guard", config.adaptiveTimeStepCostGuard);
    readString(_fields, "init_config_style", config.initConfigStyle);
    readString(_fields, "preset_structure", config.presetStructure);
    readString(_fields, "init_mode", config.initMode);
    readString(_fields, "input_file", config.inputFile);
    readString(_fields, "input_format", config.inputFormat);
    readFloat(_fields, "preset_size", config.presetSize);
    readFloat(_fields, "velocity_temperature", config.velocityTemperature);
    readFloat(_fields, "particle_temperature", config.particleTemperature);
    readInt(_fields, "init_seed", config.initSeed);
    readBool(_fields, "init_include_central_body", config.initIncludeCentralBody);
    readBool(_fields, "deterministic_mode", config.deterministicMode);
    readBool(_fields, "cosmology_enabled", config.cosmologyEnabled);
    readString(_fields, "cosmology_mode", config.cosmologyMode);
    readString(_fields, "cosmology_geometry", config.cosmologyGeometry);
    readFloat(_fields, "cosmology_box_half_extent", config.cosmologyBoxHalfExtent);
    readFloat(_fields, "cosmology_sphere_radius", config.cosmologySphereRadius);
    readFloat(_fields, "cosmology_h0", config.cosmologyHubbleH0);
    readFloat(_fields, "cosmology_omega_m", config.cosmologyOmegaMatter);
    readFloat(_fields, "cosmology_omega_lambda", config.cosmologyOmegaLambda);
    readFloat(_fields, "cosmology_omega_radiation", config.cosmologyOmegaRadiation);
    readFloat(_fields, "cosmology_initial_scale_factor", config.cosmologyInitialScaleFactor);
    readFloat(_fields, "cosmology_perturbation_amplitude", config.cosmologyPerturbationAmplitude);
    readFloat(_fields, "cosmology_peculiar_velocity_scale", config.cosmologyPeculiarVelocityScale);
    readFloat(_fields, "init_central_mass", config.initCentralMass);
    readFloat(_fields, "init_central_x", config.initCentralX);
    readFloat(_fields, "init_central_y", config.initCentralY);
    readFloat(_fields, "init_central_z", config.initCentralZ);
    readFloat(_fields, "init_central_vx", config.initCentralVx);
    readFloat(_fields, "init_central_vy", config.initCentralVy);
    readFloat(_fields, "init_central_vz", config.initCentralVz);
    readFloat(_fields, "init_disk_mass", config.initDiskMass);
    readFloat(_fields, "init_disk_radius_min", config.initDiskRadiusMin);
    readFloat(_fields, "init_disk_radius_max", config.initDiskRadiusMax);
    readFloat(_fields, "init_disk_thickness", config.initDiskThickness);
    readFloat(_fields, "init_velocity_scale", config.initVelocityScale);
    readFloat(_fields, "init_cloud_half_extent", config.initCloudHalfExtent);
    readFloat(_fields, "init_cube_half_extent", config.initCubeHalfExtent);
    readFloat(_fields, "init_sphere_radius", config.initSphereRadius);
    readFloat(_fields, "init_cloud_speed", config.initCloudSpeed);
    readFloat(_fields, "init_particle_mass", config.initParticleMass);
    readFloat(_fields, "scene_offset_x", config.sceneOffsetX);
    readFloat(_fields, "scene_offset_y", config.sceneOffsetY);
    readFloat(_fields, "scene_offset_z", config.sceneOffsetZ);
    readFloat(_fields, "scene_rotation_x", config.sceneRotationX);
    readFloat(_fields, "scene_rotation_y", config.sceneRotationY);
    readFloat(_fields, "scene_rotation_z", config.sceneRotationZ);
    readString(_fields, "scene_copy_axis", config.sceneCopyAxis);
    readInt(_fields, "scene_rotation_copies", config.sceneRotationCopies);
    readBool(_fields, "scene_mirror_x", config.sceneMirrorX);
    readBool(_fields, "scene_mirror_y", config.sceneMirrorY);
    readBool(_fields, "scene_mirror_z", config.sceneMirrorZ);
    readFloat(_fields, "octree_theta", config.octreeTheta);
    readFloat(_fields, "octree_softening", config.octreeSoftening);
    readString(_fields, "octree_opening_criterion", config.octreeOpeningCriterion);
    readBool(_fields, "octree_theta_auto_tune", config.octreeThetaAutoTune);
    readFloat(_fields, "octree_theta_auto_min", config.octreeThetaAutoMin);
    readFloat(_fields, "octree_theta_auto_max", config.octreeThetaAutoMax);
    readInt(_fields, "linear_octree_leaf_capacity", config.linearOctreeLeafCapacity);
    readString(_fields, "cuda_cache_preference", config.cudaCachePreference);
    readBool(_fields, "treepm_enabled", config.treePmEnabled);
    readString(_fields, "treepm_preset", config.treePmPreset);
    readString(_fields, "treepm_model", config.treePmModel);
    readString(_fields, "treepm_layout", config.treePmLayout);
    readString(_fields, "treepm_precision", config.treePmPrecision);
    readString(_fields, "treepm_assignment", config.treePmAssignment);
    readBool(_fields, "treepm_local_grid", config.treePmLocalGrid);
    readInt(_fields, "treepm_grid_size", config.treePmGridSize);
    readInt(_fields, "treepm_jacobi_iterations", config.treePmJacobiIterations);
    readFloat(_fields, "treepm_cutoff_factor", config.treePmCutoffFactor);
    readInt(_fields, "treepm_max_local_neighbors", config.treePmMaxLocalNeighbors);
    readInt(_fields, "treepm_particle_limit", config.treePmParticleLimit);
    readInt(_fields, "treepm_dense_cell_threshold", config.treePmDenseCellThreshold);
    readBool(_fields, "treepm_gravity_only_buffers", config.treePmGravityOnlyBuffers);
    readFloat(_fields, "thermal_ambient_temperature", config.thermalAmbientTemperature);
    readFloat(_fields, "thermal_specific_heat", config.thermalSpecificHeat);
    readFloat(_fields, "thermal_heating_coeff", config.thermalHeatingCoeff);
    readFloat(_fields, "thermal_radiation_coeff", config.thermalRadiationCoeff);
    readBool(_fields, "sph_enabled", config.sphEnabled);
    readFloat(_fields, "sph_smoothing_length", config.sphSmoothingLength);
    readFloat(_fields, "sph_rest_density", config.sphRestDensity);
    readFloat(_fields, "sph_gas_constant", config.sphGasConstant);
    readFloat(_fields, "sph_viscosity", config.sphViscosity);
    readFloat(_fields, "sph_max_acceleration", config.sphMaxAcceleration);
    readFloat(_fields, "sph_max_speed", config.sphMaxSpeed);
    readFloat(_fields, "physics_max_acceleration", config.physicsMaxAcceleration);
    readFloat(_fields, "physics_min_softening", config.physicsMinSoftening);
    readFloat(_fields, "physics_min_distance2", config.physicsMinDistance2);
    readFloat(_fields, "physics_min_theta", config.physicsMinTheta);
    readInt(_fields, "energy_measure_every_steps", config.energyMeasureEverySteps);
    readInt(_fields, "energy_sample_limit", config.energySampleLimit);
    readInt(_fields, "client_particle_cap", config.clientParticleCap);
    readFloat(_fields, "default_zoom", config.defaultZoom);
    readInt(_fields, "default_luminosity", config.defaultLuminosity);
    readString(_fields, "ui_theme", config.uiTheme);
    readInt(_fields, "ui_fps_limit", config.uiFpsLimit);
    readInt(_fields, "client_remote_command_timeout_ms", config.clientRemoteCommandTimeoutMs);
    readInt(_fields, "client_remote_status_timeout_ms", config.clientRemoteStatusTimeoutMs);
    readInt(_fields, "client_remote_snapshot_timeout_ms", config.clientRemoteSnapshotTimeoutMs);
    readInt(_fields, "client_snapshot_queue_capacity", config.clientSnapshotQueueCapacity);
    readString(_fields, "client_snapshot_drop_policy", config.clientSnapshotDropPolicy);
    readString(_fields, "export_directory", config.exportDirectory);
    readString(_fields, "export_format", config.exportFormat);
    readBool(_fields, "render_culling_enabled", config.renderCullingEnabled);
    readBool(_fields, "render_lod_enabled", config.renderLODEnabled);
    readFloat(_fields, "render_lod_near_distance", config.renderLODNearDistance);
    readFloat(_fields, "render_lod_far_distance", config.renderLODFarDistance);
    if (config.cosmologyEnabled && config.cosmologyMode == "comoving") {
        // This mode is a separate periodic PM solver, not a visual-expansion preset.
        config.initMode = "cosmology";
        config.presetStructure = "cosmology";
        // CUDA comoving PM is not qualified yet; selecting this GUI mode must keep a visible,
        // reproducible reference path rather than start a stalled renderer.
        config.solver = "octree_cpu";
        config.integrator = "leapfrog";
        config.cosmologyGeometry = "cube";
        config.treePmEnabled = true;
        config.treePmPreset = "pm_only";
        config.treePmModel = "pm_only";
        config.treePmAssignment = "tsc";
        config.treePmLocalGrid = false;
        config.sphEnabled = false;
        config.adaptiveTimeStepsEnabled = false;
    }
}
} // namespace bltzr_qt
