/*
 * @file modules/qt/ui/mainWindow/layout/propertyInitializer.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Property initialization implementations.
 */

#include "ui/MainWindow.hpp"
#include "ui/mainWindow/layout/propertyInitializer.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>

namespace bltzr_qt::mainWindow::layout {

void PropertyInitializer::initializeObjectNames(MainWindow* mainWindow)
{
    if (mainWindow == nullptr) {
        return;
    }

    mainWindow->_validationLabel->setObjectName("validationLabel");
    mainWindow->_statusLabel->setObjectName("statusLabel");
    mainWindow->_runtimeMetricsLabel->setObjectName("runtimeMetricsLabel");
    mainWindow->_queueMetricsLabel->setObjectName("queueMetricsLabel");
    mainWindow->_energyMetricsLabel->setObjectName("energyMetricsLabel");
    mainWindow->_gpuMetricsLabel->setObjectName("gpuMetricsLabel");
    mainWindow->_pauseButton->setObjectName("pauseButton");
    mainWindow->_stepButton->setObjectName("stepButton");
    mainWindow->_resetButton->setObjectName("resetButton");
    mainWindow->_recoverButton->setObjectName("recoverButton");
    mainWindow->_applyConnectorButton->setObjectName("applyConnectorButton");
    mainWindow->_exportButton->setObjectName("exportButton");
    mainWindow->_saveConfigButton->setObjectName("saveConfigButton");
    mainWindow->_loadInputButton->setObjectName("loadInputButton");
    mainWindow->_serverAutostartCheck->setObjectName("serverAutostartCheck");
    mainWindow->_serverHostEdit->setObjectName("serverHostEdit");
    mainWindow->_serverBinEdit->setObjectName("serverBinEdit");
    mainWindow->_serverPortSpin->setObjectName("serverPortSpin");
    mainWindow->_sphCheck->setObjectName("sphCheck");
    mainWindow->_sphSmoothingSpin->setObjectName("sphSmoothingSpin");
    mainWindow->_sphRestDensitySpin->setObjectName("sphRestDensitySpin");
    mainWindow->_sphGasConstantSpin->setObjectName("sphGasConstantSpin");
    mainWindow->_sphViscositySpin->setObjectName("sphViscositySpin");
    mainWindow->_dtSpin->setObjectName("dtSpin");
    mainWindow->_zoomSlider->setObjectName("zoomSlider");
    mainWindow->_luminositySlider->setObjectName("luminositySlider");
    mainWindow->_solverCombo->setObjectName("solverCombo");
    mainWindow->_integratorCombo->setObjectName("integratorCombo");
    mainWindow->_performanceCombo->setObjectName("performanceCombo");
    mainWindow->_simulationProfileCombo->setObjectName("simulationProfileCombo");
    mainWindow->_presetCombo->setObjectName("presetCombo");
    mainWindow->_view3dCombo->setObjectName("view3dCombo");
    mainWindow->_thetaSpin->setObjectName("thetaSpin");
    mainWindow->_softeningSpin->setObjectName("softeningSpin");
    mainWindow->_applyPresetButton->setObjectName("applyPresetButton");
    mainWindow->_loadPresetButton->setObjectName("loadPresetButton");
    mainWindow->_yawSlider->setObjectName("yawSlider");
    mainWindow->_pitchSlider->setObjectName("pitchSlider");
    mainWindow->_rollSlider->setObjectName("rollSlider");
    mainWindow->_cullingCheck->setObjectName("cullingCheck");
    mainWindow->_lodCheck->setObjectName("lodCheck");
    mainWindow->_octreeOverlayCheck->setObjectName("octreeOverlayCheck");
    mainWindow->_octreeOverlayDepthSpin->setObjectName("octreeOverlayDepthSpin");
    mainWindow->_octreeOverlayOpacitySpin->setObjectName("octreeOverlayOpacitySpin");
    mainWindow->_gpuTelemetryCheck->setObjectName("gpuTelemetryCheck");
}

void PropertyInitializer::initializeLabelsAndTooltips(MainWindow* mainWindow)
{
    if (mainWindow == nullptr) {
        return;
    }

    mainWindow->_validationLabel->setText("validation pending");
    mainWindow->_statusLabel->setText("workspace ready");
    mainWindow->_runtimeMetricsLabel->setText("runtime");
    mainWindow->_queueMetricsLabel->setText("queue");
    mainWindow->_energyMetricsLabel->setText("energy");
    mainWindow->_gpuMetricsLabel->setText("gpu");

    mainWindow->_pauseButton->setToolTip("Pause or resume the simulation runtime");
    mainWindow->_stepButton->setToolTip("Advance a single simulation step");
    mainWindow->_resetButton->setToolTip("Reset the current simulation");
    mainWindow->_recoverButton->setToolTip("Attempt to recover a faulted runtime");
    mainWindow->_applyConnectorButton->setToolTip("Apply the remote connector settings");
    mainWindow->_exportButton->setToolTip("Export the current snapshot");
    mainWindow->_saveConfigButton->setToolTip("Save the current config to disk");
    mainWindow->_loadInputButton->setToolTip("Load an input file");
    mainWindow->_serverAutostartCheck->setToolTip("Start the server automatically when connecting");
    mainWindow->_serverHostEdit->setToolTip("Remote server host name or address");
    mainWindow->_serverBinEdit->setToolTip("Path to the server executable");
    mainWindow->_serverPortSpin->setToolTip("Remote server port");
    mainWindow->_solverCombo->setToolTip("Select the simulation solver");
    mainWindow->_integratorCombo->setToolTip("Select the time integrator");
    mainWindow->_performanceCombo->setToolTip("Select the UI/performance profile");
    mainWindow->_simulationProfileCombo->setToolTip("Select a simulation profile");
    mainWindow->_presetCombo->setToolTip("Select the initial scene preset");
    mainWindow->_view3dCombo->setToolTip("Select the 3D viewport mode");
    mainWindow->_thetaSpin->setToolTip("Barnes-Hut opening angle");
    mainWindow->_softeningSpin->setToolTip("Octree softening distance");
    mainWindow->_sphCheck->setToolTip("Enable SPH physics");
    mainWindow->_sphSmoothingSpin->setToolTip("SPH smoothing length");
    mainWindow->_sphRestDensitySpin->setToolTip("SPH rest density");
    mainWindow->_sphGasConstantSpin->setToolTip("SPH gas constant");
    mainWindow->_sphViscositySpin->setToolTip("SPH viscosity");
    mainWindow->_dtSpin->setToolTip("Simulation timestep");
    mainWindow->_zoomSlider->setToolTip("Global zoom factor");
    mainWindow->_luminositySlider->setToolTip("Scene luminosity");
    mainWindow->_yawSlider->setToolTip("3D camera yaw");
    mainWindow->_pitchSlider->setToolTip("3D camera pitch");
    mainWindow->_rollSlider->setToolTip("3D camera roll");
    mainWindow->_cullingCheck->setToolTip("Enable particle culling");
    mainWindow->_lodCheck->setToolTip("Enable level-of-detail filtering");
    mainWindow->_octreeOverlayCheck->setToolTip("Show octree overlay");
    mainWindow->_octreeOverlayDepthSpin->setToolTip("Octree overlay depth");
    mainWindow->_octreeOverlayOpacitySpin->setToolTip("Octree overlay opacity");
    mainWindow->_gpuTelemetryCheck->setToolTip("Show GPU telemetry");
}

} // namespace bltzr_qt::mainWindow::layout
