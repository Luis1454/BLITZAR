#ifndef GRAVITY_ENGINE_INCLUDE_GRAPHICS_VIEWMATH_HPP_
#define GRAVITY_ENGINE_INCLUDE_GRAPHICS_VIEWMATH_HPP_

#include "graphics/GraphicsTypes.hpp"
#include "types/SimulationTypes.hpp"

#include <array>

namespace grav {

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

struct GimbalOverlay {
    Rect2D bounds;
    Point2D center;
    float radius;
    std::array<Point2D, 3> handles;
};

ProjectedPoint projectParticle(const RenderParticle &particle, ViewMode mode, const CameraState &camera);
GimbalOverlay computeGimbal(const Rect2D &viewport, ViewMode mode, const CameraState &camera);
GimbalAxis pickGimbalAxis(const GimbalOverlay &overlay, const Point2D &mouse);

} // namespace grav

#endif // GRAVITY_ENGINE_INCLUDE_GRAPHICS_VIEWMATH_HPP_
