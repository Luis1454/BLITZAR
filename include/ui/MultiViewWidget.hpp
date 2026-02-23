#ifndef GRAVITY_UI_MULTIVIEWWIDGET_H
#define GRAVITY_UI_MULTIVIEWWIDGET_H

#include "ui/ParticleView.hpp"

#include <QWidget>

#include <cstddef>
#include <vector>

namespace qtui {

class MultiViewWidget : public QWidget {
    public:
        explicit MultiViewWidget();

        void setSnapshot(std::vector<RenderParticle> snapshot);
        void setMaxDrawParticles(std::size_t maxDrawParticles);
        std::size_t displayedParticleCount() const;
        void setZoom(float zoom);
        void setLuminosity(int luminosity);
        void set3DMode(ViewMode mode);
        void set3DCameraAngles(float yaw, float pitch, float roll);

    private:
        ParticleView _xy;
        ParticleView _xz;
        ParticleView _yz;
        ParticleView _view3d;
        std::size_t _maxDrawParticles;
        std::vector<RenderParticle> _snapshot;
};

} // namespace qtui

#endif // GRAVITY_UI_MULTIVIEWWIDGET_H
