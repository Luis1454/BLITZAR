/*
 * @file modules/qt/ui/EnergyGraphWidgetPaint.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "ui/EnergyGraphWidgetPaint.hpp"
#include "ui/EnergyGraphWidget.hpp"
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPen>
#include <QRectF>
#include <QStringView>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace grav_qt {
static bool isDarkTheme(const QPalette& palette)
{
    return palette.color(QPalette::Window).lightness() < 128;
}

static QColor panelCurveColor(const QColor& darkColor, const QColor& lightColor, bool darkTheme)
{
    return darkTheme ? darkColor : lightColor;
}

void EnergyGraphWidgetPaint::paint(QWidget& widget, const std::vector<EnergyPoint>& history,
                                   UiPaintEvent* event)
{
    (void)event;
    QPainter p(&widget);
    const QPalette widgetPalette = widget.palette();
    const bool darkTheme = isDarkTheme(widgetPalette);
    const QColor background = widgetPalette.color(QPalette::Window);
    const QColor border = widgetPalette.color(QPalette::Mid);
    const QColor labelColor = widgetPalette.color(QPalette::WindowText);
    QColor gridColor = border;
    gridColor.setAlpha(darkTheme ? 190 : 120);
    const QColor kineticColor =
        panelCurveColor(QColor(92, 255, 140), QColor(0, 122, 52), darkTheme);
    const QColor potentialColor =
        panelCurveColor(QColor(255, 120, 108), QColor(180, 45, 35), darkTheme);
    const QColor thermalColor =
        panelCurveColor(QColor(255, 170, 90), QColor(176, 99, 10), darkTheme);
    const QColor radiatedColor =
        panelCurveColor(QColor(180, 120, 255), QColor(108, 52, 188), darkTheme);
    const QColor totalColor =
        panelCurveColor(QColor(120, 200, 255), QColor(0, 102, 170), darkTheme);
    const QColor driftColor =
        panelCurveColor(QColor(255, 230, 120), QColor(168, 132, 0), darkTheme);
    p.fillRect(widget.rect(), background);
    p.setPen(border);
    p.drawRect(widget.rect().adjusted(0, 0, -1, -1));
    const QRectF outerRect = widget.rect().adjusted(12, 10, -12, -10);
    const QFontMetricsF metrics = p.fontMetrics();
    const QStringList shortLabels = {QStringLiteral("Kin"),   QStringLiteral("Pot"),
                                     QStringLiteral("Therm"), QStringLiteral("Rad"),
                                     QStringLiteral("Total"), QStringLiteral("Drift")};
    const qreal headerLineHeight = metrics.height() + 2.0;
    const qreal headerHeight = headerLineHeight * 2.0 + 8.0;
    const qreal legendInset = 2.0;
    const qreal legendGapX = 10.0;
    const qreal legendLineWidth = 12.0;
    const qreal legendTextGap = 4.0;
    const qreal legendRowHeight = metrics.height() + 4.0;
    const qreal legendAvailableWidth =
        std::max<qreal>(120.0, outerRect.width() - legendInset * 2.0);
    int legendRows = 1;
    qreal legendRowWidth = 0.0;
    for (const QString& label : shortLabels) {
        const qreal entryWidth =
            legendLineWidth + legendTextGap + metrics.horizontalAdvance(label) + legendGapX;
        if (legendRowWidth > 0.0 && legendRowWidth + entryWidth > legendAvailableWidth) {
            legendRows += 1;
            legendRowWidth = 0.0;
        }
        legendRowWidth += entryWidth;
    }
    const qreal legendHeight = legendRows * legendRowHeight + 2.0;
    const QRectF plotRect(
        outerRect.left() + 40.0, outerRect.top() + headerHeight + legendHeight,
        std::max<qreal>(120.0, outerRect.width() - 48.0),
        std::max<qreal>(64.0, outerRect.height() - headerHeight - legendHeight - 18.0));
    const qreal splitY = plotRect.top() + plotRect.height() * 0.68;
    const QRectF energyRect(plotRect.left(), plotRect.top(), plotRect.width(),
                            splitY - plotRect.top() - 6.0);
    const QRectF driftRect(plotRect.left(), splitY + 6.0, plotRect.width(),
                           plotRect.bottom() - (splitY + 6.0));
    const QRectF headerRect(outerRect.left(), outerRect.top(), outerRect.width(), headerHeight);
    const QRectF legendRect(outerRect.left(), outerRect.top() + headerHeight, outerRect.width(),
                            legendHeight);
    p.setPen(gridColor);
    p.drawLine(QPointF(plotRect.left(), splitY), QPointF(plotRect.right(), splitY));
    p.drawRect(energyRect);
    p.drawRect(driftRect);
    const auto formatMetric = [](float value, const QString& suffix) {
        return QString::number(value, std::fabs(value) >= 1000.0f ? 'g' : 'f',
                               std::fabs(value) >= 100.0f ? 1 : 2) +
               suffix;
    };
    const auto drawAxisLabels = [&]() {
        p.setPen(labelColor);
        p.drawText(QRectF(energyRect.left() + 6.0, energyRect.top() + 2.0, 120.0, 14.0),
                   Qt::AlignLeft | Qt::AlignVCenter, EnergyGraphWidget::energyYAxisLabel());
        p.drawText(QRectF(driftRect.left() + 6.0, driftRect.top() + 2.0, 100.0, 14.0),
                   Qt::AlignLeft | Qt::AlignVCenter, EnergyGraphWidget::driftYAxisLabel());
    };
    const auto drawLegend = [&]() {
        const std::array<QColor, 6> colors = {kineticColor,  potentialColor, thermalColor,
                                              radiatedColor, totalColor,     driftColor};
        qreal x = legendRect.left() + legendInset;
        qreal y = legendRect.top() + metrics.ascent();
        for (int i = 0; i < shortLabels.size() && i < static_cast<int>(colors.size()); ++i) {
            const QString& label = shortLabels.at(i);
            const qreal entryWidth =
                legendLineWidth + legendTextGap + metrics.horizontalAdvance(label) + legendGapX;
            if (x > legendRect.left() + legendInset &&
                x + entryWidth > legendRect.right() - legendInset) {
                x = legendRect.left() + legendInset;
                y += legendRowHeight;
            }
            p.setPen(QPen(colors.at(i), 2.0));
            p.drawLine(QPointF(x, y - metrics.ascent() * 0.35),
                       QPointF(x + legendLineWidth, y - metrics.ascent() * 0.35));
            p.setPen(labelColor);
            p.drawText(QPointF(x + legendLineWidth + legendTextGap, y), label);
            x += entryWidth;
        }
    };
    const auto drawHeader = [&]() {
        p.setPen(labelColor);
        p.drawText(
            QRectF(headerRect.left(), headerRect.top(), headerRect.width(), headerLineHeight),
            Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("Energy timeline"));
        if (history.empty()) {
            p.drawText(QRectF(headerRect.left(), headerRect.top() + headerLineHeight,
                              headerRect.width(), headerLineHeight),
                       Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("Waiting for telemetry"));
            return;
        }
        const EnergyPoint& latest = history.back();
        const QString summary = QStringLiteral("Total %1    Drift %2    Time %3    Samples %4")
                                    .arg(formatMetric(latest.total, QStringLiteral(" J")))
                                    .arg(formatMetric(latest.drift, QStringLiteral("%")))
                                    .arg(formatMetric(latest.time, QStringLiteral(" s")))
                                    .arg(history.size());
        p.drawText(QRectF(headerRect.left(), headerRect.top() + headerLineHeight,
                          headerRect.width(), headerLineHeight),
                   Qt::AlignLeft | Qt::AlignVCenter, summary);
    };
    drawHeader();
    drawAxisLabels();
    drawLegend();
    if (history.size() < 2) {
        p.setPen(labelColor);
        p.drawText(energyRect, Qt::AlignCenter, QStringLiteral("Awaiting energy telemetry"));
        return;
    }
    constexpr std::size_t visibleSampleLimit = 160u;
    constexpr float visibleTimeSpanSec = 12.0f;
    std::size_t visibleStart =
        (history.size() > visibleSampleLimit) ? (history.size() - visibleSampleLimit) : 0u;
    const EnergyPoint& latest = history.back();
    for (std::size_t i = history.size(); i > 0u; --i) {
        const std::size_t index = i - 1u;
        if (latest.time - history[index].time > visibleTimeSpanSec && index >= visibleStart) {
            visibleStart = index + 1u;
            break;
        }
    }
    if (history.size() - visibleStart < 2u) {
        visibleStart = history.size() >= 2u ? (history.size() - 2u) : 0u;
    }
    float minEnergy = std::numeric_limits<float>::infinity();
    float maxEnergy = -std::numeric_limits<float>::infinity();
    float maxAbsDrift = 0.01f;
    float minTime = std::numeric_limits<float>::infinity();
    float maxTime = -std::numeric_limits<float>::infinity();
    for (std::size_t i = visibleStart; i < history.size(); ++i) {
        const EnergyPoint& sample = history[i];
        minEnergy = std::min(
            minEnergy, std::min(std::min(sample.kinetic, sample.potential),
                                std::min(sample.thermal, std::min(sample.radiated, sample.total))));
        maxEnergy = std::max(
            maxEnergy, std::max(std::max(sample.kinetic, sample.potential),
                                std::max(sample.thermal, std::max(sample.radiated, sample.total))));
        maxAbsDrift = std::max(maxAbsDrift, std::fabs(sample.drift));
        minTime = std::min(minTime, sample.time);
        maxTime = std::max(maxTime, sample.time);
    }
    if (maxEnergy <= minEnergy + 1e-9f)
        maxEnergy = minEnergy + 1.0f;
    if (maxTime <= minTime + 1e-6f)
        maxTime = minTime + 1.0f;
    const auto buildPath = [&](auto valueAccessor, const QRectF& targetRect, float vMin, float vMax,
                               bool centered) {
        QPainterPath path;
        for (std::size_t i = visibleStart; i < history.size(); ++i) {
            const qreal timeNorm =
                static_cast<qreal>((history[i].time - minTime) / (maxTime - minTime));
            const qreal x = targetRect.left() + targetRect.width() * std::clamp(timeNorm, 0.0, 1.0);
            const float value = valueAccessor(history[i]);
            qreal y = 0.0;
            if (centered) {
                const float norm = value / vMax;
                y = targetRect.top() + targetRect.height() * (1.0 - (norm * 0.5f + 0.5f));
            }
            else {
                const float norm = (value - vMin) / (vMax - vMin);
                y = targetRect.top() + targetRect.height() * (1.0 - norm);
            }
            if (i == visibleStart) {
                path.moveTo(x, y);
            }
            else {
                path.lineTo(x, y);
            }
        }
        return path;
    };
    p.setRenderHint(QPainter::Antialiasing, true);
    const auto faded = [](QColor color) {
        color.setAlpha(185);
        return color;
    };
    p.setPen(QPen(faded(kineticColor), 1.2));
    p.drawPath(buildPath(
        [](const EnergyPoint& point) {
            return point.kinetic;
        },
        energyRect, minEnergy, maxEnergy, false));
    p.setPen(QPen(faded(potentialColor), 1.2));
    p.drawPath(buildPath(
        [](const EnergyPoint& point) {
            return point.potential;
        },
        energyRect, minEnergy, maxEnergy, false));
    p.setPen(QPen(totalColor, 2.1));
    p.drawPath(buildPath(
        [](const EnergyPoint& point) {
            return point.total;
        },
        energyRect, minEnergy, maxEnergy, false));
    p.setPen(QPen(faded(thermalColor), 1.0));
    p.drawPath(buildPath(
        [](const EnergyPoint& point) {
            return point.thermal;
        },
        energyRect, minEnergy, maxEnergy, false));
    p.setPen(QPen(faded(radiatedColor), 1.0));
    p.drawPath(buildPath(
        [](const EnergyPoint& point) {
            return point.radiated;
        },
        energyRect, minEnergy, maxEnergy, false));
    p.setPen(gridColor);
    if (minEnergy < 0.0f && maxEnergy > 0.0f) {
        const qreal zeroNorm = static_cast<qreal>((0.0f - minEnergy) / (maxEnergy - minEnergy));
        const qreal zeroY = energyRect.top() + energyRect.height() * (1.0 - zeroNorm);
        p.drawLine(QPointF(energyRect.left(), zeroY), QPointF(energyRect.right(), zeroY));
    }
    p.drawLine(QPointF(driftRect.left(), driftRect.center().y()),
               QPointF(driftRect.right(), driftRect.center().y()));
    p.setPen(QPen(driftColor, 1.8));
    p.drawPath(buildPath(
        [](const EnergyPoint& point) {
            return point.drift;
        },
        driftRect, -maxAbsDrift, maxAbsDrift, true));
    const qreal latestTimeNorm = static_cast<qreal>((latest.time - minTime) / (maxTime - minTime));
    const qreal latestEnergyY =
        energyRect.top() +
        energyRect.height() * (1.0 - ((latest.total - minEnergy) / (maxEnergy - minEnergy)));
    const qreal latestDriftY =
        driftRect.top() + driftRect.height() * (1.0 - ((latest.drift / maxAbsDrift) * 0.5f + 0.5f));
    const qreal latestX =
        energyRect.left() + energyRect.width() * std::clamp(latestTimeNorm, 0.0, 1.0);
    p.setPen(Qt::NoPen);
    p.setBrush(totalColor);
    p.drawEllipse(QPointF(latestX, latestEnergyY), 3.0, 3.0);
    p.setBrush(driftColor);
    p.drawEllipse(QPointF(latestX, latestDriftY), 3.0, 3.0);
    p.setPen(labelColor);
    p.drawText(QRectF(outerRect.left(), energyRect.top() - 2.0, 36.0, 14.0),
               Qt::AlignRight | Qt::AlignTop, QString::number(maxEnergy, 'g', 4));
    p.drawText(QRectF(outerRect.left(), energyRect.bottom() - 12.0, 36.0, 14.0),
               Qt::AlignRight | Qt::AlignBottom, QString::number(minEnergy, 'g', 4));
    p.drawText(QRectF(outerRect.left(), driftRect.top() - 2.0, 36.0, 14.0),
               Qt::AlignRight | Qt::AlignTop, QString::number(maxAbsDrift, 'f', 2));
    p.drawText(QRectF(outerRect.left(), driftRect.bottom() - 12.0, 36.0, 14.0),
               Qt::AlignRight | Qt::AlignBottom, QString::number(-maxAbsDrift, 'f', 2));
    p.drawText(QRectF(driftRect.left(), driftRect.bottom() + 4.0, 100.0, 14.0),
               Qt::AlignLeft | Qt::AlignVCenter, QString("%1 s").arg(minTime, 0, 'f', 2));
    p.drawText(QRectF(driftRect.right() - 100.0, driftRect.bottom() + 4.0, 100.0, 14.0),
               Qt::AlignRight | Qt::AlignVCenter, QString("%1 s").arg(maxTime, 0, 'f', 2));
    p.drawText(QRectF(energyRect.right() - 180.0, energyRect.top() + 2.0, 176.0, 14.0),
               Qt::AlignRight | Qt::AlignVCenter,
               QStringLiteral("Current %1").arg(formatMetric(latest.total, QStringLiteral(" J"))));
    p.drawText(QRectF(driftRect.right() - 180.0, driftRect.top() + 2.0, 176.0, 14.0),
               Qt::AlignRight | Qt::AlignVCenter,
               QStringLiteral("Current %1").arg(formatMetric(latest.drift, QStringLiteral("%"))));
    p.drawText(QRectF(driftRect.left(), driftRect.top() + 2.0, 180.0, 14.0),
               Qt::AlignLeft | Qt::AlignVCenter,
               QStringLiteral("Window %1 s").arg(QString::number(maxTime - minTime, 'f', 2)));
    p.setRenderHint(QPainter::Antialiasing, false);
}
} // namespace grav_qt
