#include "graphics/ViewMath.hpp"
#include <algorithm>
#include <cmath>
namespace grav {
constexpr float kIsoYaw = 0.78539816339f;
constexpr float kIsoPitch = 0.61547970867f;
static void rotate3D(float x, float y, float z, float yaw, float pitch, float roll, float& outX,
                     float& outY, float& outZ)
{
    const float cy = std::cos(yaw);
    const float sy = std::sin(yaw);
    const float cp = std::cos(pitch);
    const float sp = std::sin(pitch);
    const float cr = std::cos(roll);
    const float sr = std::sin(roll);
    const float x1 = cy * x - sy * z;
    const float z1 = sy * x + cy * z;
    const float y1 = cp * y - sp * z1;
    const float z2 = sp * y + cp * z1;
    outX = cr * x1 - sr * y1;
    outY = sr * x1 + cr * y1;
    outZ = z2;
}
static void baseOrientation(ViewMode mode, float& yaw, float& pitch, float& roll)
{
    yaw = 0.0f;
    pitch = 0.0f;
    roll = 0.0f;
    if (mode == ViewMode::Iso || mode == ViewMode::Perspective)
        yaw = kIsoYaw;
    pitch = kIsoPitch;
}
static void modeComponents(ViewMode mode, float x, float y, float z, float& sx, float& sy,
                           float& depth)
{
    switch (mode) {
    case ViewMode::XY:
        sx = x;
        sy = y;
        depth = z;
        break;
    case ViewMode::XZ:
        sx = x;
        sy = z;
        depth = y;
        break;
    case ViewMode::YZ:
        sx = y;
        sy = z;
        depth = x;
        break;
    case ViewMode::Iso:
    case ViewMode::Perspective:
    default:
        sx = x;
        sy = y;
        depth = z;
        break;
    }
}
static float distanceSquared(const Point2D& a, const Point2D& b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return dx * dx + dy * dy;
}
ProjectedPoint projectParticle(const RenderParticle& particle, ViewMode mode,
                               const CameraState& camera)
{
    float baseYaw = 0.0f;
    float basePitch = 0.0f;
    float baseRoll = 0.0f;
    baseOrientation(mode, baseYaw, basePitch, baseRoll);
    float rx = 0.0f;
    float ry = 0.0f;
    float rz = 0.0f;
    rotate3D(particle.x, particle.y, particle.z, baseYaw + camera.yaw, basePitch + camera.pitch,
             baseRoll + camera.roll, rx, ry, rz);
    float sx = 0.0f;
    float sy = 0.0f;
    float depth = 0.0f;
    modeComponents(mode, rx, ry, rz, sx, sy, depth);
    if (mode != ViewMode::Perspective)
        return ProjectedPoint{sx, sy, depth, true};
    const float cameraDistance = 40.0f;
    const float denom = cameraDistance - depth;
    if (std::fabs(denom) < 1e-3f)
        return ProjectedPoint{0.0f, 0.0f, depth, false};
    const float perspectiveScale = cameraDistance / denom;
    if (perspectiveScale <= 0.0f || perspectiveScale > 30.0f)
        return ProjectedPoint{0.0f, 0.0f, depth, false};
    return ProjectedPoint{sx * perspectiveScale, sy * perspectiveScale, depth, true};
}
GimbalOverlay computeGimbal(const Rect2D& viewport, ViewMode mode, const CameraState& camera)
{
    const float size = std::clamp(std::min(viewport.width, viewport.height) * 0.26f, 54.0f, 94.0f);
    const float margin = 8.0f;
    const Rect2D bounds{viewport.width - size - margin, margin, size, size};
    const Point2D center{bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.5f};
    const float radius = bounds.width * 0.45f;
    float baseYaw = 0.0f;
    float basePitch = 0.0f;
    float baseRoll = 0.0f;
    baseOrientation(mode, baseYaw, basePitch, baseRoll);
    std::array<Point2D, 3> handles{};
    const std::array<std::array<float, 3>, 3> axes = {std::array<float, 3>{1.0f, 0.0f, 0.0f},
                                                      std::array<float, 3>{0.0f, 1.0f, 0.0f},
                                                      std::array<float, 3>{0.0f, 0.0f, 1.0f}};
    for (std::size_t i = 0; i < axes.size(); ++i) {
        float rx = 0.0f;
        float ry = 0.0f;
        float rz = 0.0f;
        rotate3D(axes[i][0], axes[i][1], axes[i][2], baseYaw + camera.yaw, basePitch + camera.pitch,
                 baseRoll + camera.roll, rx, ry, rz);
        float sx = 0.0f;
        float sy = 0.0f;
        float depth = 0.0f;
        modeComponents(mode, rx, ry, rz, sx, sy, depth);
        (void)depth;
        handles[i] = Point2D{center.x + sx * radius * 0.8f, center.y - sy * radius * 0.8f};
    }
    return GimbalOverlay{bounds, center, radius, handles};
}
GimbalAxis pickGimbalAxis(const GimbalOverlay& overlay, const Point2D& mouse)
{
    const float threshold2 = 11.0f * 11.0f;
    if (distanceSquared(mouse, overlay.handles[0]) <= threshold2) {
        return GimbalAxis::X;
    }
    if (distanceSquared(mouse, overlay.handles[1]) <= threshold2) {
        return GimbalAxis::Y;
    }
    if (distanceSquared(mouse, overlay.handles[2]) <= threshold2) {
        return GimbalAxis::Z;
    }
    return GimbalAxis::None;
}
} // namespace grav
