/*
 * @file modules/qt/panels/control/GuiPhysics.hpp
 * @brief Physics sidebar panel builder.
 */
#ifndef BLITZAR_MODULES_QT_SRC_PANELS_CONTROL_PHYSICS_HPP_
#define BLITZAR_MODULES_QT_SRC_PANELS_CONTROL_PHYSICS_HPP_

#include <QWidget>

namespace bltzr_qt {
struct PhysicsControls;
QWidget* buildPhysicsPanel(QWidget* parent, PhysicsControls& controls);

} // namespace bltzr_qt

#endif // BLITZAR_MODULES_QT_SRC_PANELS_CONTROL_PHYSICS_HPP_
