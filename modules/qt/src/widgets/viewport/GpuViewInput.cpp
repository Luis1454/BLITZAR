/*
 * @file modules/qt/src/widgets/viewport/GpuViewInput.cpp
 * @brief Gimbal mouse interaction for the OpenGL point-sprite viewport.
 */

#include "widgets/overlays/Painter.hpp"
#include "widgets/viewport/GpuView.hpp"
#include <QMouseEvent>

namespace bltzr_qt {

void GpuView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QOpenGLWidget::mousePressEvent(event);
        return;
    }
    const GimbalOverlay gimbal = computeGimbal(rect(), _mode, _camera);
    if (!gimbal.rect.contains(event->position())) {
        QOpenGLWidget::mousePressEvent(event);
        return;
    }
    _dragAxis = pickGimbalAxis(gimbal, event->position());
    if (_dragAxis == grav::GimbalAxis::None) {
        QOpenGLWidget::mousePressEvent(event);
        return;
    }
    _lastMousePos = event->position();
    event->accept();
}

void GpuView::mouseMoveEvent(QMouseEvent* event)
{
    if (_dragAxis == grav::GimbalAxis::None) {
        QOpenGLWidget::mouseMoveEvent(event);
        return;
    }
    const QPointF position = event->position();
    const float dx = static_cast<float>(position.x() - _lastMousePos.x());
    const float dy = static_cast<float>(position.y() - _lastMousePos.y());
    if (_dragAxis == grav::GimbalAxis::X) {
        _camera.yaw += dx * 0.5f;
    }
    else if (_dragAxis == grav::GimbalAxis::Y) {
        _camera.pitch -= dy * 0.5f;
    }
    else {
        _camera.roll += dx * 0.5f;
    }
    _lastMousePos = position;
    update();
    event->accept();
}

void GpuView::mouseReleaseEvent(QMouseEvent* event)
{
    if (_dragAxis != grav::GimbalAxis::None) {
        _dragAxis = grav::GimbalAxis::None;
        event->accept();
        return;
    }
    QOpenGLWidget::mouseReleaseEvent(event);
}

} // namespace bltzr_qt
