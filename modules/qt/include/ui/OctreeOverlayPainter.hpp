// File: modules/qt/include/ui/OctreeOverlayPainter.hpp
// Purpose: Client module implementation for BLITZAR extension workflows.

#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_OCTREEOVERLAYPAINTER_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_OCTREEOVERLAYPAINTER_HPP_
/*
 * Module: ui
 * Responsibility: Paint octree overlay cells on top of a particle viewport.
 */
#include "graphics/ViewMath.hpp"
#include "ui/OctreeOverlay.hpp"
/// Description: Defines the QPainter data or behavior contract.
class QPainter;
/// Description: Defines the QRect data or behavior contract.
class QRect;

namespace grav_qt {
/// Description: Defines the OctreeOverlayPainter data or behavior contract.
class OctreeOverlayPainter {
public:
    /// Description: Describes the paint operation contract.
    static void paint(QPainter& painter, const QRect& viewport, grav::ViewMode mode,
                      const grav::CameraState& camera, const std::vector<OctreeOverlayNode>& nodes,
                      float zoom, int opacity);
};
} // namespace grav_qt
#endif // GRAVITY_MODULES_QT_INCLUDE_UI_OCTREEOVERLAYPAINTER_HPP_
