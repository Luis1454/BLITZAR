#ifndef GRAVITY_UI_QTVIEWMATH_H
#define GRAVITY_UI_QTVIEWMATH_H

#include "backend/SimulationBackend.hpp"

#include <QPointF>
#include <QRect>
#include <QRectF>

#include <array>

namespace grav_qt {

enum class ViewMode {
    XY,
    XZ,
    YZ,
    Iso,
    Perspective
};

enum class GimbalAxis {
    None,
    X,
    Y,
    Z
};

struct ProjectedPoint {
    float x;
    float y;
    float depth;
    bool valid;
};

struct CameraState {
    float yaw;
    float pitch;
    float roll;
};

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

ProjectedPoint projectParticle(const RenderParticle &particle, ViewMode mode, const CameraState &camera);
GimbalOverlay computeGimbal(const QRect &bounds, ViewMode mode, const CameraState &camera);
GimbalAxis pickGimbalAxis(const GimbalOverlay &overlay, const QPointF &mouse);

} // namespace grav_qt

#endif // GRAVITY_UI_QTVIEWMATH_H

