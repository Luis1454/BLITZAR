/*
 * @file modules/qt/include/ui/energyGraph/renderer.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Energy graph paint implementation entry point.
 */

#ifndef BLITZAR_MODULES_QT_INCLUDE_UI_ENERGYGRAPH_RENDERER_HPP_
#define BLITZAR_MODULES_QT_INCLUDE_UI_ENERGYGRAPH_RENDERER_HPP_

#include "ui/EnergyGraphWidgetPaint.hpp"

namespace bltzr_qt::energyGraph {

void paint(QWidget& widget, const std::vector<EnergyPoint>& history, UiPaintEvent* event);

} // namespace bltzr_qt::energyGraph

#endif // BLITZAR_MODULES_QT_INCLUDE_UI_ENERGYGRAPH_RENDERER_HPP_
