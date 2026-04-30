/*
 * @file modules/qt/include/ui/EnergyGraphWidget.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#ifndef BLITZAR_MODULES_QT_INCLUDE_UI_ENERGYGRAPHWIDGET_HPP_
#define BLITZAR_MODULES_QT_INCLUDE_UI_ENERGYGRAPHWIDGET_HPP_
/*
 * Module: ui
 * Responsibility: Render the recent energy history exposed by the client runtime.
 */
#include "ui/QtViewMath.hpp"
#include <QStringList>
#include <QWidget>
#include <vector>
/*
 * @brief Defines the qpaint event type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class QPaintEvent;
typedef QPaintEvent UiPaintEvent;

namespace bltzr_qt {
class EnergyGraphWidget : public QWidget {
public:
    explicit EnergyGraphWidget();
    void clearHistory();
    void pushSample(const SimulationStats& stats);
    std::size_t sampleCount() const;
    static QString energyXAxisLabel();
    static QString energyYAxisLabel();
    static QString driftXAxisLabel();
    static QString driftYAxisLabel();
    static QStringList legendLabels();

private:
    typedef UiPaintEvent* PaintEventHandle;
    void paintEvent(PaintEventHandle event) override;
    std::vector<EnergyPoint> _history;
};
} // namespace bltzr_qt
#endif // BLITZAR_MODULES_QT_INCLUDE_UI_ENERGYGRAPHWIDGET_HPP_
