#include "ui/MultiViewWidget.hpp"

#include <QGridLayout>
#include <QSizePolicy>

#include <algorithm>
#include <cstddef>
#include <utility>

namespace grav_qt {

MultiViewWidget::MultiViewWidget()
    : QWidget(nullptr),
      _xy(new ParticleView(ViewMode::XY)),
      _xz(new ParticleView(ViewMode::XZ)),
      _yz(new ParticleView(ViewMode::YZ)),
      _view3d(new ParticleView(ViewMode::Perspective)),
      _maxDrawParticles(50000u)
{
    auto *grid = new QGridLayout(this);
    grid->setSpacing(6);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->addWidget(_xy, 0, 0);
    grid->addWidget(_xz, 0, 1);
    grid->addWidget(_yz, 1, 0);
    grid->addWidget(_view3d, 1, 1);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void MultiViewWidget::setSnapshot(std::vector<RenderParticle> snapshot)
{
    const std::size_t cap = std::max<std::size_t>(2u, _maxDrawParticles);
    if (snapshot.size() <= cap) {
        _snapshot = std::move(snapshot);
    } else {
        _snapshot.clear();
        _snapshot.reserve(cap);
        const std::size_t stride = (snapshot.size() + cap - 1u) / cap;
        for (std::size_t i = 0; i < snapshot.size() && _snapshot.size() < cap; i += stride) {
            _snapshot.push_back(snapshot[i]);
        }
    }

    if (_xy) {
        _xy->setSnapshot(_snapshot);
    }
    if (_xz) {
        _xz->setSnapshot(_snapshot);
    }
    if (_yz) {
        _yz->setSnapshot(_snapshot);
    }
    if (_view3d) {
        _view3d->setSnapshot(_snapshot);
    }
}

void MultiViewWidget::setMaxDrawParticles(std::size_t maxDrawParticles)
{
    _maxDrawParticles = std::max<std::size_t>(2u, maxDrawParticles);
}

std::size_t MultiViewWidget::displayedParticleCount() const
{
    return _snapshot.size();
}

void MultiViewWidget::setZoom(float zoom)
{
    if (_xy) {
        _xy->setZoom(zoom);
    }
    if (_xz) {
        _xz->setZoom(zoom);
    }
    if (_yz) {
        _yz->setZoom(zoom);
    }
    if (_view3d) {
        _view3d->setZoom(zoom);
    }
}

void MultiViewWidget::setLuminosity(int luminosity)
{
    if (_xy) {
        _xy->setLuminosity(luminosity);
    }
    if (_xz) {
        _xz->setLuminosity(luminosity);
    }
    if (_yz) {
        _yz->setLuminosity(luminosity);
    }
    if (_view3d) {
        _view3d->setLuminosity(luminosity);
    }
}

void MultiViewWidget::set3DMode(ViewMode mode)
{
    if (_view3d) {
        _view3d->setMode(mode);
    }
}

void MultiViewWidget::set3DCameraAngles(float yaw, float pitch, float roll)
{
    if (_view3d) {
        _view3d->setCameraAngles(yaw, pitch, roll);
    }
}

} // namespace grav_qt
