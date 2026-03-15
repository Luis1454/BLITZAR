#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_QTVIEWMATH_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_QTVIEWMATH_HPP_

#include "graphics/ViewMath.hpp"

#include <QPointF>
#include <QRect>
#include <QRectF>

#include <array>
#include <vector>

namespace grav_qt {

// Re-export enums and types from grav namespace for backward compatibility in Qt module
using ViewMode = grav::ViewMode;
using GimbalAxis = grav::GimbalAxis;
using ProjectedPoint = grav::ProjectedPoint;
using CameraState = grav::CameraState;
using grav::projectParticle;

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
inline QPointF toQPointF(const grav::Point2D &p)
{
    return QPointF(static_cast<double>(p.x), static_cast<double>(p.y));
}

inline grav::Point2D fromQPointF(const QPointF &p)
{
    return grav::Point2D{static_cast<float>(p.x()), static_cast<float>(p.y())};
}

inline QRectF toQRectF(const grav::Rect2D &r)
{
    return QRectF(static_cast<double>(r.x), static_cast<double>(r.y), static_cast<double>(r.width), static_cast<double>(r.height));
}

inline grav::Rect2D fromQRectF(const QRectF &r)
{
    return grav::Rect2D{static_cast<float>(r.x()), static_cast<float>(r.y()), static_cast<float>(r.width()), static_cast<float>(r.height())};
}

GimbalOverlay computeGimbal(const QRect &bounds, ViewMode mode, const CameraState &camera);
GimbalAxis pickGimbalAxis(const GimbalOverlay &overlay, const QPointF &mouse);

} // namespace grav_qt

#endif // GRAVITY_MODULES_QT_INCLUDE_UI_QTVIEWMATH_HPP_
