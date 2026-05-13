/*
 * @file modules/qt/src/widgets/overlays/Painter.hpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#ifndef BLITZAR_MODULES_QT_SRC_WIDGETS_OVERLAYS_PAINTER_HPP_
#define BLITZAR_MODULES_QT_SRC_WIDGETS_OVERLAYS_PAINTER_HPP_
/*
 * Module: qt
 * Responsibility: Paint octree overlay cells on top of a particle viewport.
 */
#include "graphics/ViewMath.hpp"
#include "widgets/overlays/Octree.hpp"
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

namespace bltzr_qt {
void paintOctreeOverlay(QPainter& painter, const QRect& viewport, grav::ViewMode mode,
                        const grav::CameraState& camera, const std::vector<OctreeNode>& nodes,
                        float zoom, int opacity);
} // namespace bltzr_qt
#endif // BLITZAR_MODULES_QT_SRC_WIDGETS_OVERLAYS_PAINTER_HPP_
