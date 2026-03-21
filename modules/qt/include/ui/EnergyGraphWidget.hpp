#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_ENERGYGRAPHWIDGET_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_ENERGYGRAPHWIDGET_HPP_

/*
 * Module: ui
 * Responsibility: Render the recent energy history exposed by the client runtime.
 */

#include "ui/QtViewMath.hpp"

#include <QStringList>
#include <QWidget>

#include <vector>

class QPaintEvent;

typedef QPaintEvent UiPaintEvent;
namespace grav_qt {

/// Renders the rolling energy and drift history for the current simulation session.
class EnergyGraphWidget : public QWidget {
    public:
        /// Builds an empty energy graph widget.
        explicit EnergyGraphWidget();

        /// Appends a telemetry sample to the visible energy history.
        void pushSample(const SimulationStats &stats);
        /// Returns the localized X-axis label for the energy plot.
        static QString energyXAxisLabel();
        /// Returns the localized Y-axis label for the energy plot.
        static QString energyYAxisLabel();
        /// Returns the localized X-axis label for the drift plot.
        static QString driftXAxisLabel();
        /// Returns the localized Y-axis label for the drift plot.
        static QString driftYAxisLabel();
        /// Returns the legend entries rendered by the graph.
        static QStringList legendLabels();

    private:
        typedef UiPaintEvent * PaintEventHandle;
        void paintEvent(PaintEventHandle event) override;
        std::vector<EnergyPoint> _history;
};

} // namespace grav_qt


#endif // GRAVITY_MODULES_QT_INCLUDE_UI_ENERGYGRAPHWIDGET_HPP_
