#include "ui/QtViewMath.hpp"

namespace grav_qt {

GimbalOverlay computeGimbal(const QRect &bounds, ViewMode mode, const CameraState &camera)
{
    const grav::Rect2D viewport{
        static_cast<float>(bounds.x()),
        static_cast<float>(bounds.y()),
        static_cast<float>(bounds.width()),
        static_cast<float>(bounds.height())
    };
    const grav::GimbalOverlay go = grav::computeGimbal(viewport, mode, camera);

    GimbalOverlay result;
    result.rect = toQRectF(go.bounds);
    result.center = toQPointF(go.center);
    result.radius = go.radius;
    result.handles = {
        toQPointF(go.handles[0]),
        toQPointF(go.handles[1]),
        toQPointF(go.handles[2])
    };
    return result;
}

GimbalAxis pickGimbalAxis(const GimbalOverlay &overlay, const QPointF &mouse)
{
    grav::GimbalOverlay go;
    go.handles = {
        fromQPointF(overlay.handles[0]),
        fromQPointF(overlay.handles[1]),
        fromQPointF(overlay.handles[2])
    };
    return grav::pickGimbalAxis(go, fromQPointF(mouse));
}

} // namespace grav_qt
