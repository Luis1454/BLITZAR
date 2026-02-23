#ifndef GRAVITY_UI_ENERGYGRAPHWIDGET_H
#define GRAVITY_UI_ENERGYGRAPHWIDGET_H

#include "ui/QtViewMath.hpp"

#include <QWidget>

#include <vector>

class QPaintEvent;

using UiPaintEvent = QPaintEvent;

namespace qtui {

class EnergyGraphWidget : public QWidget {
    public:
        explicit EnergyGraphWidget();

        void pushSample(const SimulationStats &stats);

    private:
        using PaintEventHandle = UiPaintEvent *;
        void paintEvent(PaintEventHandle event) override;
        std::vector<EnergyPoint> _history;
};

} // namespace qtui

#endif // GRAVITY_UI_ENERGYGRAPHWIDGET_H
