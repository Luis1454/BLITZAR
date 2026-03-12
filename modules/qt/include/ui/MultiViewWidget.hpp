#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_MULTIVIEWWIDGET_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_MULTIVIEWWIDGET_HPP_

#include "ui/ParticleView.hpp"

#include <QPointer>
#include <QWidget>

#include <cstddef>
#include <vector>

namespace grav_qt {

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
        QPointer<ParticleView> _xy;
        QPointer<ParticleView> _xz;
        QPointer<ParticleView> _yz;
        QPointer<ParticleView> _view3d;
        std::size_t _maxDrawParticles;
        std::vector<RenderParticle> _snapshot;
};

} // namespace grav_qt


#endif // GRAVITY_MODULES_QT_INCLUDE_UI_MULTIVIEWWIDGET_HPP_
