/*
 * @file modules/qt/include/ui/panels/PhysicsControlPanel.hpp
 * @brief Physics sidebar panel builder.
 */
#ifndef BLITZAR_MODULES_QT_INCLUDE_UI_PANELS_PHYSICSCONTROLPANEL_HPP_
#define BLITZAR_MODULES_QT_INCLUDE_UI_PANELS_PHYSICSCONTROLPANEL_HPP_

#include <QWidget>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;

namespace bltzr_qt {

class PhysicsControlPanel {
public:
    static QWidget* build(QWidget* parent, QComboBox* solverCombo, QComboBox* integratorCombo,
                          QDoubleSpinBox* dtSpin, QDoubleSpinBox* thetaSpin,
                          QDoubleSpinBox* softeningSpin, QCheckBox* sphCheck,
                          QDoubleSpinBox* sphSmoothingSpin, QDoubleSpinBox* sphRestDensitySpin,
                          QDoubleSpinBox* sphGasConstantSpin, QDoubleSpinBox* sphViscositySpin);
};

} // namespace bltzr_qt

#endif // BLITZAR_MODULES_QT_INCLUDE_UI_PANELS_PHYSICSCONTROLPANEL_HPP_
