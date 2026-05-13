/*
 * @file modules/qt/src/widgets/graphs/Paint.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#ifndef BLITZAR_MODULES_QT_SRC_WIDGETS_GRAPHS_PAINT_HPP_
#define BLITZAR_MODULES_QT_SRC_WIDGETS_GRAPHS_PAINT_HPP_
/*
 * Module: qt
 * Responsibility: Paint the energy timeline widget using the current telemetry
 * history.
 */
#include "support/geometry/ViewMath.hpp"
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
void paintGraph(QWidget& widget, const std::vector<EnergyPoint>& history, UiPaintEvent* event);
} // namespace bltzr_qt
#endif // BLITZAR_MODULES_QT_SRC_WIDGETS_GRAPHS_PAINT_HPP_
