#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_QTVIEWMATH_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_QTVIEWMATH_HPP_

#include "graphics/ViewMath.hpp"

#include <QPointF>
#include <QRect>
#include <QRectF>

#include <array>
#include <vector>

namespace grav_qt {

struct EnergyPoint {
    float kinetic;
    float potential;
    float thermal;
    float radiated;
    float total;
    float drift;
};

struct GimbalOverlay {
    QRectF rect;
    QPointF center;
    float radius;
    std::array<QPointF, 3> handles;
};

// Conversion helpers
QPointF toQPointF(const grav::Point2D &p);
grav::Point2D fromQPointF(const QPointF &p);
QRectF toQRectF(const grav::Rect2D &r);
grav::Rect2D fromQRectF(const QRectF &r);

GimbalOverlay computeGimbal(const QRect &bounds, grav::ViewMode mode, const grav::CameraState &camera);
grav::GimbalAxis pickGimbalAxis(const GimbalOverlay &overlay, const QPointF &mouse);

} // namespace grav_qt

#endif // GRAVITY_MODULES_QT_INCLUDE_UI_QTVIEWMATH_HPP_
