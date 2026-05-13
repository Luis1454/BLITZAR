/*
 * @file modules/qt/src/panels/control/Physics.hpp
 * @brief Physics sidebar panel builder.
 */
#ifndef BLITZAR_MODULES_QT_SRC_PANELS_CONTROL_PHYSICS_HPP_
#define BLITZAR_MODULES_QT_SRC_PANELS_CONTROL_PHYSICS_HPP_

#include <QWidget>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;

namespace bltzr_qt {
QWidget* buildPhysicsPanel(QWidget* parent, QComboBox* solverCombo, QComboBox* integratorCombo,
                           QDoubleSpinBox* dtSpin, QDoubleSpinBox* thetaSpin,
                           QDoubleSpinBox* softeningSpin, QCheckBox* sphCheck,
                           QDoubleSpinBox* sphSmoothingSpin, QDoubleSpinBox* sphRestDensitySpin,
                           QDoubleSpinBox* sphGasConstantSpin, QDoubleSpinBox* sphViscositySpin);

} // namespace bltzr_qt

#endif // BLITZAR_MODULES_QT_SRC_PANELS_CONTROL_PHYSICS_HPP_
