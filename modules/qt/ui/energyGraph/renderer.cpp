/*
 * @file modules/qt/ui/energyGraph/renderer.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Energy graph paint implementation split into focused helpers.
 */

#include "ui/energyGraph/renderer.hpp"

#include "ui/EnergyGraphWidget.hpp"
#include "ui/energyGraph/data.hpp"
#include "ui/energyGraph/layout.hpp"
#include "ui/energyGraph/plotting.hpp"
#include "ui/energyGraph/theme.hpp"

#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QPen>
#include <array>
#include <algorithm>
#include <cmath>

namespace bltzr_qt::energyGraph {

static QString metricLabel(float value, const QString& suffix)
{
    return QString::number(value, std::fabs(value) >= 1000.0f ? 'g' : 'f',
                           std::fabs(value) >= 100.0f ? 1 : 2) + suffix;
}

static void drawAxisLabels(QPainter& painter, const Layout& layout, const ColorPalette& palette)
{
    painter.setPen(palette.label);
    painter.drawText(QRectF(layout.energyRect.left() + 6.0, layout.energyRect.top() + 2.0, 120.0, 14.0),
                     Qt::AlignLeft | Qt::AlignVCenter, EnergyGraphWidget::energyYAxisLabel());
    painter.drawText(QRectF(layout.driftRect.left() + 6.0, layout.driftRect.top() + 2.0, 100.0, 14.0),
                     Qt::AlignLeft | Qt::AlignVCenter, EnergyGraphWidget::driftYAxisLabel());
}

static void drawLegend(QPainter& painter, const Layout& layout, const QFontMetricsF& metrics,
                       const ColorPalette& palette, const QStringList& shortLabels)
{
    constexpr qreal legendInset = 2.0;
    constexpr qreal legendGapX = 10.0;
    constexpr qreal legendLineWidth = 12.0;
    constexpr qreal legendTextGap = 4.0;
    const qreal legendRowHeight = metrics.height() + 4.0;

    const std::array<QColor, 6> colors = {palette.kineticCurve, palette.potentialCurve,
                                          palette.thermalCurve, palette.radiatedCurve,
                                          palette.totalCurve, palette.driftCurve};
    qreal x = layout.legendRect.left() + legendInset;
    qreal y = layout.legendRect.top() + metrics.ascent();

    for (int i = 0; i < shortLabels.size() && i < static_cast<int>(colors.size()); ++i) {
        const QString& label = shortLabels.at(i);
        const qreal entryWidth =
            legendLineWidth + legendTextGap + metrics.horizontalAdvance(label) + legendGapX;
        if (x > layout.legendRect.left() + legendInset &&
            x + entryWidth > layout.legendRect.right() - legendInset) {
            x = layout.legendRect.left() + legendInset;
            y += legendRowHeight;
        }
        painter.setPen(QPen(colors.at(i), 2.0));
        painter.drawLine(QPointF(x, y - metrics.ascent() * 0.35),
                         QPointF(x + legendLineWidth, y - metrics.ascent() * 0.35));
        painter.setPen(palette.label);
        painter.drawText(QPointF(x + legendLineWidth + legendTextGap, y), label);
        x += entryWidth;
    }
}

template <typename MetricFormatter>
static void drawHeader(QPainter& painter, const Layout& layout, const ColorPalette& palette,
                       const std::vector<EnergyPoint>& history, const MetricFormatter& formatMetric)
{
    painter.setPen(palette.label);
    painter.drawText(QRectF(layout.headerRect.left(), layout.headerRect.top(), layout.headerRect.width(),
                            layout.headerLineHeight),
                     Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("Energy timeline"));

    if (history.empty()) {
        painter.drawText(QRectF(layout.headerRect.left(), layout.headerRect.top() + layout.headerLineHeight,
                                layout.headerRect.width(), layout.headerLineHeight),
                         Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("Waiting for telemetry"));
        return;
    }

    const EnergyPoint& latest = history.back();
    const QString summary = QStringLiteral("Total %1    Drift %2    Time %3    Samples %4")
                                .arg(formatMetric(latest.total, QStringLiteral(" J")))
                                .arg(formatMetric(latest.drift, QStringLiteral("%")))
                                .arg(formatMetric(latest.time, QStringLiteral(" s")))
                                .arg(history.size());
    painter.drawText(QRectF(layout.headerRect.left(), layout.headerRect.top() + layout.headerLineHeight,
                            layout.headerRect.width(), layout.headerLineHeight),
                     Qt::AlignLeft | Qt::AlignVCenter, summary);
}

void paint(QWidget& widget, const std::vector<EnergyPoint>& history, UiPaintEvent* event)
{
    (void)event;
    QPainter painter(&widget);
    const ColorPalette palette = buildColorPalette(widget.palette());

    painter.fillRect(widget.rect(), palette.background);
    painter.setPen(palette.border);
    painter.drawRect(widget.rect().adjusted(0, 0, -1, -1));

    const QStringList shortLabels = {QStringLiteral("Kin"),   QStringLiteral("Pot"),
                                     QStringLiteral("Therm"), QStringLiteral("Rad"),
                                     QStringLiteral("Total"), QStringLiteral("Drift")};
    const QFontMetricsF metrics = painter.fontMetrics();
    const Layout layout = computeLayout(widget.rect(), metrics, shortLabels);

    painter.setPen(palette.grid);
    painter.drawLine(layout.energyRect.bottomLeft() + QPointF(0.0, 6.0),
                     layout.energyRect.bottomRight() + QPointF(0.0, 6.0));
    painter.drawRect(layout.energyRect);
    painter.drawRect(layout.driftRect);

    const auto formatMetric = [](float value, const QString& suffix) {
        return metricLabel(value, suffix);
    };

    drawAxisLabels(painter, layout, palette);
    drawLegend(painter, layout, metrics, palette, shortLabels);
    drawHeader(painter, layout, palette, history, formatMetric);

    if (history.size() < 2) {
        painter.setPen(palette.label);
        painter.drawText(layout.energyRect, Qt::AlignCenter, QStringLiteral("Awaiting energy telemetry"));
        return;
    }

    const DataRange dataRange = computeDataRange(history);
    drawCurves(painter, layout, palette, history, dataRange);
    drawCurrentMarkers(painter, layout, palette, *dataRange.latestPoint, dataRange);
    drawAxisValues(painter, layout, palette, dataRange);
}

} // namespace bltzr_qt::energyGraph