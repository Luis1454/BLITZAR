// File: engine/include/graphics/ViewMath.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_GRAPHICS_VIEWMATH_HPP_
#define GRAVITY_ENGINE_INCLUDE_GRAPHICS_VIEWMATH_HPP_
#include "graphics/GraphicsTypes.hpp"
#include "types/SimulationTypes.hpp"
#include <array>

namespace grav {
/// Description: Enumerates the supported ViewMode values.
enum class ViewMode {
    XY,
    XZ,
    YZ,
    Iso,
    Perspective
};
/// Description: Enumerates the supported GimbalAxis values.
enum class GimbalAxis {
    None,
    X,
    Y,
    Z
};

/// Description: Defines the ProjectedPoint data or behavior contract.
struct ProjectedPoint {
    float x;
    float y;
    float depth;
    bool valid;
};

/// Description: Defines the CameraState data or behavior contract.
struct CameraState {
    float yaw;
    float pitch;
    float roll;
};

/// Description: Defines the GimbalOverlay data or behavior contract.
struct GimbalOverlay {
    Rect2D bounds;
    Point2D center;
    float radius;
    std::array<Point2D, 3> handles;
};

/// Description: Describes the project particle operation contract.
ProjectedPoint projectParticle(const RenderParticle& particle, ViewMode mode,
                               const CameraState& camera);
/// Description: Executes the computeGimbal operation.
GimbalOverlay computeGimbal(const Rect2D& viewport, ViewMode mode, const CameraState& camera);
/// Description: Executes the pickGimbalAxis operation.
GimbalAxis pickGimbalAxis(const GimbalOverlay& overlay, const Point2D& mouse);
} // namespace grav
#endif // GRAVITY_ENGINE_INCLUDE_GRAPHICS_VIEWMATH_HPP_
