/*
 * @file modules/qt/src/widgets/viewport/MultiView.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "widgets/viewport/MultiView.hpp"
#include "Constants.hpp"
#include "widgets/viewport/RenderSnapshot.hpp"
#include <QGridLayout>
#include <QSizePolicy>
#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <utility>
#include <vector>

namespace bltzr_qt {
MultiView::MultiView(QWidget* parent)
    : QWidget(parent),
      _cpuViews{},
      _gpuViews{},
      _viewStacks{new QStackedWidget(this), new QStackedWidget(this), new QStackedWidget(this),
                  new QStackedWidget(this)},
      _gpuBackend(qgetenv("BLITZAR_RENDERER").compare("cpu", Qt::CaseInsensitive) != 0),
      _maxDrawParticles(50000u),
      _zoom(kDefaultZoom),
      _octreeOverlayEnabled(false),
      _octreeOverlayDepth(kOverlayDepthDefault),
      _octreeOverlayOpacity(kOverlayOpacityDefault)
{
    _cpuViews = {new Particle(grav::ViewMode::XY, _viewStacks[0]),
                 new Particle(grav::ViewMode::XZ, _viewStacks[1]),
                 new Particle(grav::ViewMode::YZ, _viewStacks[2]),
                 new Particle(grav::ViewMode::Perspective, _viewStacks[3])};
    _gpuViews = {new GpuView(grav::ViewMode::XY, _viewStacks[0]),
                 new GpuView(grav::ViewMode::XZ, _viewStacks[1]),
                 new GpuView(grav::ViewMode::YZ, _viewStacks[2]),
                 new GpuView(grav::ViewMode::Perspective, _viewStacks[3])};
    auto* grid = new QGridLayout(this);
    grid->setSpacing(6);
    grid->setContentsMargins(0, 0, 0, 0);
    for (std::size_t index = 0; index < _viewStacks.size(); ++index) {
        _viewStacks[index]->addWidget(_cpuViews[index]);
        _viewStacks[index]->addWidget(_gpuViews[index]);
        if (_gpuBackend) {
            _viewStacks[index]->setCurrentWidget(_gpuViews[index]);
        }
        else {
            _viewStacks[index]->setCurrentWidget(_cpuViews[index]);
        }
        _gpuViews[index]->setUnavailableCallback([this]() {
            activateCpuBackend();
        });
    }
    grid->addWidget(_viewStacks[0], 0, 0);
    grid->addWidget(_viewStacks[1], 0, 1);
    grid->addWidget(_viewStacks[2], 1, 0);
    grid->addWidget(_viewStacks[3], 1, 1);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void MultiView::setSnapshot(std::vector<RenderParticle> snapshot)
{
    // Rendering uses a local frame; simulation and export coordinates remain untouched.
    _snapshot = prepareRenderSnapshot(std::move(snapshot), _maxDrawParticles);
    rebuildOctreeOverlay();
    for (std::size_t index = 0; index < _cpuViews.size(); ++index) {
        _cpuViews[index]->setSnapshot(_snapshot);
        _gpuViews[index]->setSnapshot(_snapshot);
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

std::string MultiView::rendererStatusText() const
{
    bool densityEnabled = false;
    for (const RenderParticle& particle : _snapshot) {
        if (particle.densityNorm > 0.0f) {
            densityEnabled = true;
            break;
        }
    }
    const std::string densityStatus = densityEnabled ? " density=coarse-grid" : " density=off";
    std::ostringstream zoomStatus;
    zoomStatus << " zoom_lambda=" << std::fixed << std::setprecision(2)
               << zoomCompensationLambda(_zoom);
    if (!_gpuBackend) {
        return "renderer=qimage-cpu points=" + std::to_string(_snapshot.size()) + zoomStatus.str() +
               densityStatus;
    }
    std::size_t readyCount = 0u;
    float frameMs = 0.0f;
    float uploadMs = 0.0f;
    std::size_t points = 0u;
    for (const QPointer<GpuView>& view : _gpuViews) {
        if (!view || !view->isReady()) {
            continue;
        }
        const GpuViewMetrics metrics = view->metrics();
        readyCount += 1u;
        frameMs += metrics.lastFrameMs;
        uploadMs += metrics.lastUploadMs;
        points = std::max(points, metrics.uploadedPoints);
    }
    if (readyCount == 0u) {
        return "renderer=opengl initializing points=" + std::to_string(_snapshot.size()) +
               zoomStatus.str() + densityStatus;
    }
    std::ostringstream text;
    text << "renderer=opengl ready=" << readyCount << "/4 points=" << points << zoomStatus.str()
         << " submit_ms=" << std::fixed << std::setprecision(3)
         << (frameMs / static_cast<float>(readyCount))
         << " upload_ms=" << (uploadMs / static_cast<float>(readyCount)) << densityStatus;
    return text.str();
}

void MultiView::setZoom(float zoom)
{
    _zoom = std::max(kViewportMinZoom, zoom);
    for (std::size_t index = 0; index < _cpuViews.size(); ++index) {
        _cpuViews[index]->setZoom(_zoom);
        _gpuViews[index]->setZoom(_zoom);
    }
}

float MultiView::zoomCompensation() const
{
    return zoomCompensationLambda(_zoom);
}

void MultiView::setLuminosity(int luminosity)
{
    for (std::size_t index = 0; index < _cpuViews.size(); ++index) {
        _cpuViews[index]->setLuminosity(luminosity);
        _gpuViews[index]->setLuminosity(luminosity);
    }
}

void MultiView::set3DMode(grav::ViewMode mode)
{
    _cpuViews[3]->setMode(mode);
    _gpuViews[3]->setMode(mode);
}

void MultiView::set3DCameraAngles(float yaw, float pitch, float roll)
{
    for (std::size_t index = 0; index < _cpuViews.size(); ++index) {
        _cpuViews[index]->setCameraAngles(yaw, pitch, roll);
        _gpuViews[index]->setCameraAngles(yaw, pitch, roll);
    }
}

void MultiView::setRenderSettings(bool culling, bool lod, float nearDist, float farDist)
{
    for (std::size_t index = 0; index < _cpuViews.size(); ++index) {
        _cpuViews[index]->setRenderSettings(culling, lod, nearDist, farDist);
        _gpuViews[index]->setRenderSettings(culling, lod, nearDist, farDist);
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
    for (std::size_t index = 0; index < _cpuViews.size(); ++index) {
        _cpuViews[index]->setOctreeOverlay(_octreeOverlay, _octreeOverlayEnabled,
                                           _octreeOverlayOpacity);
        _gpuViews[index]->setOctreeOverlay(_octreeOverlay, _octreeOverlayEnabled,
                                           _octreeOverlayOpacity);
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

void MultiView::activateCpuBackend()
{
    if (!_gpuBackend) {
        return;
    }
    _gpuBackend = false;
    for (std::size_t index = 0; index < _viewStacks.size(); ++index) {
        _viewStacks[index]->setCurrentWidget(_cpuViews[index]);
    }
}
} // namespace bltzr_qt
