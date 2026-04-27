// File: modules/qt/include/ui/MultiViewWidget.hpp
// Purpose: Client module implementation for BLITZAR extension workflows.

#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_MULTIVIEWWIDGET_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_MULTIVIEWWIDGET_HPP_
/*
 * Module: ui
 * Responsibility: Coordinate the four synchronized particle views shown in the Qt
 * workspace.
 */
#include "ui/OctreeOverlay.hpp"
#include "ui/ParticleView.hpp"
#include <QPointer>
#include <QWidget>
#include <cstddef>
#include <vector>
namespace grav_qt {
/// Owns the synchronized XY, XZ, YZ, and 3D particle views.
class MultiViewWidget : public QWidget {
public:
    /// Builds the default four-view layout.
    explicit MultiViewWidget();
    /// Replaces the shared snapshot shown by every sub-view.
    void setSnapshot(std::vector<RenderParticle> snapshot);
    /// Limits how many particles each sub-view may draw.
    void setMaxDrawParticles(std::size_t maxDrawParticles);
    /// Returns how many particles are currently displayed.
    std::size_t displayedParticleCount() const;
    /// Applies a shared zoom factor to all sub-views.
    void setZoom(float zoom);
    /// Applies a shared luminosity bias to all sub-views.
    void setLuminosity(int luminosity);
    /// Selects the rendering mode used by the 3D sub-view.
    void set3DMode(grav::ViewMode mode);
    /// Applies shared yaw, pitch, and roll angles to the 3D sub-view.
    void set3DCameraAngles(float yaw, float pitch, float roll);
    /// Applies the shared culling and level-of-detail settings.
    void setRenderSettings(bool culling, bool lod, float nearDist, float farDist);
    /// Enables the octree debugging overlay and updates its depth/opacity parameters.
    void setOctreeOverlay(bool enabled, int depth, int opacity);
    /// Reports whether the overlay is currently active.
    bool octreeOverlayEnabled() const;
    /// Reports the configured maximum overlay depth.
    int octreeOverlayDepth() const;
    /// Reports the configured overlay opacity.
    int octreeOverlayOpacity() const;
    /// Reports how many overlay cells are currently cached.
    std::size_t octreeOverlayNodeCount() const;

private:
    void applyOctreeOverlay();
    void rebuildOctreeOverlay();
    QPointer<ParticleView> _xy;
    QPointer<ParticleView> _xz;
    QPointer<ParticleView> _yz;
    QPointer<ParticleView> _view3d;
    std::size_t _maxDrawParticles;
    std::vector<RenderParticle> _snapshot;
    std::vector<OctreeOverlayNode> _octreeOverlay;
    bool _octreeOverlayEnabled;
    int _octreeOverlayDepth;
    int _octreeOverlayOpacity;
};
} // namespace grav_qt
#endif // GRAVITY_MODULES_QT_INCLUDE_UI_MULTIVIEWWIDGET_HPP_
