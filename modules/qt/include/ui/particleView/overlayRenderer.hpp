/*
 * @file modules/qt/include/ui/particleView/overlayRenderer.hpp
 * @brief Gimbal overlay and octree visualization rendering.
 */

#ifndef BLTZAR_QT_PARTICLEVIEW_OVERLAYRENDERER_HPP
#define BLTZAR_QT_PARTICLEVIEW_OVERLAYRENDERER_HPP

class QPainter;
class ParticleView;

namespace bltzr_qt::particleView {

/**
 * @brief Render gimbal overlay and octree visualization.
 */
class OverlayRenderer final {
public:
    /**
     * @brief Draw gimbal circle, axis handles, and labels.
     * @param view Source particle view
     * @param painter Qt painter for drawing
     */
    static void drawGimbalOverlay(const ParticleView* view, QPainter* painter);

    /**
     * @brief Draw octree node boxes and node count.
     * @param view Source particle view
     * @param painter Qt painter for drawing
     */
    static void drawOctreeOverlay(const ParticleView* view, QPainter* painter);

    /**
     * @brief Draw viewport border and statistics.
     * @param view Source particle view
     * @param painter Qt painter for drawing
     */
    static void drawBorder(const ParticleView* view, QPainter* painter);
};

}  // namespace bltzr_qt::particleView

#endif  // BLTZAR_QT_PARTICLEVIEW_OVERLAYRENDERER_HPP
