/*
 * @file modules/qt/ui/QtViewMath.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "ui/QtViewMath.hpp"

namespace bltzr_qt {
QPointF toQPointF(const grav::Point2D& p)
{
    return QPointF(static_cast<double>(p.x), static_cast<double>(p.y));
}

grav::Point2D fromQPointF(const QPointF& p)
{
    return grav::Point2D{static_cast<float>(p.x()), static_cast<float>(p.y())};
}

QRectF toQRectF(const grav::Rect2D& r)
{
    return QRectF(static_cast<double>(r.x), static_cast<double>(r.y), static_cast<double>(r.width),
                  static_cast<double>(r.height));
}

grav::Rect2D fromQRectF(const QRectF& r)
{
    return grav::Rect2D{static_cast<float>(r.x()), static_cast<float>(r.y()),
                        static_cast<float>(r.width()), static_cast<float>(r.height())};
}

GimbalOverlay computeGimbal(const QRect& bounds, grav::ViewMode mode,
                            const grav::CameraState& camera)
{
    const grav::Rect2D viewport{static_cast<float>(bounds.x()), static_cast<float>(bounds.y()),
                                static_cast<float>(bounds.width()),
                                static_cast<float>(bounds.height())};
    const grav::GimbalOverlay go = grav::computeGimbal(viewport, mode, camera);
    GimbalOverlay result;
    result.rect = toQRectF(go.bounds);
    result.center = toQPointF(go.center);
    result.radius = go.radius;
    result.handles = {toQPointF(go.handles[0]), toQPointF(go.handles[1]), toQPointF(go.handles[2])};
    return result;
}

grav::GimbalAxis pickGimbalAxis(const GimbalOverlay& overlay, const QPointF& mouse)
{
    grav::GimbalOverlay go;
    go.handles = {fromQPointF(overlay.handles[0]), fromQPointF(overlay.handles[1]),
                  fromQPointF(overlay.handles[2])};
    return grav::pickGimbalAxis(go, fromQPointF(mouse));
}
} // namespace bltzr_qt
