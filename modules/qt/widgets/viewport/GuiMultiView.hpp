/*
 * @file modules/qt/widgets/viewport/GuiMultiView.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#ifndef BLITZAR_MODULES_QT_SRC_WIDGETS_VIEWPORT_MULTIVIEW_HPP_
#define BLITZAR_MODULES_QT_SRC_WIDGETS_VIEWPORT_MULTIVIEW_HPP_
/*
 * Module: qt
 * Responsibility: Coordinate the four synchronized particle views shown in the Qt
 * workspace.
 */
#include "widgets/overlays/GuiOctree.hpp"
#include "widgets/viewport/GuiGpuView.hpp"
#include "widgets/viewport/GuiParticle.hpp"
#include <QPointer>
#include <QStackedWidget>
#include <QWidget>
#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace bltzr_qt {
class MultiView : public QWidget {
public:
    explicit MultiView(QWidget* parent = nullptr);
    void setSnapshot(std::vector<RenderParticle> snapshot);
    void setMaxDrawParticles(std::size_t maxDrawParticles);
    std::size_t displayedParticleCount() const;
    std::string rendererStatusText() const;
    void setZoom(float zoom);
    void setLuminosity(int luminosity);
    void set3DMode(grav::ViewMode mode);
    void set3DCameraAngles(float yaw, float pitch, float roll);
    void setRenderSettings(bool culling, bool lod, float nearDist, float farDist);
    void setOctreeOverlay(bool enabled, int depth, int opacity);
    bool octreeOverlayEnabled() const;
    int octreeOverlayDepth() const;
    int octreeOverlayOpacity() const;
    std::size_t octreeOverlayNodeCount() const;
    float zoomCompensation() const;

private:
    void applyOctreeOverlay();
    void rebuildOctreeOverlay();
    void activateCpuBackend();
    std::array<QPointer<Particle>, 4> _cpuViews;
    std::array<QPointer<GpuView>, 4> _gpuViews;
    std::array<QPointer<QStackedWidget>, 4> _viewStacks;
    bool _gpuBackend;
    std::size_t _maxDrawParticles;
    float _zoom;
    std::vector<RenderParticle> _snapshot;
    std::vector<OctreeNode> _octreeOverlay;
    bool _octreeOverlayEnabled;
    int _octreeOverlayDepth;
    int _octreeOverlayOpacity;
};
} // namespace bltzr_qt
#endif // BLITZAR_MODULES_QT_SRC_WIDGETS_VIEWPORT_MULTIVIEW_HPP_
