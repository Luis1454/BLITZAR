/*
 * @file modules/qt/include/ui/OctreeOverlayPainter.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_OCTREEOVERLAYPAINTER_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_OCTREEOVERLAYPAINTER_HPP_
/*
 * Module: ui
 * Responsibility: Paint octree overlay cells on top of a particle viewport.
 */
#include "graphics/ViewMath.hpp"
#include "ui/OctreeOverlay.hpp"
/*
 * @brief Defines the qpainter type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class QPainter;
/*
 * @brief Defines the qrect type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class QRect;

namespace grav_qt {
class OctreeOverlayPainter {
public:
    static void paint(QPainter& painter, const QRect& viewport, grav::ViewMode mode,
                      const grav::CameraState& camera, const std::vector<OctreeOverlayNode>& nodes,
                      float zoom, int opacity);
};
} // namespace grav_qt
#endif // GRAVITY_MODULES_QT_INCLUDE_UI_OCTREEOVERLAYPAINTER_HPP_
