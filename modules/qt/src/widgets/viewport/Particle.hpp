/*
 * @file modules/qt/src/widgets/viewport/Particle.hpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#ifndef BLITZAR_MODULES_QT_SRC_WIDGETS_VIEWPORT_PARTICLE_HPP_
#define BLITZAR_MODULES_QT_SRC_WIDGETS_VIEWPORT_PARTICLE_HPP_
/*
 * Module: qt
 * Responsibility: Render a single particle projection or 3D view inside the Qt
 * workspace.
 */
#include "Constants.hpp"
#include "widgets/overlays/Octree.hpp"
#include "support/geometry/ViewMath.hpp"
#include <QImage>
#include <QPointF>
#include <QWidget>
#include <functional>
#include <optional>
#include <vector>
/*
 * @brief Defines the qmouse event type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class QMouseEvent;
/*
 * @brief Defines the qpaint event type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class QPaintEvent;
typedef QMouseEvent UiMouseEvent;
typedef QPaintEvent UiPaintEvent;

namespace bltzr_qt {
class Particle : public QWidget {
public:
    explicit Particle(grav::ViewMode mode);
    void setSnapshot(const std::vector<RenderParticle>& snapshot);
    void setMode(grav::ViewMode mode);
    void setZoom(float zoom);
    void setLuminosity(int luminosity);
    void setCameraAngles(float yaw, float pitch, float roll);
    void setRenderSettings(bool culling, bool lod, float nearDist, float farDist);
    void setOctreeOverlay(const std::vector<OctreeNode>& overlay, bool enabled, int opacity);

private:
    typedef UiMouseEvent* MouseEventHandle;
    typedef UiPaintEvent* PaintEventHandle;
    void mousePressEvent(MouseEventHandle event) override;
    void mouseMoveEvent(MouseEventHandle event) override;
    void paintEvent(PaintEventHandle event) override;
    void mouseReleaseEvent(MouseEventHandle event) override;
    grav::ViewMode _mode;
    std::optional<std::reference_wrapper<const std::vector<RenderParticle>>> _snapshot;
    QImage _framebuffer;
    float _zoom;
    int _luminosity;
    grav::CameraState _camera;
    float _adaptiveTemperatureScale;
    float _adaptivePressureScale;
    grav::GimbalAxis _dragAxis;
    QPointF _lastMousePos;
    bool _cullingEnabled = true;
    bool _lodEnabled = true;
    float _lodNearDistance = kRenderLODNearDistance;
    float _lodFarDistance = kRenderLODFarDistance;
    std::optional<std::reference_wrapper<const std::vector<OctreeNode>>> _octreeOverlay;
    bool _octreeOverlayEnabled = false;
    int _octreeOverlayOpacity = kOverlayOpacityDefault;
};
} // namespace bltzr_qt
#endif // BLITZAR_MODULES_QT_SRC_WIDGETS_VIEWPORT_PARTICLE_HPP_
