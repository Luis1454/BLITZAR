/*
 * @file modules/qt/include/ui/energyGraph/plotting.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Plot rendering helpers for energy graph widget painting.
 */

#ifndef BLITZAR_MODULES_QT_INCLUDE_UI_ENERGYGRAPH_PLOTTING_HPP_
#define BLITZAR_MODULES_QT_INCLUDE_UI_ENERGYGRAPH_PLOTTING_HPP_

#include "ui/energyGraph/data.hpp"
#include "ui/energyGraph/layout.hpp"
#include "ui/energyGraph/theme.hpp"

#include <QPainter>

namespace bltzr_qt::energyGraph {

void drawCurves(QPainter& painter, const Layout& layout, const ColorPalette& palette,
                const std::vector<EnergyPoint>& history, const DataRange& dataRange);
void drawCurrentMarkers(QPainter& painter, const Layout& layout, const ColorPalette& palette,
                        const EnergyPoint& latest, const DataRange& dataRange);
void drawAxisValues(QPainter& painter, const Layout& layout, const ColorPalette& palette,
                    const DataRange& dataRange);

} // namespace bltzr_qt::energyGraph

#endif // BLITZAR_MODULES_QT_INCLUDE_UI_ENERGYGRAPH_PLOTTING_HPP_
