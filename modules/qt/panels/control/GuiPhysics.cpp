/*
 * @file modules/qt/panels/control/GuiPhysics.cpp
 * @brief Implementation of the physics sidebar panel.
 */

#include "panels/control/GuiPhysics.hpp"
#include "panels/control/GuiDisclosure.hpp"
#include "window/core/GuiWidgets.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>

namespace bltzr_qt {

QWidget* buildPhysicsPanel(QWidget* parent, PhysicsControls& controls)
{
    auto* scroll = new QScrollArea(parent);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* page = new QWidget(scroll);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(8);

    auto* coreBox = new QGroupBox("Simulation", page);
    auto* coreLayout = new QVBoxLayout(coreBox);
    auto* coreForm = new QFormLayout();
    coreForm->addRow("Engine", controls.solverCombo.data());
    coreForm->addRow("Integrator", controls.integratorCombo.data());
    coreForm->addRow("Time step", controls.dtSpin.data());
    coreForm->addRow("Accuracy", controls.thetaSpin.data());
    coreForm->addRow("Softening", controls.softeningSpin.data());
    coreLayout->addLayout(coreForm);

    auto* treePmBox = new QGroupBox("Gravity model", page);
    auto* treePmLayout = new QVBoxLayout(treePmBox);
    auto* treePmForm = new QFormLayout();
    treePmForm->addRow(controls.treePmEnabledCheck.data());
    treePmForm->addRow("Preset", controls.treePmPresetCombo.data());
    treePmLayout->addLayout(treePmForm);

    auto* treePmDetails = new QWidget(page);
    auto* treePmDetailsLayout = new QVBoxLayout(treePmDetails);
    auto* treePmDetailsForm = new QFormLayout();
    treePmDetailsForm->addRow("Model", controls.treePmModelCombo.data());
    treePmDetailsForm->addRow("Particle layout", controls.treePmLayoutCombo.data());
    treePmDetailsForm->addRow("Precision", controls.treePmPrecisionCombo.data());
    treePmDetailsForm->addRow("Assignment", controls.treePmAssignmentCombo.data());
    treePmDetailsForm->addRow(controls.treePmLocalGridCheck.data());
    treePmDetailsForm->addRow("Grid size", controls.treePmGridSizeSpin.data());
    treePmDetailsForm->addRow("Jacobi iterations", controls.treePmJacobiIterationsSpin.data());
    treePmDetailsForm->addRow("Cutoff factor", controls.treePmCutoffFactorSpin.data());
    treePmDetailsForm->addRow("Local neighbors", controls.treePmMaxLocalNeighborsSpin.data());
    treePmDetailsForm->addRow("Particle limit", controls.treePmParticleLimitSpin.data());
    treePmDetailsForm->addRow("Dense cell threshold", controls.treePmDenseCellThresholdSpin.data());
    treePmDetailsForm->addRow(controls.treePmGravityOnlyBuffersCheck.data());
    treePmDetailsLayout->addLayout(treePmDetailsForm);
    treePmDetailsLayout->addStretch(1);

    auto* adaptiveDetails = new QWidget(page);
    auto* adaptiveLayout = new QVBoxLayout(adaptiveDetails);
    auto* adaptiveForm = new QFormLayout();
    adaptiveForm->addRow(controls.adaptiveTimeStepsCheck.data());
    adaptiveForm->addRow("Max level", controls.adaptiveMaxLevelSpin.data());
    adaptiveForm->addRow("Eta", controls.adaptiveEtaSpin.data());
    adaptiveForm->addRow(controls.adaptiveCostGuardCheck.data());
    adaptiveLayout->addLayout(adaptiveForm);
    adaptiveLayout->addStretch(1);

    auto* sphDetails = new QWidget(page);
    auto* sphLayout = new QVBoxLayout(sphDetails);
    auto* sphForm = new QFormLayout();
    sphForm->addRow("Smoothing length", controls.sphSmoothingSpin.data());
    sphForm->addRow("Rest density", controls.sphRestDensitySpin.data());
    sphForm->addRow("Gas constant", controls.sphGasConstantSpin.data());
    sphForm->addRow("Viscosity", controls.sphViscositySpin.data());
    sphLayout->addWidget(controls.sphCheck.data());
    sphLayout->addLayout(sphForm);
    sphLayout->addStretch(1);

    layout->addWidget(coreBox);
    layout->addWidget(treePmBox);
    layout->addWidget(buildDisclosure(page, "Advanced TreePM controls", treePmDetails));
    layout->addWidget(buildDisclosure(page, "Adaptive time stepping", adaptiveDetails));
    layout->addWidget(buildDisclosure(page, "Fluid model (SPH)", sphDetails));
    layout->addStretch(1);
    scroll->setWidget(page);
    return scroll;
}

} // namespace bltzr_qt
