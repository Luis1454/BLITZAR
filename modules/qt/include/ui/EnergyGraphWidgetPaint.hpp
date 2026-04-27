/*
 * @file modules/qt/include/ui/EnergyGraphWidgetPaint.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_ENERGYGRAPHWIDGETPAINT_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_ENERGYGRAPHWIDGETPAINT_HPP_
/*
 * Module: ui
 * Responsibility: Paint the energy timeline widget using the current telemetry
 * history.
 */
#include "ui/QtViewMath.hpp"
#include <QWidget>
#include <vector>
/*
 * @brief Defines the qpaint event type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class QPaintEvent;
typedef QPaintEvent UiPaintEvent;

namespace grav_qt {
class EnergyGraphWidgetPaint final {
public:
    static void paint(QWidget& widget, const std::vector<EnergyPoint>& history,
                      UiPaintEvent* event);
};
} // namespace grav_qt
#endif // GRAVITY_MODULES_QT_INCLUDE_UI_ENERGYGRAPHWIDGETPAINT_HPP_
