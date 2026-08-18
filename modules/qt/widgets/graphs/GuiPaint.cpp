/*
 * @file modules/qt/widgets/graphs/GuiPaint.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "widgets/graphs/GuiPaint.hpp"
#include "widgets/graphs/GuiGraph.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPen>
#include <QStringList>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace bltzr_qt {
struct GraphPaintColors {
    QColor background;
    QColor border;
    QColor grid;
    QColor label;
    QColor kinetic;
    QColor potential;
    QColor thermal;
    QColor radiated;
    QColor total;
    QColor drift;
};

struct GraphPaintLayout {
    QRectF outer;
    QRectF header;
    QRectF legend;
    QRectF energy;
    QRectF drift;
    qreal headerLineHeight = 0.0;
    qreal legendRowHeight = 0.0;
};

struct GraphPaintBounds {
    std::size_t start = 0u;
    float minEnergy = 0.0f;
    float maxEnergy = 1.0f;
    float maxAbsDrift = 0.01f;
    float minTime = 0.0f;
    float maxTime = 1.0f;
};

static bool isDarkTheme(const QPalette& palette)
{
    return palette.color(QPalette::Window).lightness() < 128;
}

static QColor panelCurveColor(const QColor& darkColor, const QColor& lightColor, bool darkTheme)
{
    return darkTheme ? darkColor : lightColor;
}

static GraphPaintColors resolveColors(const QPalette& palette)
{
    const bool darkTheme = isDarkTheme(palette);
    GraphPaintColors colors;
    colors.background = palette.color(QPalette::Window);
    colors.border = palette.color(QPalette::Mid);
    colors.grid = colors.border;
    colors.grid.setAlpha(darkTheme ? 190 : 120);
    colors.label = palette.color(QPalette::WindowText);
    colors.kinetic = panelCurveColor(QColor(92, 255, 140), QColor(0, 122, 52), darkTheme);
    colors.potential = panelCurveColor(QColor(255, 120, 108), QColor(180, 45, 35), darkTheme);
    colors.thermal = panelCurveColor(QColor(255, 170, 90), QColor(176, 99, 10), darkTheme);
    colors.radiated = panelCurveColor(QColor(180, 120, 255), QColor(108, 52, 188), darkTheme);
    colors.total = panelCurveColor(QColor(120, 200, 255), QColor(0, 102, 170), darkTheme);
    colors.drift = panelCurveColor(QColor(255, 230, 120), QColor(168, 132, 0), darkTheme);
    return colors;
}

static GraphPaintLayout createLayout(const QRect& widgetRect, const QFontMetricsF& metrics,
                                     const QStringList& labels)
{
    GraphPaintLayout layout;
    layout.outer = widgetRect.adjusted(12, 10, -12, -10);
    layout.headerLineHeight = metrics.height() + 2.0;
    const qreal headerHeight = layout.headerLineHeight * 2.0 + 8.0;
    constexpr qreal legendInset = 2.0;
    constexpr qreal legendGapX = 10.0;
    constexpr qreal legendLineWidth = 12.0;
    constexpr qreal legendTextGap = 4.0;
    layout.legendRowHeight = metrics.height() + 4.0;
    const qreal availableWidth = std::max<qreal>(120.0, layout.outer.width() - legendInset * 2.0);
    int legendRows = 1;
    qreal rowWidth = 0.0;
    for (const QString& label : labels) {
        const qreal entryWidth = legendLineWidth + legendTextGap + metrics.horizontalAdvance(label) +
                                 legendGapX;
        if (rowWidth > 0.0 && rowWidth + entryWidth > availableWidth) {
            ++legendRows;
            rowWidth = 0.0;
        }
        rowWidth += entryWidth;
    }
    const qreal legendHeight = legendRows * layout.legendRowHeight + 2.0;
    const QRectF plot(
        layout.outer.left() + 40.0, layout.outer.top() + headerHeight + legendHeight,
        std::max<qreal>(120.0, layout.outer.width() - 48.0),
        std::max<qreal>(64.0, layout.outer.height() - headerHeight - legendHeight - 18.0));
    const qreal splitY = plot.top() + plot.height() * 0.68;
    layout.energy = QRectF(plot.left(), plot.top(), plot.width(), splitY - plot.top() - 6.0);
    layout.drift = QRectF(plot.left(), splitY + 6.0, plot.width(), plot.bottom() - splitY - 6.0);
    layout.header = QRectF(layout.outer.left(), layout.outer.top(), layout.outer.width(), headerHeight);
    layout.legend = QRectF(layout.outer.left(), layout.outer.top() + headerHeight,
                           layout.outer.width(), legendHeight);
    return layout;
}

static QString formatMetric(float value, const QString& suffix)
{
    return QString::number(value, std::fabs(value) >= 1000.0f ? 'g' : 'f',
                            std::fabs(value) >= 100.0f ? 1 : 2) + suffix;
}

static void drawFrame(QPainter& painter, const QRect& widgetRect, const GraphPaintLayout& layout,
                      const GraphPaintColors& colors)
{
    painter.fillRect(widgetRect, colors.background);
    painter.setPen(colors.border);
    painter.drawRect(widgetRect.adjusted(0, 0, -1, -1));
    painter.setPen(colors.grid);
    painter.drawLine(QPointF(layout.energy.left(), layout.energy.bottom() + 6.0),
                     QPointF(layout.energy.right(), layout.energy.bottom() + 6.0));
    painter.drawRect(layout.energy);
    painter.drawRect(layout.drift);
}

static void drawHeader(QPainter& painter, const GraphPaintLayout& layout,
                       const GraphPaintColors& colors, const std::vector<EnergyPoint>& history)
{
    painter.setPen(colors.label);
    painter.drawText(QRectF(layout.header.left(), layout.header.top(), layout.header.width(),
                            layout.headerLineHeight),
                     Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("Energy timeline"));
    const QRectF summaryRect(layout.header.left(), layout.header.top() + layout.headerLineHeight,
                             layout.header.width(), layout.headerLineHeight);
    if (history.empty()) {
        painter.drawText(summaryRect, Qt::AlignLeft | Qt::AlignVCenter,
                         QStringLiteral("Waiting for telemetry"));
        return;
    }
    const EnergyPoint& latest = history.back();
    const QString summary = QStringLiteral("Total %1    Drift %2    Time %3    Samples %4")
                                .arg(formatMetric(latest.total, QStringLiteral(" J")))
                                .arg(formatMetric(latest.drift, QStringLiteral("%")))
                                .arg(formatMetric(latest.time, QStringLiteral(" s")))
                                .arg(history.size());
    painter.drawText(summaryRect, Qt::AlignLeft | Qt::AlignVCenter, summary);
}

static void drawAxisLabels(QPainter& painter, const GraphPaintLayout& layout,
                           const GraphPaintColors& colors)
{
    painter.setPen(colors.label);
    painter.drawText(QRectF(layout.energy.left() + 6.0, layout.energy.top() + 2.0, 120.0, 14.0),
                     Qt::AlignLeft | Qt::AlignVCenter, Graph::energyYAxisLabel());
    painter.drawText(QRectF(layout.drift.left() + 6.0, layout.drift.top() + 2.0, 100.0, 14.0),
                     Qt::AlignLeft | Qt::AlignVCenter, Graph::driftYAxisLabel());
}

static void drawLegend(QPainter& painter, const GraphPaintLayout& layout,
                       const GraphPaintColors& colors, const QStringList& labels,
                       const QFontMetricsF& metrics)
{
    constexpr qreal inset = 2.0;
    constexpr qreal gapX = 10.0;
    constexpr qreal lineWidth = 12.0;
    constexpr qreal textGap = 4.0;
    const std::array<QColor, 6> curveColors = {colors.kinetic, colors.potential, colors.thermal,
                                                colors.radiated, colors.total, colors.drift};
    qreal x = layout.legend.left() + inset;
    qreal y = layout.legend.top() + metrics.ascent();
    for (int index = 0; index < labels.size() && index < static_cast<int>(curveColors.size());
         ++index) {
        const QString& label = labels.at(index);
        const qreal entryWidth = lineWidth + textGap + metrics.horizontalAdvance(label) + gapX;
        if (x > layout.legend.left() + inset &&
            x + entryWidth > layout.legend.right() - inset) {
            x = layout.legend.left() + inset;
            y += layout.legendRowHeight;
        }
        painter.setPen(QPen(curveColors.at(index), 2.0));
        painter.drawLine(QPointF(x, y - metrics.ascent() * 0.35),
                         QPointF(x + lineWidth, y - metrics.ascent() * 0.35));
        painter.setPen(colors.label);
        painter.drawText(QPointF(x + lineWidth + textGap, y), label);
        x += entryWidth;
    }
}

static std::size_t visibleStart(const std::vector<EnergyPoint>& history)
{
    constexpr std::size_t sampleLimit = 160u;
    constexpr float timeSpanSec = 12.0f;
    std::size_t start = history.size() > sampleLimit ? history.size() - sampleLimit : 0u;
    const float latestTime = history.back().time;
    for (std::size_t index = history.size(); index > 0u; --index) {
        const std::size_t candidate = index - 1u;
        if (latestTime - history[candidate].time > timeSpanSec && candidate >= start) {
            start = candidate + 1u;
            break;
        }
    }
    return history.size() - start < 2u ? history.size() - 2u : start;
}

static GraphPaintBounds calculateBounds(const std::vector<EnergyPoint>& history)
{
    GraphPaintBounds bounds;
    bounds.start = visibleStart(history);
    bounds.minEnergy = std::numeric_limits<float>::infinity();
    bounds.maxEnergy = -std::numeric_limits<float>::infinity();
    bounds.minTime = std::numeric_limits<float>::infinity();
    bounds.maxTime = -std::numeric_limits<float>::infinity();
    for (std::size_t index = bounds.start; index < history.size(); ++index) {
        const EnergyPoint& sample = history[index];
        bounds.minEnergy = std::min(
            bounds.minEnergy, std::min(std::min(sample.kinetic, sample.potential),
                                       std::min(sample.thermal, std::min(sample.radiated, sample.total))));
        bounds.maxEnergy = std::max(
            bounds.maxEnergy, std::max(std::max(sample.kinetic, sample.potential),
                                       std::max(sample.thermal, std::max(sample.radiated, sample.total))));
        bounds.maxAbsDrift = std::max(bounds.maxAbsDrift, std::fabs(sample.drift));
        bounds.minTime = std::min(bounds.minTime, sample.time);
        bounds.maxTime = std::max(bounds.maxTime, sample.time);
    }
    if (bounds.maxEnergy <= bounds.minEnergy + 1e-9f)
        bounds.maxEnergy = bounds.minEnergy + 1.0f;
    if (bounds.maxTime <= bounds.minTime + 1e-6f)
        bounds.maxTime = bounds.minTime + 1.0f;
    return bounds;
}

template <typename ValueAccessor>
static QPainterPath buildPath(const std::vector<EnergyPoint>& history, const QRectF& targetRect,
                              const GraphPaintBounds& bounds, ValueAccessor valueAccessor,
                              float valueMin, float valueMax, bool centered)
{
    QPainterPath path;
    for (std::size_t index = bounds.start; index < history.size(); ++index) {
        const EnergyPoint& sample = history[index];
        const qreal timeNorm = static_cast<qreal>((sample.time - bounds.minTime) /
                                                  (bounds.maxTime - bounds.minTime));
        const qreal x = targetRect.left() + targetRect.width() * std::clamp(timeNorm, 0.0, 1.0);
        const float value = valueAccessor(sample);
        const float normalized = centered ? value / valueMax : (value - valueMin) / (valueMax - valueMin);
        const qreal y = targetRect.top() + targetRect.height() *
                        (1.0 - (centered ? normalized * 0.5f + 0.5f : normalized));
        if (index == bounds.start)
            path.moveTo(x, y);
        else
            path.lineTo(x, y);
    }
    return path;
}

static QColor faded(QColor color)
{
    color.setAlpha(185);
    return color;
}

static void drawEnergyCurves(QPainter& painter, const GraphPaintLayout& layout,
                             const GraphPaintColors& colors,
                             const std::vector<EnergyPoint>& history,
                             const GraphPaintBounds& bounds)
{
    const auto drawCurve = [&](const QColor& color, qreal width, auto accessor) {
        painter.setPen(QPen(color, width));
        painter.drawPath(buildPath(history, layout.energy, bounds, accessor, bounds.minEnergy,
                                   bounds.maxEnergy, false));
    };
    drawCurve(faded(colors.kinetic), 1.2, [](const EnergyPoint& point) { return point.kinetic; });
    drawCurve(faded(colors.potential), 1.2,
              [](const EnergyPoint& point) { return point.potential; });
    drawCurve(colors.total, 2.1, [](const EnergyPoint& point) { return point.total; });
    drawCurve(faded(colors.thermal), 1.0, [](const EnergyPoint& point) { return point.thermal; });
    drawCurve(faded(colors.radiated), 1.0,
              [](const EnergyPoint& point) { return point.radiated; });
    if (bounds.minEnergy < 0.0f && bounds.maxEnergy > 0.0f) {
        const qreal zeroNorm = static_cast<qreal>((0.0f - bounds.minEnergy) /
                                                  (bounds.maxEnergy - bounds.minEnergy));
        const qreal zeroY = layout.energy.top() + layout.energy.height() * (1.0 - zeroNorm);
        painter.setPen(colors.grid);
        painter.drawLine(QPointF(layout.energy.left(), zeroY),
                         QPointF(layout.energy.right(), zeroY));
    }
}

static void drawDriftCurve(QPainter& painter, const GraphPaintLayout& layout,
                           const GraphPaintColors& colors,
                           const std::vector<EnergyPoint>& history,
                           const GraphPaintBounds& bounds)
{
    painter.setPen(colors.grid);
    painter.drawLine(QPointF(layout.drift.left(), layout.drift.center().y()),
                     QPointF(layout.drift.right(), layout.drift.center().y()));
    painter.setPen(QPen(colors.drift, 1.8));
    painter.drawPath(buildPath(history, layout.drift, bounds,
                               [](const EnergyPoint& point) { return point.drift; },
                               -bounds.maxAbsDrift, bounds.maxAbsDrift, true));
}

static void drawCurrentMarkers(QPainter& painter, const GraphPaintLayout& layout,
                               const GraphPaintColors& colors, const EnergyPoint& latest,
                               const GraphPaintBounds& bounds)
{
    const qreal timeNorm = static_cast<qreal>((latest.time - bounds.minTime) /
                                              (bounds.maxTime - bounds.minTime));
    const qreal x = layout.energy.left() + layout.energy.width() * std::clamp(timeNorm, 0.0, 1.0);
    const qreal energyNorm = static_cast<qreal>((latest.total - bounds.minEnergy) /
                                                (bounds.maxEnergy - bounds.minEnergy));
    const qreal energyY = layout.energy.top() + layout.energy.height() * (1.0 - energyNorm);
    const qreal driftNorm = (latest.drift / bounds.maxAbsDrift) * 0.5 + 0.5;
    const qreal driftY = layout.drift.top() + layout.drift.height() * (1.0 - driftNorm);
    painter.setPen(Qt::NoPen);
    painter.setBrush(colors.total);
    painter.drawEllipse(QPointF(x, energyY), 3.0, 3.0);
    painter.setBrush(colors.drift);
    painter.drawEllipse(QPointF(x, driftY), 3.0, 3.0);
}

static void drawScaleLabels(QPainter& painter, const GraphPaintLayout& layout,
                            const GraphPaintColors& colors, const GraphPaintBounds& bounds,
                            const EnergyPoint& latest)
{
    painter.setPen(colors.label);
    painter.drawText(QRectF(layout.outer.left(), layout.energy.top() - 2.0, 36.0, 14.0),
                     Qt::AlignRight | Qt::AlignTop, QString::number(bounds.maxEnergy, 'g', 4));
    painter.drawText(QRectF(layout.outer.left(), layout.energy.bottom() - 12.0, 36.0, 14.0),
                     Qt::AlignRight | Qt::AlignBottom, QString::number(bounds.minEnergy, 'g', 4));
    painter.drawText(QRectF(layout.outer.left(), layout.drift.top() - 2.0, 36.0, 14.0),
                     Qt::AlignRight | Qt::AlignTop, QString::number(bounds.maxAbsDrift, 'f', 2));
    painter.drawText(QRectF(layout.outer.left(), layout.drift.bottom() - 12.0, 36.0, 14.0),
                     Qt::AlignRight | Qt::AlignBottom, QString::number(-bounds.maxAbsDrift, 'f', 2));
    painter.drawText(QRectF(layout.drift.left(), layout.drift.bottom() + 4.0, 100.0, 14.0),
                     Qt::AlignLeft | Qt::AlignVCenter, QString("%1 s").arg(bounds.minTime, 0, 'f', 2));
    painter.drawText(QRectF(layout.drift.right() - 100.0, layout.drift.bottom() + 4.0, 100.0, 14.0),
                     Qt::AlignRight | Qt::AlignVCenter, QString("%1 s").arg(bounds.maxTime, 0, 'f', 2));
    painter.drawText(QRectF(layout.energy.right() - 180.0, layout.energy.top() + 2.0, 176.0, 14.0),
                     Qt::AlignRight | Qt::AlignVCenter,
                     QStringLiteral("Current %1").arg(formatMetric(latest.total, QStringLiteral(" J"))));
    painter.drawText(QRectF(layout.drift.right() - 180.0, layout.drift.top() + 2.0, 176.0, 14.0),
                     Qt::AlignRight | Qt::AlignVCenter,
                     QStringLiteral("Current %1").arg(formatMetric(latest.drift, QStringLiteral("%"))));
    painter.drawText(QRectF(layout.drift.left(), layout.drift.top() + 2.0, 180.0, 14.0),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("Window %1 s").arg(QString::number(bounds.maxTime - bounds.minTime,
                                                                         'f', 2)));
}

void paintGraph(QWidget& widget, const std::vector<EnergyPoint>& history, UiPaintEvent* event)
{
    (void)event;
    QPainter painter(&widget);
    const GraphPaintColors colors = resolveColors(widget.palette());
    const QFontMetricsF metrics = painter.fontMetrics();
    const QStringList labels = {QStringLiteral("Kin"), QStringLiteral("Pot"), QStringLiteral("Therm"),
                                QStringLiteral("Rad"), QStringLiteral("Total"), QStringLiteral("Drift")};
    const GraphPaintLayout layout = createLayout(widget.rect(), metrics, labels);
    drawFrame(painter, widget.rect(), layout, colors);
    drawHeader(painter, layout, colors, history);
    drawAxisLabels(painter, layout, colors);
    drawLegend(painter, layout, colors, labels, metrics);
    if (history.size() < 2u) {
        painter.setPen(colors.label);
        painter.drawText(layout.energy, Qt::AlignCenter, QStringLiteral("Waiting for energy telemetry"));
        return;
    }
    const GraphPaintBounds bounds = calculateBounds(history);
    painter.setRenderHint(QPainter::Antialiasing, true);
    drawEnergyCurves(painter, layout, colors, history, bounds);
    drawDriftCurve(painter, layout, colors, history, bounds);
    drawCurrentMarkers(painter, layout, colors, history.back(), bounds);
    drawScaleLabels(painter, layout, colors, bounds, history.back());
    painter.setRenderHint(QPainter::Antialiasing, false);
}
} // namespace bltzr_qt
