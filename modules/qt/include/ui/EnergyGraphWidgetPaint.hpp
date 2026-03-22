#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_ENERGYGRAPHWIDGETPAINT_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_ENERGYGRAPHWIDGETPAINT_HPP_

/*
 * Module: ui
 * Responsibility: Paint the energy timeline widget using the current telemetry history.
 */

#include "ui/QtViewMath.hpp"

#include <QWidget>

#include <vector>

class QPaintEvent;

typedef QPaintEvent UiPaintEvent;

namespace grav_qt {

class EnergyGraphWidgetPaint final {
    public:
        static void paint(QWidget &widget, const std::vector<EnergyPoint> &history, UiPaintEvent *event);
};

} // namespace grav_qt

#endif // GRAVITY_MODULES_QT_INCLUDE_UI_ENERGYGRAPHWIDGETPAINT_HPP_
