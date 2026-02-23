#ifndef GRAVITY_UI_PARTICLEVIEW_H
#define GRAVITY_UI_PARTICLEVIEW_H

#include "ui/QtViewMath.hpp"

#include <QImage>
#include <QPointF>
#include <QWidget>

#include <functional>
#include <optional>
#include <vector>

class QMouseEvent;
class QPaintEvent;

using UiMouseEvent = QMouseEvent;
using UiPaintEvent = QPaintEvent;

namespace qtui {

class ParticleView : public QWidget {
    public:
        explicit ParticleView(ViewMode mode);

        void setSnapshot(const std::vector<RenderParticle> &snapshot);
        void setMode(ViewMode mode);
        void setZoom(float zoom);
        void setLuminosity(int luminosity);
        void setCameraAngles(float yaw, float pitch, float roll);

    private:
        using MouseEventHandle = UiMouseEvent *;
        using PaintEventHandle = UiPaintEvent *;
        void mousePressEvent(MouseEventHandle event) override;
        void mouseMoveEvent(MouseEventHandle event) override;
        void mouseReleaseEvent(MouseEventHandle event) override;
        void paintEvent(PaintEventHandle event) override;
        ViewMode _mode;
        std::optional<std::reference_wrapper<const std::vector<RenderParticle>>> _snapshot;
        QImage _framebuffer;
        float _zoom;
        int _luminosity;
        CameraState _camera;
        float _adaptiveTemperatureScale;
        float _adaptivePressureScale;
        GimbalAxis _dragAxis;
        QPointF _lastMousePos;
};

} // namespace qtui

#endif // GRAVITY_UI_PARTICLEVIEW_H
