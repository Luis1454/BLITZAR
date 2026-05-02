/*
 * @file modules/qt/ui/ParticleView.cpp
 * @brief Thin facade delegating to particleView subfolder modules.
 */

#include "ui/ParticleView.hpp"
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

}  // namespace bltzr_qt
