/*
 * @file modules/qt/src/widgets/viewport/MultiView.hpp
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
#include "widgets/overlays/Octree.hpp"
#include "widgets/viewport/Particle.hpp"
#include <QPointer>
#include <QWidget>
#include <cstddef>
#include <vector>

namespace bltzr_qt {
class MultiView : public QWidget {
public:
    explicit MultiView();
    void setSnapshot(std::vector<RenderParticle> snapshot);
    void setMaxDrawParticles(std::size_t maxDrawParticles);
    std::size_t displayedParticleCount() const;
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

private:
    void applyOctreeOverlay();
    void rebuildOctreeOverlay();
    QPointer<Particle> _xy;
    QPointer<Particle> _xz;
    QPointer<Particle> _yz;
    QPointer<Particle> _view3d;
    std::size_t _maxDrawParticles;
    std::vector<RenderParticle> _snapshot;
    std::vector<OctreeNode> _octreeOverlay;
    bool _octreeOverlayEnabled;
    int _octreeOverlayDepth;
    int _octreeOverlayOpacity;
};
} // namespace bltzr_qt
#endif // BLITZAR_MODULES_QT_SRC_WIDGETS_VIEWPORT_MULTIVIEW_HPP_
