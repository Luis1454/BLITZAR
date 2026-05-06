/*
 * @file modules/qt/ui/energyGraph/layout.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Layout and geometry computation for energy graph widget painting.
 */

#include "ui/energyGraph/layout.hpp"
#include <QFontMetricsF>
#include <QRect>
#include <QRectF>
#include <QStringList>
#include <algorithm>

namespace bltzr_qt::energyGraph {

Layout computeLayout(const QRect& widgetRect, const QFontMetricsF& fontMetrics,
                     const QStringList& shortLabels)
{
    Layout layout;

    layout.outerRect = widgetRect.adjusted(12, 10, -12, -10);
    layout.headerLineHeight = fontMetrics.height() + 2.0;

    constexpr qreal legendInset = 2.0;
    constexpr qreal legendGapX = 10.0;
    constexpr qreal legendLineWidth = 12.0;
    constexpr qreal legendTextGap = 4.0;
    const qreal legendRowHeight = fontMetrics.height() + 4.0;
    const qreal legendAvailableWidth =
        std::max<qreal>(120.0, layout.outerRect.width() - legendInset * 2.0);

    int legendRows = 1;
    qreal legendRowWidth = 0.0;
    for (const QString& label : shortLabels) {
        const qreal entryWidth =
            legendLineWidth + legendTextGap + fontMetrics.horizontalAdvance(label) + legendGapX;
        if (legendRowWidth > 0.0 && legendRowWidth + entryWidth > legendAvailableWidth) {
            legendRows += 1;
            legendRowWidth = 0.0;
        }
        legendRowWidth += entryWidth;
    }
    layout.legendRows = legendRows;

    const qreal headerHeight = layout.headerLineHeight * 2.0 + 8.0;
    const qreal legendHeight = legendRows * legendRowHeight + 2.0;

    const QRectF plotRect(
        layout.outerRect.left() + 40.0, layout.outerRect.top() + headerHeight + legendHeight,
        std::max<qreal>(120.0, layout.outerRect.width() - 48.0),
        std::max<qreal>(64.0, layout.outerRect.height() - headerHeight - legendHeight - 18.0));

    const qreal splitY = plotRect.top() + plotRect.height() * 0.68;

    layout.headerRect = QRectF(layout.outerRect.left(), layout.outerRect.top(),
                              layout.outerRect.width(), headerHeight);
    layout.legendRect = QRectF(layout.outerRect.left(), layout.outerRect.top() + headerHeight,
                              layout.outerRect.width(), legendHeight);
    layout.energyRect =
        QRectF(plotRect.left(), plotRect.top(), plotRect.width(), splitY - plotRect.top() - 6.0);
    layout.driftRect = QRectF(plotRect.left(), splitY + 6.0, plotRect.width(),
                             plotRect.bottom() - (splitY + 6.0));

    return layout;
}

} // namespace bltzr_qt::energyGraph
