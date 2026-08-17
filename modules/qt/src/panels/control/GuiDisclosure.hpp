/*
 * @file modules/qt/src/panels/control/GuiDisclosure.hpp
 * @brief Collapsible section used for secondary controls.
 */

#ifndef BLITZAR_MODULES_QT_SRC_PANELS_CONTROL_DISCLOSURE_HPP_
#define BLITZAR_MODULES_QT_SRC_PANELS_CONTROL_DISCLOSURE_HPP_

#include <QString>

class QWidget;

namespace bltzr_qt {

QWidget* buildDisclosure(QWidget* parent, const QString& title, QWidget* content);

} // namespace bltzr_qt

#endif // BLITZAR_MODULES_QT_SRC_PANELS_CONTROL_DISCLOSURE_HPP_
