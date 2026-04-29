/*
 * @file modules/qt/ui/panels/PhysicsControlPanel.cpp
 * @brief Implementation of the physics sidebar panel.
 */

#include "ui/panels/PhysicsControlPanel.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QVBoxLayout>

namespace bltzr_qt {

QWidget* PhysicsControlPanel::build(QWidget* parent, QComboBox* solverCombo,
                                    QComboBox* integratorCombo, QDoubleSpinBox* dtSpin,
                                    QDoubleSpinBox* thetaSpin, QDoubleSpinBox* softeningSpin,
                                    QCheckBox* sphCheck, QDoubleSpinBox* sphSmoothingSpin,
                                    QDoubleSpinBox* sphRestDensitySpin,
                                    QDoubleSpinBox* sphGasConstantSpin,
                                    QDoubleSpinBox* sphViscositySpin)
{
    auto* page = new QWidget(parent);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(8);

    auto* coreBox = new QGroupBox("Physics Core", page);
    auto* coreLayout = new QVBoxLayout(coreBox);
    auto* coreForm = new QFormLayout();
    coreForm->addRow("solver", solverCombo);
    coreForm->addRow("integrator", integratorCombo);
    coreForm->addRow("dt", dtSpin);
    coreForm->addRow("theta", thetaSpin);
    coreForm->addRow("softening", softeningSpin);
    coreLayout->addLayout(coreForm);

    auto* sphBox = new QGroupBox("SPH", page);
    auto* sphLayout = new QVBoxLayout(sphBox);
    auto* sphForm = new QFormLayout();
    sphForm->addRow("h", sphSmoothingSpin);
    sphForm->addRow("rest density", sphRestDensitySpin);
    sphForm->addRow("gas K", sphGasConstantSpin);
    sphForm->addRow("viscosity", sphViscositySpin);
    sphLayout->addWidget(sphCheck);
    sphLayout->addLayout(sphForm);

    layout->addWidget(coreBox);
    layout->addWidget(sphBox);
    layout->addStretch(1);
    return page;
}

} // namespace bltzr_qt
