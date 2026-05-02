/*
 * @file modules/qt/include/ui/particleView/gimbalController.hpp
 * @brief Gimbal rotation control for mouse interaction.
 */

#ifndef BLTZAR_QT_PARTICLEVIEW_GIMBALCONTROLLER_HPP
#define BLTZAR_QT_PARTICLEVIEW_GIMBALCONTROLLER_HPP

class QMouseEvent;
class ParticleView;

namespace bltzr_qt {
enum class GimbalAxis;
}

namespace bltzr_qt::particleView {

/**
 * @brief Handle mouse-based gimbal rotation control.
 */
class GimbalController final {
public:
    /**
     * @brief Process mouse press event to start rotation.
     * @param view Target particle view
     * @param event Qt mouse event
     */
    static void handleMousePress(ParticleView* view, QMouseEvent* event);

    /**
     * @brief Process mouse move event to update rotation.
     * @param view Target particle view
     * @param event Qt mouse event
     */
    static void handleMouseMove(ParticleView* view, QMouseEvent* event);

    /**
     * @brief Process mouse release event to end rotation.
     * @param view Target particle view
     * @param event Qt mouse event
     */
    static void handleMouseRelease(ParticleView* view, QMouseEvent* event);

    /**
     * @brief Determine gimbal axis from point in view.
     * @param view Target particle view
     * @param x Screen X coordinate
     * @param y Screen Y coordinate
     * @return Gimbal axis (X, Y, Z, or None)
     */
    static bltzr_qt::GimbalAxis pickAxis(ParticleView* view, int x, int y);
};

}  // namespace bltzr_qt::particleView

#endif  // BLTZAR_QT_PARTICLEVIEW_GIMBALCONTROLLER_HPP
