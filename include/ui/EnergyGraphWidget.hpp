#ifndef GRAVITY_UI_ENERGYGRAPHWIDGET_H
#define GRAVITY_UI_ENERGYGRAPHWIDGET_H

#include "ui/QtViewMath.hpp"

#include <QWidget>

#include <vector>

class QPaintEvent;

namespace qtui {

class EnergyGraphWidget : public QWidget {
    public:
        explicit EnergyGraphWidget(QWidget *parent = nullptr);

        void pushSample(const SimulationStats &stats);

    protected:
        void paintEvent(QPaintEvent *event) override;

    private:
        std::vector<EnergyPoint> _history;
};

} // namespace qtui

#endif // GRAVITY_UI_ENERGYGRAPHWIDGET_H
