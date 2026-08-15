/*
 * @file modules/qt/src/widgets/viewport/MultiView.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "Constants.hpp"
#include "widgets/viewport/MultiView.hpp"
#include <QByteArray>
#include <QGridLayout>
#include <QSizePolicy>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <limits>
#include <sstream>
#include <utility>
#include <vector>

namespace bltzr_qt {
namespace {
std::vector<RenderParticle> spatialSample(const std::vector<RenderParticle>& input,
                                          std::size_t cap)
{
    if (input.size() <= cap) {
        return input;
    }
    constexpr int kGridSide = 16;
    constexpr int kBinCount = kGridSide * kGridSide * kGridSide;
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    float maxZ = std::numeric_limits<float>::lowest();
    for (const RenderParticle& particle : input) {
        minX = std::min(minX, particle.x);
        minY = std::min(minY, particle.y);
        minZ = std::min(minZ, particle.z);
        maxX = std::max(maxX, particle.x);
        maxY = std::max(maxY, particle.y);
        maxZ = std::max(maxZ, particle.z);
    }
    const float scaleX = static_cast<float>(kGridSide) / std::max(1.0e-6f, maxX - minX);
    const float scaleY = static_cast<float>(kGridSide) / std::max(1.0e-6f, maxY - minY);
    const float scaleZ = static_cast<float>(kGridSide) / std::max(1.0e-6f, maxZ - minZ);
    std::vector<int> bins(kBinCount, -1);
    for (std::size_t index = 0u; index < input.size(); ++index) {
        const RenderParticle& particle = input[index];
        const int x = std::clamp(static_cast<int>((particle.x - minX) * scaleX), 0, kGridSide - 1);
        const int y = std::clamp(static_cast<int>((particle.y - minY) * scaleY), 0, kGridSide - 1);
        const int z = std::clamp(static_cast<int>((particle.z - minZ) * scaleZ), 0, kGridSide - 1);
        const int bin = x + kGridSide * (y + kGridSide * z);
        if (bins[static_cast<std::size_t>(bin)] < 0 ||
            particle.mass > input[static_cast<std::size_t>(bins[static_cast<std::size_t>(bin)])].mass) {
            bins[static_cast<std::size_t>(bin)] = static_cast<int>(index);
        }
    }
    std::vector<RenderParticle> result;
    result.reserve(cap);
    std::vector<unsigned char> selected(input.size(), 0u);
    for (const int index : bins) {
        if (index < 0 || result.size() >= cap) {
            continue;
        }
        result.push_back(input[static_cast<std::size_t>(index)]);
        selected[static_cast<std::size_t>(index)] = 1u;
    }
    const std::size_t stride = std::max<std::size_t>(1u, (input.size() + cap - 1u) / cap);
    for (std::size_t index = 0u; index < input.size() && result.size() < cap; index += stride) {
        if (selected[index] == 0u) {
            result.push_back(input[index]);
        }
    }
    return result;
}

void centerRenderSnapshot(std::vector<RenderParticle>& snapshot)
{
    if (snapshot.empty()) {
        return;
    }
    float minX = snapshot.front().x;
    float minY = snapshot.front().y;
    float minZ = snapshot.front().z;
    float maxX = minX;
    float maxY = minY;
    float maxZ = minZ;
    for (const RenderParticle& particle : snapshot) {
        minX = std::min(minX, particle.x);
        minY = std::min(minY, particle.y);
        minZ = std::min(minZ, particle.z);
        maxX = std::max(maxX, particle.x);
        maxY = std::max(maxY, particle.y);
        maxZ = std::max(maxZ, particle.z);
    }
    const float centerX = 0.5f * (minX + maxX);
    const float centerY = 0.5f * (minY + maxY);
    const float centerZ = 0.5f * (minZ + maxZ);
    for (RenderParticle& particle : snapshot) {
        particle.x -= centerX;
        particle.y -= centerY;
        particle.z -= centerZ;
    }
}
} // namespace

MultiView::MultiView()
    : QWidget(nullptr),
      _cpuViews{new Particle(grav::ViewMode::XY), new Particle(grav::ViewMode::XZ),
                new Particle(grav::ViewMode::YZ), new Particle(grav::ViewMode::Perspective)},
      _gpuViews{new GpuView(grav::ViewMode::XY), new GpuView(grav::ViewMode::XZ),
                new GpuView(grav::ViewMode::YZ), new GpuView(grav::ViewMode::Perspective)},
      _viewStacks{new QStackedWidget(), new QStackedWidget(), new QStackedWidget(),
                  new QStackedWidget()},
      _gpuBackend(qgetenv("BLITZAR_RENDERER").compare("cpu", Qt::CaseInsensitive) != 0),
      _maxDrawParticles(50000u),
      _zoom(kDefaultZoom),
      _octreeOverlayEnabled(false),
      _octreeOverlayDepth(kOverlayDepthDefault),
      _octreeOverlayOpacity(kOverlayOpacityDefault)
{
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
        _gpuViews[index]->setUnavailableCallback([this]() { activateCpuBackend(); });
    }
    grid->addWidget(_viewStacks[0], 0, 0);
    grid->addWidget(_viewStacks[1], 0, 1);
    grid->addWidget(_viewStacks[2], 1, 0);
    grid->addWidget(_viewStacks[3], 1, 1);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void MultiView::setSnapshot(std::vector<RenderParticle> snapshot)
{
    const std::size_t cap = std::max<std::size_t>(2u, _maxDrawParticles);
    // Rendering uses a local frame; simulation and export coordinates remain untouched.
    centerRenderSnapshot(snapshot);
    _snapshot = spatialSample(snapshot, cap);
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
    text << "renderer=opengl ready=" << readyCount << "/4 points=" << points
         << zoomStatus.str()
         << " submit_ms=" << std::fixed << std::setprecision(3)
         << (frameMs / static_cast<float>(readyCount)) << " upload_ms="
         << (uploadMs / static_cast<float>(readyCount)) << densityStatus;
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
