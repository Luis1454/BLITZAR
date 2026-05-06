/*
 * @file modules/qt/include/ui/energyGraph/layout.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Layout and geometry computation for energy graph widget painting.
 */

#ifndef BLTZR_QT_ENERGY_GRAPH_LAYOUT_HPP
#define BLTZR_QT_ENERGY_GRAPH_LAYOUT_HPP

#include <QFontMetricsF>
#include <QRect>
#include <QRectF>
#include <QStringList>

namespace bltzr_qt::energyGraph {

/**
 * @brief Computed layout geometry for energy graph widget.
 */
struct Layout {
    QRectF outerRect;
    QRectF headerRect;
    QRectF legendRect;
    QRectF energyRect;
    QRectF driftRect;
    qreal headerLineHeight = 0.0;
    int legendRows = 1;
};

/**
 * @brief Compute layout geometry given widget bounds and font metrics.
 * @param widgetRect The widget's bounding rectangle
 * @param fontMetrics Font metrics for text sizing
 * @param shortLabels Legend labels for width calculation
 * @return Fully computed layout structure
 */
Layout computeLayout(const QRect& widgetRect, const QFontMetricsF& fontMetrics,
                     const QStringList& shortLabels);

} // namespace bltzr_qt::energyGraph

#endif // BLTZR_QT_ENERGY_GRAPH_LAYOUT_HPP
