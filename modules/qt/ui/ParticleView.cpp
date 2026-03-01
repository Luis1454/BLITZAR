#include "ui/ParticleView.hpp"
#include "ui/ParticleViewColor.hpp"

#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QSizePolicy>

#include <algorithm>
#include <array>
#include <cmath>

namespace grav_qt {
ParticleView::ParticleView(ViewMode mode)
    : QWidget(nullptr),
      _mode(mode),
      _snapshot(std::nullopt),
      _zoom(8.0f),
      _luminosity(100),
      _camera{0.0f, 0.0f, 0.0f},
      _adaptiveTemperatureScale(2.0f),
      _adaptivePressureScale(2.0f),
      _dragAxis(GimbalAxis::None)
{
    setMinimumSize(220, 180);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void ParticleView::setSnapshot(const std::vector<RenderParticle> &snapshot)
{
    _snapshot = std::cref(snapshot);
    update();
}

void ParticleView::setMode(ViewMode mode)
{
    _mode = mode;
    update();
}

void ParticleView::setZoom(float zoom)
{
    _zoom = std::max(0.1f, zoom);
    update();
}

void ParticleView::setLuminosity(int luminosity)
{
    _luminosity = std::clamp(luminosity, 0, 255);
    update();
}

void ParticleView::setCameraAngles(float yaw, float pitch, float roll)
{
    _camera.yaw = yaw;
    _camera.pitch = pitch;
    _camera.roll = roll;
    update();
}

void ParticleView::mousePressEvent(MouseEventHandle event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    const GimbalOverlay gimbal = computeGimbal(rect(), _mode, _camera);
    if (!gimbal.rect.contains(event->position())) {
        QWidget::mousePressEvent(event);
        return;
    }

    const GimbalAxis axis = pickGimbalAxis(gimbal, event->position());
    if (axis == GimbalAxis::None) {
        QWidget::mousePressEvent(event);
        return;
    }

    _dragAxis = axis;
    _lastMousePos = event->position();
    event->accept();
}

void ParticleView::mouseMoveEvent(MouseEventHandle event)
{
    if (_dragAxis == GimbalAxis::None) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    const QPointF pos = event->position();
    const float dx = static_cast<float>(pos.x() - _lastMousePos.x());
    const float dy = static_cast<float>(pos.y() - _lastMousePos.y());
    const float delta = (dx - dy) * 0.01f;

    if (_dragAxis == GimbalAxis::X) {
        _camera.pitch += delta;
    } else if (_dragAxis == GimbalAxis::Y) {
        _camera.yaw += delta;
    } else if (_dragAxis == GimbalAxis::Z) {
        _camera.roll += delta;
    }
    _lastMousePos = pos;
    update();
    event->accept();
}

void ParticleView::mouseReleaseEvent(MouseEventHandle event)
{
    _dragAxis = GimbalAxis::None;
    QWidget::mouseReleaseEvent(event);
}

void ParticleView::paintEvent(PaintEventHandle event)
{
    (void)event;
    if (_framebuffer.size() != size()) {
        _framebuffer = QImage(size(), QImage::Format_ARGB32);
    }
    _framebuffer.fill(QColor(10, 10, 16));

    const int widthPx = _framebuffer.width();
    const int heightPx = _framebuffer.height();
    const float centerX = width() * 0.5f;
    const float centerY = height() * 0.5f;
    const QRgb heavyColor = heavyBodyColor(_luminosity);

    if (_snapshot.has_value()) {
        const std::vector<RenderParticle> &snapshot = _snapshot->get();
        updateAdaptiveScales(snapshot, _adaptiveTemperatureScale, _adaptivePressureScale);

        for (const RenderParticle &particle : snapshot) {
            const ProjectedPoint pp = projectParticle(particle, _mode, _camera);
            if (!pp.valid) {
                continue;
            }

            const int x = static_cast<int>(centerX + pp.x * _zoom);
            const int y = static_cast<int>(centerY - pp.y * _zoom);
            if (x < 0 || x >= widthPx || y < 0 || y >= heightPx) {
                continue;
            }

            QRgb color = 0;
            if (isHeavyBody(particle)) {
                color = heavyColor;
            } else {
                color = particleRampColorFast(particle, _adaptiveTemperatureScale, _adaptivePressureScale, _luminosity);
            }

            _framebuffer.setPixel(x, y, color);
        }
    }

    QPainter painter(this);
    painter.drawImage(0, 0, _framebuffer);

    const GimbalOverlay gimbal = computeGimbal(rect(), _mode, _camera);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(QColor(120, 120, 135), 1.0));
    painter.setBrush(QColor(18, 18, 26, 180));
    painter.drawEllipse(gimbal.rect);

    const std::array<QColor, 3> colors = {
        QColor(255, 95, 95),
        QColor(85, 255, 125),
        QColor(105, 150, 255)
    };
    for (std::size_t i = 0; i < gimbal.handles.size(); ++i) {
        painter.setPen(QPen(colors[i], 1.6));
        painter.drawLine(gimbal.center, gimbal.handles[i]);
        painter.setBrush(colors[i]);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(gimbal.handles[i], 3.6, 3.6);
    }
    painter.setRenderHint(QPainter::Antialiasing, false);

    painter.setPen(QColor(48, 48, 60));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

} // namespace grav_qt
