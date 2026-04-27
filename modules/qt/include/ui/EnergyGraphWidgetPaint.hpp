// File: modules/qt/include/ui/EnergyGraphWidgetPaint.hpp
// Purpose: Client module implementation for BLITZAR extension workflows.

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
/// Description: Defines the QPaintEvent data or behavior contract.
class QPaintEvent;
typedef QPaintEvent UiPaintEvent;

namespace grav_qt {
/// Description: Defines the EnergyGraphWidgetPaint data or behavior contract.
class EnergyGraphWidgetPaint final {
public:
    /// Description: Describes the paint operation contract.
    static void paint(QWidget& widget, const std::vector<EnergyPoint>& history,
                      UiPaintEvent* event);
};
} // namespace grav_qt
#endif // GRAVITY_MODULES_QT_INCLUDE_UI_ENERGYGRAPHWIDGETPAINT_HPP_
