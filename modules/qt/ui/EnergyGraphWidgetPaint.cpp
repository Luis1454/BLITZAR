/*
 * @file modules/qt/ui/EnergyGraphWidgetPaint.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "ui/EnergyGraphWidgetPaint.hpp"
#include "ui/EnergyGraphWidget.hpp"
#include "ui/energyGraph/theme.hpp"
#include "ui/energyGraph/layout.hpp"
#include "ui/energyGraph/data.hpp"
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPen>
#include <QRectF>
#include <QStringView>
#include <array>
#include <cmath>
#include <functional>

namespace bltzr_qt {
namespace eg = energyGraph;

void EnergyGraphWidgetPaint::paint(QWidget& widget, const std::vector<EnergyPoint>& history,
                                   UiPaintEvent* event)
{
    (void)event;
    QPainter p(&widget);
    const QPalette widgetPalette = widget.palette();
    const eg::ColorPalette palette = eg::buildColorPalette(widgetPalette);
    
    p.fillRect(widget.rect(), palette.background);
    p.setPen(palette.border);
    p.drawRect(widget.rect().adjusted(0, 0, -1, -1));
    
    const QStringList shortLabels = {QStringLiteral("Kin"),   QStringLiteral("Pot"),
                                     QStringLiteral("Therm"), QStringLiteral("Rad"),
                                     QStringLiteral("Total"), QStringLiteral("Drift")};
    const QFontMetricsF metrics = p.fontMetrics();
    const eg::Layout layout = eg::computeLayout(widget.rect(), metrics, shortLabels);
    
    p.setPen(palette.grid);
    p.drawLine(layout.energyRect.bottomLeft() + QPointF(0.0, 6.0),
               layout.energyRect.bottomRight() + QPointF(0.0, 6.0));
    p.drawRect(layout.energyRect);
    p.drawRect(layout.driftRect);
    
    const auto formatMetric = [](float value, const QString& suffix) {
        return QString::number(value, std::fabs(value) >= 1000.0f ? 'g' : 'f',
                               std::fabs(value) >= 100.0f ? 1 : 2) + suffix;
    };
    
    drawAxisLabels(p, layout, palette);
    drawLegend(p, layout, metrics, palette, shortLabels);
    drawHeader(p, layout, metrics, palette, history, formatMetric);
    
    if (history.size() < 2) {
        p.setPen(palette.label);
        p.drawText(layout.energyRect, Qt::AlignCenter, QStringLiteral("Awaiting energy telemetry"));
        return;
    }
    
    const eg::DataRange dataRange = eg::computeDataRange(history);
    drawCurves(p, layout, palette, dataRange);
    drawCurrentMarkers(p, layout, palette, *dataRange.latestPoint, dataRange);
    drawAxisValues(p, layout, palette, dataRange);
}

static void drawAxisLabels(QPainter& p, const eg::Layout& layout, const eg::ColorPalette& palette)
{
    p.setPen(palette.label);
    p.drawText(QRectF(layout.energyRect.left() + 6.0, layout.energyRect.top() + 2.0, 120.0, 14.0),
               Qt::AlignLeft | Qt::AlignVCenter, EnergyGraphWidget::energyYAxisLabel());
    p.drawText(QRectF(layout.driftRect.left() + 6.0, layout.driftRect.top() + 2.0, 100.0, 14.0),
               Qt::AlignLeft | Qt::AlignVCenter, EnergyGraphWidget::driftYAxisLabel());
}

static void drawLegend(QPainter& p, const eg::Layout& layout, const QFontMetricsF& metrics,
                       const eg::ColorPalette& palette, const QStringList& shortLabels)
{
    constexpr qreal legendInset = 2.0;
    constexpr qreal legendGapX = 10.0;
    constexpr qreal legendLineWidth = 12.0;
    constexpr qreal legendTextGap = 4.0;
    const qreal legendRowHeight = metrics.height() + 4.0;

    const std::array<QColor, 6> colors = {palette.kineticCurve, palette.potentialCurve, palette.thermalCurve,
                                          palette.radiatedCurve, palette.totalCurve, palette.driftCurve};
    qreal x = layout.legendRect.left() + legendInset;
    qreal y = layout.legendRect.top() + metrics.ascent();

    for (int i = 0; i < shortLabels.size() && i < static_cast<int>(colors.size()); ++i) {
        const QString& label = shortLabels.at(i);
        const qreal entryWidth = legendLineWidth + legendTextGap + metrics.horizontalAdvance(label) + legendGapX;
        if (x > layout.legendRect.left() + legendInset && x + entryWidth > layout.legendRect.right() - legendInset) {
            x = layout.legendRect.left() + legendInset;
            y += legendRowHeight;
        }
        p.setPen(QPen(colors.at(i), 2.0));
        p.drawLine(QPointF(x, y - metrics.ascent() * 0.35), QPointF(x + legendLineWidth, y - metrics.ascent() * 0.35));
        p.setPen(palette.label);
        p.drawText(QPointF(x + legendLineWidth + legendTextGap, y), label);
        x += entryWidth;
    }
}

static void drawHeader(QPainter& p, const eg::Layout& layout, const QFontMetricsF& metrics,
                       const eg::ColorPalette& palette, const std::vector<EnergyPoint>& history,
                       const std::function<QString(float, const QString&)>& formatMetric)
{
    p.setPen(palette.label);
    p.drawText(QRectF(layout.headerRect.left(), layout.headerRect.top(), layout.headerRect.width(),
                      layout.headerLineHeight),
               Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("Energy timeline"));

    if (history.empty()) {
        p.drawText(QRectF(layout.headerRect.left(), layout.headerRect.top() + layout.headerLineHeight,
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
    p.drawText(QRectF(layout.headerRect.left(), layout.headerRect.top() + layout.headerLineHeight,
                      layout.headerRect.width(), layout.headerLineHeight),
               Qt::AlignLeft | Qt::AlignVCenter, summary);
}

static void drawCurves(QPainter& p, const eg::Layout& layout, const eg::ColorPalette& palette,
                       const eg::DataRange& dataRange)
{
    const auto buildPath = [&](auto valueAccessor, const QRectF& targetRect, float vMin,
                               float vMax, bool centered) {
        QPainterPath path;
        for (std::size_t i = dataRange.visibleStart; i < dataRange.visibleStart + 100; ++i) { // Simplified
            // This is a placeholder; full implementation would iterate through history properly
        }
        return path;
    };

    p.setRenderHint(QPainter::Antialiasing, true);
    const auto faded = [](QColor color) {
        color.setAlpha(185);
        return color;
    };

    p.setPen(QPen(faded(palette.kineticCurve), 1.2));
    p.setPen(QPen(faded(palette.potentialCurve), 1.2));
    p.setPen(QPen(palette.totalCurve, 2.1));
    p.setPen(QPen(faded(palette.thermalCurve), 1.0));
    p.setPen(QPen(faded(palette.radiatedCurve), 1.0));

    if (dataRange.minEnergy < 0.0f && dataRange.maxEnergy > 0.0f) {
        const qreal zeroNorm = static_cast<qreal>((0.0f - dataRange.minEnergy) / (dataRange.maxEnergy - dataRange.minEnergy));
        const qreal zeroY = layout.energyRect.top() + layout.energyRect.height() * (1.0 - zeroNorm);
        p.setPen(palette.grid);
        p.drawLine(QPointF(layout.energyRect.left(), zeroY), QPointF(layout.energyRect.right(), zeroY));
    }

    p.setPen(palette.grid);
    p.drawLine(QPointF(layout.driftRect.left(), layout.driftRect.center().y()),
               QPointF(layout.driftRect.right(), layout.driftRect.center().y()));

    p.setPen(QPen(palette.driftCurve, 1.8));
    p.setRenderHint(QPainter::Antialiasing, false);
}

static void drawCurrentMarkers(QPainter& p, const eg::Layout& layout,
                               const eg::ColorPalette& palette, const EnergyPoint& latest,
                               const eg::DataRange& dataRange)
{
    const qreal latestTimeNorm = static_cast<qreal>((latest.time - dataRange.minTime) / (dataRange.maxTime - dataRange.minTime));
    const qreal latestX = layout.energyRect.left() + layout.energyRect.width() * std::clamp(latestTimeNorm, 0.0, 1.0);

    const qreal latestEnergyY = layout.energyRect.top() + layout.energyRect.height() * (1.0 - ((latest.total - dataRange.minEnergy) / (dataRange.maxEnergy - dataRange.minEnergy)));
    const qreal latestDriftY = layout.driftRect.top() + layout.driftRect.height() * (1.0 - ((latest.drift / dataRange.maxAbsDrift) * 0.5f + 0.5f));

    p.setPen(Qt::NoPen);
    p.setBrush(palette.totalCurve);
    p.drawEllipse(QPointF(latestX, latestEnergyY), 3.0, 3.0);
    p.setBrush(palette.driftCurve);
    p.drawEllipse(QPointF(latestX, latestDriftY), 3.0, 3.0);
}

static void drawAxisValues(QPainter& p, const eg::Layout& layout, const eg::ColorPalette& palette,
                           const eg::DataRange& dataRange)
{
    p.setPen(palette.label);
    p.drawText(QRectF(layout.outerRect.left(), layout.energyRect.top() - 2.0, 36.0, 14.0),
               Qt::AlignRight | Qt::AlignTop, QString::number(dataRange.maxEnergy, 'g', 4));
    p.drawText(QRectF(layout.outerRect.left(), layout.energyRect.bottom() - 12.0, 36.0, 14.0),
               Qt::AlignRight | Qt::AlignBottom, QString::number(dataRange.minEnergy, 'g', 4));
    p.drawText(QRectF(layout.outerRect.left(), layout.driftRect.top() - 2.0, 36.0, 14.0),
               Qt::AlignRight | Qt::AlignTop, QString::number(dataRange.maxAbsDrift, 'f', 2));
    p.drawText(QRectF(layout.outerRect.left(), layout.driftRect.bottom() - 12.0, 36.0, 14.0),
               Qt::AlignRight | Qt::AlignBottom, QString::number(-dataRange.maxAbsDrift, 'f', 2));
    p.drawText(QRectF(layout.driftRect.left(), layout.driftRect.bottom() + 4.0, 100.0, 14.0),
               Qt::AlignLeft | Qt::AlignVCenter, QString("%1 s").arg(dataRange.minTime, 0, 'f', 2));
    p.drawText(QRectF(layout.driftRect.right() - 100.0, layout.driftRect.bottom() + 4.0, 100.0, 14.0),
               Qt::AlignRight | Qt::AlignVCenter, QString("%1 s").arg(dataRange.maxTime, 0, 'f', 2));
    p.drawText(QRectF(layout.driftRect.left(), layout.driftRect.top() + 2.0, 180.0, 14.0),
               Qt::AlignLeft | Qt::AlignVCenter,
               QStringLiteral("Window %1 s").arg(QString::number(dataRange.maxTime - dataRange.minTime, 'f', 2)));
}
} // namespace bltzr_qt
