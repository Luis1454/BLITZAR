#ifndef GRAVITY_UI_PARTICLEVIEW_H
#define GRAVITY_UI_PARTICLEVIEW_H

#include "ui/QtViewMath.hpp"

#include <QImage>
#include <QPointF>
#include <QWidget>

#include <vector>

class QMouseEvent;
class QPaintEvent;

namespace qtui {

class ParticleView : public QWidget {
    public:
        explicit ParticleView(ViewMode mode, QWidget *parent = nullptr);

        void setSnapshot(const std::vector<RenderParticle> *snapshot);
        void setMode(ViewMode mode);
        void setZoom(float zoom);
        void setLuminosity(int luminosity);
        void setCameraAngles(float yaw, float pitch, float roll);

    protected:
        void mousePressEvent(QMouseEvent *event) override;
        void mouseMoveEvent(QMouseEvent *event) override;
        void mouseReleaseEvent(QMouseEvent *event) override;
        void paintEvent(QPaintEvent *event) override;

    private:
        ViewMode _mode;
        const std::vector<RenderParticle> *_snapshot;
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
