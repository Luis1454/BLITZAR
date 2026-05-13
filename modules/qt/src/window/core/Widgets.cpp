/*
 * @file modules/qt/src/window/core/Widgets.cpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Qt window widget ownership group construction.
 */

#include "window/core/Widgets.hpp"
#include "widgets/graphs/Graph.hpp"
#include "widgets/viewport/MultiView.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QTimer>

namespace bltzr_qt {
ViewWidgets::ViewWidgets(QWidget*)
    : multiView(new MultiView()),
      energyGraph(new Graph())
{
}

TelemetryWidgets::TelemetryWidgets(QWidget* parent)
    : validationLabel(new QLabel(parent)),
      statusLabel(new QLabel(parent)),
      runtimeMetricsLabel(new QLabel(parent)),
      queueMetricsLabel(new QLabel(parent)),
      energyMetricsLabel(new QLabel(parent)),
      gpuMetricsLabel(new QLabel(parent))
{
}

RunControls::RunControls(QWidget* parent)
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

SceneControls::SceneControls(QWidget* parent)
    : exportButton(new QPushButton("Export", parent)),
      saveConfigButton(new QPushButton("Save config", parent)),
      loadInputButton(new QPushButton("Load input", parent)),
      simulationProfileCombo(new QComboBox(parent)),
      presetCombo(new QComboBox(parent)),
      applyPresetButton(new QPushButton("Apply preset", parent)),
      loadPresetButton(new QPushButton("Load preset file", parent))
{
}

PhysicsControls::PhysicsControls(QWidget* parent)
    : sphCheck(new QCheckBox("SPH", parent)),
      sphSmoothingSpin(new QDoubleSpinBox(parent)),
      sphRestDensitySpin(new QDoubleSpinBox(parent)),
      sphGasConstantSpin(new QDoubleSpinBox(parent)),
      sphViscositySpin(new QDoubleSpinBox(parent)),
      dtSpin(new QDoubleSpinBox(parent)),
      solverCombo(new QComboBox(parent)),
      integratorCombo(new QComboBox(parent)),
      thetaSpin(new QDoubleSpinBox(parent)),
      softeningSpin(new QDoubleSpinBox(parent))
{
}

RenderControls::RenderControls(QWidget* parent)
    : zoomSlider(new QSlider(Qt::Horizontal, parent)),
      luminositySlider(new QSlider(Qt::Horizontal, parent)),
      view3dCombo(new QComboBox(parent)),
      yawSlider(new QSlider(Qt::Horizontal, parent)),
      pitchSlider(new QSlider(Qt::Horizontal, parent)),
      rollSlider(new QSlider(Qt::Horizontal, parent)),
      cullingCheck(new QCheckBox("Culling", parent)),
      lodCheck(new QCheckBox("LOD", parent)),
      octreeOverlayCheck(new QCheckBox("Octree overlay", parent)),
      octreeOverlayDepthSpin(new QSpinBox(parent)),
      octreeOverlayOpacitySpin(new QSpinBox(parent)),
      gpuTelemetryCheck(new QCheckBox("GPU telemetry", parent))
{
}

WorkspaceWidgets::WorkspaceWidgets(QWidget* parent)
    : octreeOverlayAction(nullptr),
      gpuTelemetryAction(nullptr),
      controlsDock(nullptr),
      energyDock(nullptr),
      telemetryDock(nullptr),
      validationDock(nullptr),
      timer(new QTimer(parent))
{
}

Widgets::Widgets(QWidget* parent)
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
