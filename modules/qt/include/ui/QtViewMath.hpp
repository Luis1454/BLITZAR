// File: modules/qt/include/ui/QtViewMath.hpp
// Purpose: Client module implementation for BLITZAR extension workflows.

#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_QTVIEWMATH_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_QTVIEWMATH_HPP_
/*
 * Module: ui
 * Responsibility: Provide Qt-specific geometric helpers shared by particle and
 * energy widgets.
 */
#include "graphics/ViewMath.hpp"
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <array>
#include <vector>
namespace grav_qt {
/// Stores one energy-history sample for the Qt energy graph.
struct EnergyPoint {
    float kinetic;
    float potential;
    float thermal;
    float radiated;
    float total;
    float drift;
    float time;
};
/// Describes the geometry of the interactive 3D camera gimbal overlay.
struct GimbalOverlay {
    QRectF rect;
    QPointF center;
    float radius;
    std::array<QPointF, 3> handles;
};
/// Converts a generic 2D point into a Qt point.
QPointF toQPointF(const grav::Point2D& p);
/// Converts a Qt point into the generic 2D point representation.
grav::Point2D fromQPointF(const QPointF& p);
/// Converts a generic rectangle into a Qt floating rectangle.
QRectF toQRectF(const grav::Rect2D& r);
/// Converts a Qt floating rectangle into the generic rectangle representation.
grav::Rect2D fromQRectF(const QRectF& r);
/// Computes the screen-space overlay used to draw the 3D camera gimbal.
GimbalOverlay computeGimbal(const QRect& bounds, grav::ViewMode mode,
                            const grav::CameraState& camera);
/// Picks the manipulated gimbal axis from a mouse position.
grav::GimbalAxis pickGimbalAxis(const GimbalOverlay& overlay, const QPointF& mouse);
} // namespace grav_qt
#endif // GRAVITY_MODULES_QT_INCLUDE_UI_QTVIEWMATH_HPP_
