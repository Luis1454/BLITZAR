/*
 * @file modules/qt/ui/OctreeOverlayPainter.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "ui/OctreeOverlayPainter.hpp"
#include <QColor>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <algorithm>
#include <array>
#include <cmath>

namespace grav_qt {
static QColor colorForDepth(int depth, int opacity)
{
    const int clampedOpacity = std::clamp(opacity, 0, 255);
    const int red = std::clamp(72 + (depth * 28), 0, 255);
    const int green = std::clamp(120 + (depth * 22), 0, 255);
    const int blue = std::clamp(255 - (depth * 16), 80, 255);
    return QColor(red, green, blue, clampedOpacity);
}

static void drawOrthographicNode(QPainter& painter, const QRect& viewport, grav::ViewMode mode,
                                 const OctreeOverlayNode& node, float zoom)
{
    float projectedX = 0.0f;
    float projectedY = 0.0f;
    if (mode == grav::ViewMode::XY) {
        projectedX = node.centerX;
        projectedY = node.centerY;
    }
    else if (mode == grav::ViewMode::XZ) {
        projectedX = node.centerX;
        projectedY = node.centerZ;
    }
    else {
        projectedX = node.centerY;
        projectedY = node.centerZ;
    }
    const float centerX = static_cast<float>(viewport.width()) * 0.5f;
    const float centerY = static_cast<float>(viewport.height()) * 0.5f;
    const float halfSizePx = std::max(node.halfSize * zoom, 0.5f);
    const QRectF box(centerX + (projectedX * zoom) - halfSizePx,
                     centerY - (projectedY * zoom) - halfSizePx, halfSizePx * 2.0f,
                     halfSizePx * 2.0f);
    painter.drawRect(box);
}

static bool projectCorner(const QRect& viewport, grav::ViewMode mode,
                          const grav::CameraState& camera, float zoom, const RenderParticle& corner,
                          QPointF& projected)
{
    const grav::ProjectedPoint point = grav::projectParticle(corner, mode, camera);
    if (!point.valid)
        return false;
    projected.setX(static_cast<float>(viewport.width()) * 0.5f + (point.x * zoom));
    projected.setY(static_cast<float>(viewport.height()) * 0.5f - (point.y * zoom));
    return true;
}

void OctreeOverlayPainter::paint(QPainter& painter, const QRect& viewport, grav::ViewMode mode,
                                 const grav::CameraState& camera,
                                 const std::vector<OctreeOverlayNode>& nodes, float zoom,
                                 int opacity)
{
    if (nodes.empty() || zoom <= 0.0f) {
        return;
    }
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    if (mode == grav::ViewMode::XY || mode == grav::ViewMode::XZ || mode == grav::ViewMode::YZ) {
        for (const OctreeOverlayNode& node : nodes) {
            painter.setPen(QPen(colorForDepth(node.depth, opacity), 1.0));
            drawOrthographicNode(painter, viewport, mode, node, zoom);
        }
        painter.restore();
        return;
    }
    static constexpr std::array<std::array<int, 2>, 12> kCubeEdges = {{{{0, 1}},
                                                                       {{1, 2}},
                                                                       {{2, 3}},
                                                                       {{3, 0}},
                                                                       {{4, 5}},
                                                                       {{5, 6}},
                                                                       {{6, 7}},
                                                                       {{7, 4}},
                                                                       {{0, 4}},
                                                                       {{1, 5}},
                                                                       {{2, 6}},
                                                                       {{3, 7}}}};
    for (const OctreeOverlayNode& node : nodes) {
        const float half = node.halfSize;
        const std::array<RenderParticle, 8> corners = {
            {{node.centerX - half, node.centerY - half, node.centerZ - half, 0.0f, 0.0f, 0.0f},
             {node.centerX + half, node.centerY - half, node.centerZ - half, 0.0f, 0.0f, 0.0f},
             {node.centerX + half, node.centerY + half, node.centerZ - half, 0.0f, 0.0f, 0.0f},
             {node.centerX - half, node.centerY + half, node.centerZ - half, 0.0f, 0.0f, 0.0f},
             {node.centerX - half, node.centerY - half, node.centerZ + half, 0.0f, 0.0f, 0.0f},
             {node.centerX + half, node.centerY - half, node.centerZ + half, 0.0f, 0.0f, 0.0f},
             {node.centerX + half, node.centerY + half, node.centerZ + half, 0.0f, 0.0f, 0.0f},
             {node.centerX - half, node.centerY + half, node.centerZ + half, 0.0f, 0.0f, 0.0f}}};
        std::array<QPointF, 8> projectedCorners{};
        bool allProjected = true;
        for (std::size_t cornerIndex = 0; cornerIndex < corners.size(); ++cornerIndex) {
            if (!projectCorner(viewport, mode, camera, zoom, corners[cornerIndex],
                               projectedCorners[cornerIndex])) {
                allProjected = false;
                break;
            }
        }
        if (!allProjected) {
            continue;
        }
        painter.setPen(QPen(colorForDepth(node.depth, opacity), 1.0));
        for (const std::array<int, 2>& edge : kCubeEdges) {
            painter.drawLine(projectedCorners[static_cast<std::size_t>(edge[0])],
                             projectedCorners[static_cast<std::size_t>(edge[1])]);
        }
    }
    painter.restore();
}
} // namespace grav_qt
