#pragma once

#include "ui/QtViewMath.hpp"

#include <QWidget>

#include <vector>

class QPaintEvent;

typedef QPaintEvent UiPaintEvent;
namespace grav_qt {

class EnergyGraphWidget : public QWidget {
    public:
        explicit EnergyGraphWidget();

        void pushSample(const SimulationStats &stats);

    private:
        typedef UiPaintEvent * PaintEventHandle;
        void paintEvent(PaintEventHandle event) override;
        std::vector<EnergyPoint> _history;
};

} // namespace grav_qt

