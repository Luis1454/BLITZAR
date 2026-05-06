/*
 * @file modules/qt/ui/EnergyGraphWidgetPaint.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "ui/EnergyGraphWidgetPaint.hpp"
#include "ui/energyGraph/renderer.hpp"

namespace bltzr_qt {

void EnergyGraphWidgetPaint::paint(QWidget& widget, const std::vector<EnergyPoint>& history,
                                   UiPaintEvent* event)
{
    energyGraph::paint(widget, history, event);
}

} // namespace bltzr_qt