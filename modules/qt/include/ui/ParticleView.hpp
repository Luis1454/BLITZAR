#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_PARTICLEVIEW_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_PARTICLEVIEW_HPP_

/*
 * Module: ui
 * Responsibility: Render a single particle projection or 3D view inside the Qt workspace.
 */

#include "ui/QtViewMath.hpp"

#include <QImage>
#include <QPointF>
#include <QWidget>

#include <functional>
#include <optional>
#include <vector>

class QMouseEvent;
class QPaintEvent;

typedef QMouseEvent UiMouseEvent;
typedef QPaintEvent UiPaintEvent;
namespace grav_qt {

/// Renders one projection of the current particle snapshot.
class ParticleView : public QWidget {
    public:
        /// Builds a view configured for the requested projection mode.
        explicit ParticleView(grav::ViewMode mode);

        /// Replaces the snapshot rendered by this view.
        void setSnapshot(const std::vector<RenderParticle> &snapshot);
        /// Changes the projection mode used when painting.
        void setMode(grav::ViewMode mode);
        /// Sets the magnification applied during painting.
        void setZoom(float zoom);
        /// Sets the luminosity bias applied to particle colors.
        void setLuminosity(int luminosity);
        /// Updates the camera angles used by the 3D projection.
        void setCameraAngles(float yaw, float pitch, float roll);
        /// Updates culling and level-of-detail settings.
        void setRenderSettings(bool culling, bool lod, float nearDist, float farDist);

    private:
        typedef UiMouseEvent * MouseEventHandle;
        typedef UiPaintEvent * PaintEventHandle;
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
        float _lodNearDistance = 10.0f;
        float _lodFarDistance = 60.0f;
};

} // namespace grav_qt


#endif // GRAVITY_MODULES_QT_INCLUDE_UI_PARTICLEVIEW_HPP_
