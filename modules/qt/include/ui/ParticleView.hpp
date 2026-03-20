#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_PARTICLEVIEW_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_PARTICLEVIEW_HPP_

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

class ParticleView : public QWidget {
    public:
        explicit ParticleView(grav::ViewMode mode);

        void setSnapshot(const std::vector<RenderParticle> &snapshot);
        void setMode(grav::ViewMode mode);
        void setZoom(float zoom);
        void setLuminosity(int luminosity);
        void setCameraAngles(float yaw, float pitch, float roll);
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
