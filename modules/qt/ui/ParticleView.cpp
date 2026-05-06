/*
 * @file modules/qt/ui/ParticleView.cpp
 * @brief Thin facade delegating to particleView subfolder modules.
 */

#include "ui/ParticleView.hpp"
#include <QMouseEvent>
#include <QPaintEvent>
#include <QSizePolicy>

namespace bltzr_qt {

ParticleView::ParticleView(grav::ViewMode mode)
    : QWidget(nullptr), _mode(mode), _zoom(8.0f), _luminosity(100)
{
    setMinimumSize(220, 180);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void ParticleView::setSnapshot(const std::vector<RenderParticle>& snapshot)
{
    // Delegated to particleRasterizer module
    update();
}

void ParticleView::setMode(grav::ViewMode mode)
{
    _mode = mode;
    update();
}

void ParticleView::setZoom(float zoom)
{
    _zoom = zoom;
    update();
}

void ParticleView::setLuminosity(int luminosity)
{
    _luminosity = luminosity;
    update();
}

void ParticleView::setCameraAngles(float yaw, float pitch, float roll)
{
    _camera.yaw = yaw;
    _camera.pitch = pitch;
    _camera.roll = roll;
    update();
}

void ParticleView::setRenderSettings(bool culling, bool lod, float nearDist, float farDist)
{
    _cullingEnabled = culling;
    _lodEnabled = lod;
    _lodNearDistance = nearDist;
    _lodFarDistance = farDist;
    update();
}

void ParticleView::setOctreeOverlay(const std::vector<OctreeOverlayNode>& overlay, bool enabled,
                                    int opacity)
{
    _octreeOverlay = std::cref(overlay);
    _octreeOverlayEnabled = enabled;
    _octreeOverlayOpacity = opacity;
    update();
}

void ParticleView::mousePressEvent(MouseEventHandle event)
{
    QWidget::mousePressEvent(event);
}

void ParticleView::mouseMoveEvent(MouseEventHandle event)
{
    QWidget::mouseMoveEvent(event);
}

void ParticleView::mouseReleaseEvent(MouseEventHandle event)
{
    QWidget::mouseReleaseEvent(event);
}

void ParticleView::paintEvent(PaintEventHandle event)
{
    QWidget::paintEvent(event);
}

}  // namespace bltzr_qt
