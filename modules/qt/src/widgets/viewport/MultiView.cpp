/*
 * @file modules/qt/src/widgets/viewport/MultiView.cpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "Constants.hpp"
#include "widgets/viewport/MultiView.hpp"
#include <QGridLayout>
#include <QSizePolicy>
#include <algorithm>
#include <cstddef>
#include <utility>

namespace bltzr_qt {
MultiView::MultiView()
    : QWidget(nullptr),
      _xy(new Particle(grav::ViewMode::XY)),
      _xz(new Particle(grav::ViewMode::XZ)),
      _yz(new Particle(grav::ViewMode::YZ)),
      _view3d(new Particle(grav::ViewMode::Perspective)),
      _maxDrawParticles(50000u),
      _octreeOverlayEnabled(false),
      _octreeOverlayDepth(kOverlayDepthDefault),
      _octreeOverlayOpacity(kOverlayOpacityDefault)
{
    auto* grid = new QGridLayout(this);
    grid->setSpacing(6);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->addWidget(_xy, 0, 0);
    grid->addWidget(_xz, 0, 1);
    grid->addWidget(_yz, 1, 0);
    grid->addWidget(_view3d, 1, 1);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void MultiView::setSnapshot(std::vector<RenderParticle> snapshot)
{
    const std::size_t cap = std::max<std::size_t>(2u, _maxDrawParticles);
    if (snapshot.size() <= cap) {
        _snapshot = std::move(snapshot);
    }
    else {
        _snapshot.clear();
        _snapshot.reserve(cap);
        const std::size_t stride = (snapshot.size() + cap - 1u) / cap;
        for (std::size_t i = 0; i < snapshot.size() && _snapshot.size() < cap; i += stride) {
            _snapshot.push_back(snapshot[i]);
        }
    }
    rebuildOctreeOverlay();
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
    applyOctreeOverlay();
}

void MultiView::setMaxDrawParticles(std::size_t maxDrawParticles)
{
    _maxDrawParticles = std::max<std::size_t>(2u, maxDrawParticles);
}

std::size_t MultiView::displayedParticleCount() const
{
    return _snapshot.size();
}

void MultiView::setZoom(float zoom)
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

void MultiView::setLuminosity(int luminosity)
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

void MultiView::set3DMode(grav::ViewMode mode)
{
    if (_view3d) {
        _view3d->setMode(mode);
    }
}

void MultiView::set3DCameraAngles(float yaw, float pitch, float roll)
{
    if (_view3d) {
        _view3d->setCameraAngles(yaw, pitch, roll);
    }
}

void MultiView::setRenderSettings(bool culling, bool lod, float nearDist, float farDist)
{
    if (_xy) {
        _xy->setRenderSettings(culling, lod, nearDist, farDist);
    }
    if (_xz) {
        _xz->setRenderSettings(culling, lod, nearDist, farDist);
    }
    if (_yz) {
        _yz->setRenderSettings(culling, lod, nearDist, farDist);
    }
    if (_view3d) {
        _view3d->setRenderSettings(culling, lod, nearDist, farDist);
    }
}

void MultiView::setOctreeOverlay(bool enabled, int depth, int opacity)
{
    const bool overlayChanged = _octreeOverlayEnabled != enabled || _octreeOverlayDepth != depth ||
                                _octreeOverlayOpacity != opacity;
    _octreeOverlayEnabled = enabled;
    _octreeOverlayDepth = std::clamp(depth, 0, kOverlayDepthMax);
    _octreeOverlayOpacity = std::clamp(opacity, kLuminosityMin, kLuminosityMax);
    if (!overlayChanged) {
        return;
    }
    rebuildOctreeOverlay();
    applyOctreeOverlay();
}

bool MultiView::octreeOverlayEnabled() const
{
    return _octreeOverlayEnabled;
}

int MultiView::octreeOverlayDepth() const
{
    return _octreeOverlayDepth;
}

int MultiView::octreeOverlayOpacity() const
{
    return _octreeOverlayOpacity;
}

std::size_t MultiView::octreeOverlayNodeCount() const
{
    return _octreeOverlay.size();
}

void MultiView::applyOctreeOverlay()
{
    if (_xy) {
        _xy->setOctreeOverlay(_octreeOverlay, _octreeOverlayEnabled, _octreeOverlayOpacity);
    }
    if (_xz) {
        _xz->setOctreeOverlay(_octreeOverlay, _octreeOverlayEnabled, _octreeOverlayOpacity);
    }
    if (_yz) {
        _yz->setOctreeOverlay(_octreeOverlay, _octreeOverlayEnabled, _octreeOverlayOpacity);
    }
    if (_view3d) {
        _view3d->setOctreeOverlay(_octreeOverlay, _octreeOverlayEnabled, _octreeOverlayOpacity);
    }
}

void MultiView::rebuildOctreeOverlay()
{
    if (!_octreeOverlayEnabled) {
        _octreeOverlay.clear();
        return;
    }
    _octreeOverlay = Octree::build(_snapshot, _octreeOverlayDepth);
}
} // namespace bltzr_qt
