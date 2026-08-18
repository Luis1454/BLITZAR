/*
 * @file modules/qt/window/core/GuiWidgets.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt window widget ownership group construction.
 */

#include "window/core/GuiWidgets.hpp"
#include "widgets/graphs/GuiGraph.hpp"
#include "widgets/graphs/GuiSpectrumGraph.hpp"
#include "widgets/viewport/GuiMultiView.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QSlider>
#include <QSpinBox>
#include <QTimer>

namespace bltzr_qt {
ViewWidgets::ViewWidgets(QPointer<QWidget> parent)
    : multiView(new MultiView(parent)),
      energyGraph(new Graph(parent)),
      spectrumGraph(new SpectrumGraph(parent))
{
}

TelemetryWidgets::TelemetryWidgets(QPointer<QWidget> parent)
    : validationLabel(new QLabel(parent)),
      statusLabel(new QLabel(parent)),
      runtimeMetricsLabel(new QLabel(parent)),
      queueMetricsLabel(new QLabel(parent)),
      energyMetricsLabel(new QLabel(parent)),
      gpuMetricsLabel(new QLabel(parent))
{
}

RunControls::RunControls(QPointer<QWidget> parent)
    : pauseButton(new QPushButton("Pause", parent)),
      stepButton(new QPushButton("Step", parent)),
      resetButton(new QPushButton("Reset", parent)),
      recoverButton(new QPushButton("Recover", parent)),
      applyConnectorButton(new QPushButton("Connect", parent)),
      serverAutostartCheck(new QCheckBox("autostart server", parent)),
      serverHostEdit(new QLineEdit(parent)),
      serverBinEdit(new QLineEdit(parent)),
      serverPortSpin(new QSpinBox(parent)),
      performanceCombo(new QComboBox(parent))
{
}

SceneControls::SceneControls(QPointer<QWidget> parent)
    : exportButton(new QPushButton("Export", parent)),
      saveConfigButton(new QPushButton("Save config", parent)),
      loadInputButton(new QPushButton("Load input", parent)),
      simulationProfileCombo(new QComboBox(parent)),
      presetCombo(new QComboBox(parent)),
      applyPresetButton(new QPushButton("Apply preset", parent)),
      loadPresetButton(new QPushButton("Load INI", parent))
{
}

PhysicsControls::PhysicsControls(QPointer<QWidget> parent)
    : sphCheck(new QCheckBox("SPH", parent)),
      sphSmoothingSpin(new QDoubleSpinBox(parent)),
      sphRestDensitySpin(new QDoubleSpinBox(parent)),
      sphGasConstantSpin(new QDoubleSpinBox(parent)),
      sphViscositySpin(new QDoubleSpinBox(parent)),
      dtSpin(new QDoubleSpinBox(parent)),
      particleCountSpin(new QSpinBox(parent)),
      solverCombo(new QComboBox(parent)),
      integratorCombo(new QComboBox(parent)),
      treePmEnabledCheck(new QCheckBox("Enable TreePM", parent)),
      treePmPresetCombo(new QComboBox(parent)),
      treePmModelCombo(new QComboBox(parent)),
      treePmLayoutCombo(new QComboBox(parent)),
      treePmPrecisionCombo(new QComboBox(parent)),
      treePmAssignmentCombo(new QComboBox(parent)),
      treePmLocalGridCheck(new QCheckBox("Local correction grid", parent)),
      treePmGridSizeSpin(new QSpinBox(parent)),
      treePmJacobiIterationsSpin(new QSpinBox(parent)),
      treePmCutoffFactorSpin(new QDoubleSpinBox(parent)),
      treePmMaxLocalNeighborsSpin(new QSpinBox(parent)),
      treePmParticleLimitSpin(new QSpinBox(parent)),
      treePmDenseCellThresholdSpin(new QSpinBox(parent)),
      treePmGravityOnlyBuffersCheck(new QCheckBox("Gravity-only buffers", parent)),
      adaptiveTimeStepsCheck(new QCheckBox("Adaptive per-body dt", parent)),
      adaptiveMaxLevelSpin(new QSpinBox(parent)),
      adaptiveEtaSpin(new QDoubleSpinBox(parent)),
      adaptiveCostGuardCheck(new QCheckBox("Cost guard (recommended)", parent)),
      thetaSpin(new QDoubleSpinBox(parent)),
      softeningSpin(new QDoubleSpinBox(parent))
{
}

RenderControls::RenderControls(QPointer<QWidget> parent)
    : zoomSlider(new QSlider(Qt::Horizontal, parent)),
      luminositySlider(new QSlider(Qt::Horizontal, parent)),
      view3dCombo(new QComboBox(parent)),
      yawSlider(new QSlider(Qt::Horizontal, parent)),
      pitchSlider(new QSlider(Qt::Horizontal, parent)),
      rollSlider(new QSlider(Qt::Horizontal, parent)),
      exportProgress(new QProgressBar(parent)),
      cullingCheck(new QCheckBox("Culling", parent)),
      lodCheck(new QCheckBox("LOD", parent)),
      octreeOverlayCheck(new QCheckBox("Octree overlay", parent)),
      octreeOverlayDepthSpin(new QSpinBox(parent)),
      octreeOverlayOpacitySpin(new QSpinBox(parent)),
      gpuTelemetryCheck(new QCheckBox("GPU telemetry", parent))
{
}

WorkspaceWidgets::WorkspaceWidgets(QPointer<QWidget> parent)
    : octreeOverlayAction(nullptr),
      gpuTelemetryAction(nullptr),
      controlsDock(nullptr),
      energyDock(nullptr),
      spectrumDock(nullptr),
      telemetryDock(nullptr),
      validationDock(nullptr),
      timer(new QTimer(parent))
{
}

Widgets::Widgets(QPointer<QWidget> parent)
    : view(parent),
      telemetry(parent),
      run(parent),
      scene(parent),
      physics(parent),
      render(parent),
      workspace(parent)
{
}
} // namespace bltzr_qt
