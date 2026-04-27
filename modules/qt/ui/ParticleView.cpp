// File: modules/qt/ui/ParticleView.cpp
// Purpose: Client module implementation for BLITZAR extension workflows.

#include "ui/ParticleView.hpp"
#include "ui/OctreeOverlay.hpp"
#include "ui/OctreeOverlayPainter.hpp"
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
/// Description: Executes the ParticleView operation.
ParticleView::ParticleView(grav::ViewMode mode)
    : QWidget(nullptr),
      _mode(mode),
      _snapshot(std::nullopt),
      _zoom(8.0f),
      _luminosity(100),
      _camera{0.0f, 0.0f, 0.0f},
      _adaptiveTemperatureScale(2.0f),
      _adaptivePressureScale(2.0f),
      /// Description: Executes the _dragAxis operation.
      _dragAxis(grav::GimbalAxis::None)
{
    /// Description: Executes the setMinimumSize operation.
    setMinimumSize(220, 180);
    /// Description: Executes the setSizePolicy operation.
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}
/// Description: Executes the setSnapshot operation.
void ParticleView::setSnapshot(const std::vector<RenderParticle>& snapshot)
{
    _snapshot = std::cref(snapshot);
    /// Description: Executes the update operation.
    update();
}
/// Description: Executes the setMode operation.
void ParticleView::setMode(grav::ViewMode mode)
{
    _mode = mode;
    /// Description: Executes the update operation.
    update();
}
/// Description: Executes the setZoom operation.
void ParticleView::setZoom(float zoom)
{
    _zoom = std::max(0.1f, zoom);
    /// Description: Executes the update operation.
    update();
}
/// Description: Executes the setLuminosity operation.
void ParticleView::setLuminosity(int luminosity)
{
    _luminosity = std::clamp(luminosity, 0, 255);
    /// Description: Executes the update operation.
    update();
}
/// Description: Executes the setCameraAngles operation.
void ParticleView::setCameraAngles(float yaw, float pitch, float roll)
{
    _camera.yaw = yaw;
    _camera.pitch = pitch;
    _camera.roll = roll;
    /// Description: Executes the update operation.
    update();
}
/// Description: Executes the setRenderSettings operation.
void ParticleView::setRenderSettings(bool culling, bool lod, float nearDist, float farDist)
{
    _cullingEnabled = culling;
    _lodEnabled = lod;
    _lodNearDistance = nearDist;
    _lodFarDistance = farDist;
    /// Description: Executes the update operation.
    update();
}
void ParticleView::setOctreeOverlay(const std::vector<OctreeOverlayNode>& overlay, bool enabled,
                                    int opacity)
{
    _octreeOverlay = std::cref(overlay);
    _octreeOverlayEnabled = enabled;
    _octreeOverlayOpacity = std::clamp(opacity, 0, 255);
    /// Description: Executes the update operation.
    update();
}
/// Description: Executes the mousePressEvent operation.
void ParticleView::mousePressEvent(MouseEventHandle event)
{
    if (event->button() != Qt::LeftButton) {
        /// Description: Executes the mousePressEvent operation.
        QWidget::mousePressEvent(event);
        return;
    }
    const GimbalOverlay gimbal = computeGimbal(rect(), _mode, _camera);
    if (!gimbal.rect.contains(event->position())) {
        /// Description: Executes the mousePressEvent operation.
        QWidget::mousePressEvent(event);
        return;
    }
    const grav::GimbalAxis axis = pickGimbalAxis(gimbal, event->position());
    if (axis == grav::GimbalAxis::None) {
        /// Description: Executes the mousePressEvent operation.
        QWidget::mousePressEvent(event);
        return;
    }
    _dragAxis = axis;
    _lastMousePos = event->position();
    event->accept();
}
/// Description: Executes the mouseMoveEvent operation.
void ParticleView::mouseMoveEvent(MouseEventHandle event)
{
    if (_dragAxis == grav::GimbalAxis::None) {
        /// Description: Executes the mouseMoveEvent operation.
        QWidget::mouseMoveEvent(event);
        return;
    }
    const QPointF pos = event->position();
    const float dx = event->position().x() - _lastMousePos.x();
    const float dy = event->position().y() - _lastMousePos.y();
    if (_dragAxis == grav::GimbalAxis::X) {
        _camera.yaw += dx * 0.5f;
    }
    else if (_dragAxis == grav::GimbalAxis::Y) {
        _camera.pitch -= dy * 0.5f;
    }
    else if (_dragAxis == grav::GimbalAxis::Z) {
        _camera.roll += dx * 0.5f;
    }
    _lastMousePos = pos;
    /// Description: Executes the update operation.
    update();
    event->accept();
}
/// Description: Executes the mouseReleaseEvent operation.
void ParticleView::mouseReleaseEvent(MouseEventHandle event)
{
    if (_dragAxis != grav::GimbalAxis::None) {
        _dragAxis = grav::GimbalAxis::None;
        event->accept();
        return;
    }
    /// Description: Executes the mouseReleaseEvent operation.
    QWidget::mouseReleaseEvent(event);
}
/// Description: Executes the paintEvent operation.
void ParticleView::paintEvent(PaintEventHandle event)
{
    (void)event;
    if (!_snapshot) {
        return;
    }
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
        const std::vector<RenderParticle>& snapshot = _snapshot->get();
        /// Description: Executes the updateAdaptiveScales operation.
        updateAdaptiveScales(snapshot, _adaptiveTemperatureScale, _adaptivePressureScale);
        const float lodNear2 = _lodNearDistance * _lodNearDistance;
        const float lodFar2 = _lodFarDistance * _lodFarDistance;
        for (const RenderParticle& particle : snapshot)
            if (_lodEnabled && _mode == grav::ViewMode::Perspective) {
                const float dist2 =
                    particle.x * particle.x + particle.y * particle.y + particle.z * particle.z;
                if (dist2 > lodNear2) {
                    const float factor = std::clamp((std::sqrt(dist2) - _lodNearDistance) /
                                                        (_lodFarDistance - _lodNearDistance),
                                                    0.0f, 1.0f);
                    const std::size_t skipThreshold = static_cast<std::size_t>(factor * 100.0f);
                    // Use a stable skip based on particle index/address for deterministic
                    // appearance
                    if ((reinterpret_cast<std::uintptr_t>(&particle) / sizeof(RenderParticle)) %
                            100 <
                        skipThreshold) {
                        continue;
                    }
                }
                const grav::ProjectedPoint pp = grav::projectParticle(particle, _mode, _camera);
                if (!pp.valid)
                    continue;
                const int x = static_cast<int>(centerX + pp.x * _zoom);
                const int y = static_cast<int>(centerY - pp.y * _zoom);
                if (_cullingEnabled) {
                    if (x < 0 || x >= widthPx || y < 0 || y >= heightPx)
                        continue;
                }
                else {
                    if (x < -100 || x >= widthPx + 100 || y < -100 || y >= heightPx + 100)
                        continue;
                }
                QRgb color = 0;
                if (isHeavyBody(particle)) {
                    color = heavyColor;
                }
                else {
                    color = particleRampColorFast(particle, _adaptiveTemperatureScale,
                                                  _adaptivePressureScale, _luminosity);
                }
                _framebuffer.setPixel(x, y, color);
            }
    }
    /// Description: Executes the painter operation.
    QPainter painter(this);
    painter.drawImage(0, 0, _framebuffer);
    if (_octreeOverlayEnabled && _octreeOverlay.has_value()) {
        OctreeOverlayPainter::paint(painter, rect(), _mode, _camera, _octreeOverlay->get(), _zoom,
                                    _octreeOverlayOpacity);
    }
    const GimbalOverlay gimbal = computeGimbal(rect(), _mode, _camera);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(QColor(120, 120, 135), 1.0));
    painter.setBrush(QColor(18, 18, 26, 180));
    painter.drawEllipse(gimbal.rect);
    const std::array<QColor, 3> colors = {QColor(255, 95, 95), QColor(85, 255, 125),
                                          QColor(105, 150, 255)};
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
