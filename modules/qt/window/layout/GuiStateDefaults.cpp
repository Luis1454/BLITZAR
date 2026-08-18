/*
 * @file modules/qt/window/layout/GuiStateDefaults.cpp
 * @brief Default choices and presentation state for Window controls.
 */

#include "core/constants/FndConstants.hpp"
#include "support/types/GuiEnums.hpp"
#include "window/core/GuiWindow.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QStringList>
#include <algorithm>

namespace bltzr_qt {

const QStringList kSolverList = {QString::fromStdString(to_string(Solver::PairwiseCuda)),
                                 QString::fromStdString(to_string(Solver::OctreeGpu)),
                                 QString::fromStdString(to_string(Solver::OctreeCpu)),
                                 QString::fromStdString(to_string(Solver::FmmCpu))};
const QStringList kIntegratorList = {
    QString::fromStdString(to_string(Integrator::Euler)),
    QString::fromStdString(to_string(Integrator::Rk4)),
    QString::fromStdString(to_string(Integrator::Leapfrog))};
const QStringList kTreePmPresetList = {
    "custom", "pm_only", "local_grid_fast", "hybrid_balanced", "hybrid_quality", "tree_quality"};
const QStringList kTreePmModelList = {"auto", "pm_only", "local_grid", "tree", "hybrid", "exact_tree"};
const QStringList kTreePmLayoutList = {"auto", "linear", "gather_linear", "gather_morton"};
const QStringList kTreePmPrecisionList = {"fp32", "fp64"};
const QStringList kTreePmAssignmentList = {"cic", "tsc", "pcs"};
const QStringList kPerformanceList = {
    QString::fromStdString(to_string(PerformanceProfile::Interactive)),
    QString::fromStdString(to_string(PerformanceProfile::Balanced)),
    QString::fromStdString(to_string(PerformanceProfile::Quality)),
    QString::fromStdString(to_string(PerformanceProfile::Custom))};
const QStringList kSimulationProfiles = {
    "disk_orbit", "galaxy_collision", "plummer_sphere", "binary_star", "solar_system", "sph_collapse"};
const QStringList kPresets = {"disk_orbit",    "galaxy_collision", "cosmology",      "random_cloud",
                              "cube_random",   "sphere_random",     "two_body",       "three_body",
                              "plummer_sphere", "binary_star",       "solar_system",   "sph_collapse",
                              "file"};
const QStringList kView3dModes = {"perspective", "iso"};

void Window::initializeComboBoxes()
{
    _widgets.run.pauseButton->setCheckable(true);
    _widgets.physics.solverCombo->addItems(kSolverList);
    _widgets.physics.solverCombo->setCurrentIndex(
        std::max(0, _widgets.physics.solverCombo->findText(QString::fromStdString(_config.solver))));
    _widgets.physics.integratorCombo->addItems(kIntegratorList);
    _widgets.physics.integratorCombo->setCurrentIndex(
        std::max(0, _widgets.physics.integratorCombo->findText(QString::fromStdString(_config.integrator))));
    _widgets.physics.treePmPresetCombo->addItems(kTreePmPresetList);
    _widgets.physics.treePmPresetCombo->setCurrentIndex(
        std::max(0, _widgets.physics.treePmPresetCombo->findText(QString::fromStdString(_config.treePmPreset))));
    _widgets.physics.treePmModelCombo->addItems(kTreePmModelList);
    _widgets.physics.treePmModelCombo->setCurrentIndex(
        std::max(0, _widgets.physics.treePmModelCombo->findText(QString::fromStdString(_config.treePmModel))));
    _widgets.physics.treePmLayoutCombo->addItems(kTreePmLayoutList);
    _widgets.physics.treePmLayoutCombo->setCurrentIndex(
        std::max(0, _widgets.physics.treePmLayoutCombo->findText(QString::fromStdString(_config.treePmLayout))));
    _widgets.physics.treePmPrecisionCombo->addItems(kTreePmPrecisionList);
    _widgets.physics.treePmPrecisionCombo->setCurrentIndex(
        std::max(0, _widgets.physics.treePmPrecisionCombo->findText(QString::fromStdString(_config.treePmPrecision))));
    _widgets.physics.treePmAssignmentCombo->addItems(kTreePmAssignmentList);
    _widgets.physics.treePmAssignmentCombo->setCurrentIndex(
        std::max(0, _widgets.physics.treePmAssignmentCombo->findText(QString::fromStdString(_config.treePmAssignment))));
    _widgets.run.performanceCombo->addItems(kPerformanceList);
    _widgets.run.performanceCombo->setCurrentIndex(
        std::max(0, _widgets.run.performanceCombo->findText(QString::fromStdString(_config.performanceProfile))));
    _widgets.scene.simulationProfileCombo->addItems(kSimulationProfiles);
    _widgets.scene.simulationProfileCombo->setCurrentIndex(
        std::max(0, _widgets.scene.simulationProfileCombo->findText(QString::fromStdString(_config.simulationProfile))));
    _widgets.scene.presetCombo->addItems(kPresets);
    _widgets.scene.presetCombo->setCurrentIndex(
        std::max(0, _widgets.scene.presetCombo->findText(QString::fromStdString(_config.presetStructure))));
    _widgets.render.view3dCombo->addItems(kView3dModes);
}

void Window::initializeLabelsAndTooltips()
{
    _widgets.run.serverHostEdit->setText(kDefaultLoopbackHost);
    _widgets.run.serverAutostartCheck->setChecked(true);
    _widgets.run.serverPortSpin->setRange(kNetworkPortMin, kNetworkPortMax);
    _widgets.run.serverPortSpin->setValue(kDefaultServerPort);
    _widgets.run.serverBinEdit->setPlaceholderText("blitzar-server(.exe)");
    _widgets.run.serverBinEdit->setToolTip("Path to the server executable used when autostart is enabled");
    _widgets.run.applyConnectorButton->setToolTip("Apply host, port and server binary settings, then reconnect now");
    for (QLabel* label : {_widgets.telemetry.validationLabel, _widgets.telemetry.statusLabel,
                          _widgets.telemetry.runtimeMetricsLabel, _widgets.telemetry.queueMetricsLabel,
                          _widgets.telemetry.energyMetricsLabel, _widgets.telemetry.gpuMetricsLabel}) {
        if (label) {
            label->setWordWrap(true);
            label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        }
    }
    if (_widgets.telemetry.validationLabel) {
        _widgets.telemetry.validationLabel->setObjectName("validationLabel");
        _widgets.telemetry.validationLabel->setContentsMargins(6, 4, 6, 4);
    }
    for (QLabel* label : {_widgets.telemetry.statusLabel, _widgets.telemetry.runtimeMetricsLabel,
                          _widgets.telemetry.queueMetricsLabel, _widgets.telemetry.energyMetricsLabel,
                          _widgets.telemetry.gpuMetricsLabel}) {
        if (label)
            label->setObjectName("runtimeSummaryValue");
    }
    if (_widgets.render.exportProgress) {
        _widgets.render.exportProgress->setObjectName("exportProgressBar");
        _widgets.render.exportProgress->setRange(0, 100);
        _widgets.render.exportProgress->setValue(0);
        _widgets.render.exportProgress->setFormat("Export idle");
    }
}

} // namespace bltzr_qt
