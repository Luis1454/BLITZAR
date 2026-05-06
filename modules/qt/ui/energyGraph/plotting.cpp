/*
 * @file modules/qt/ui/energyGraph/plotting.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Plot rendering helpers for energy graph widget painting.
 */

#include "ui/energyGraph/plotting.hpp"

#include <QPainterPath>
#include <QPointF>
#include <algorithm>
#include <cmath>

namespace bltzr_qt::energyGraph {

template <typename ValueAccessor>
static QPainterPath buildPath(const std::vector<EnergyPoint>& history, const DataRange& dataRange,
                              const QRectF& targetRect, ValueAccessor valueAccessor, float vMin,
                              float vMax, bool centered)
{
    QPainterPath path;
    for (std::size_t i = dataRange.visibleStart; i < history.size(); ++i) {
        const qreal timeNorm = static_cast<qreal>((history[i].time - dataRange.minTime) /
                                                  (dataRange.maxTime - dataRange.minTime));
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
        if (i == dataRange.visibleStart) {
            path.moveTo(x, y);
        }
        else {
            path.lineTo(x, y);
        }
    }
    return path;
}

void drawCurves(QPainter& painter, const Layout& layout, const ColorPalette& palette,
                const std::vector<EnergyPoint>& history, const DataRange& dataRange)
{
    painter.setRenderHint(QPainter::Antialiasing, true);

    const auto faded = [](QColor color) {
        color.setAlpha(185);
        return color;
    };

    painter.setPen(QPen(faded(palette.kineticCurve), 1.2));
    painter.drawPath(buildPath(history, dataRange, layout.energyRect,
                               [](const EnergyPoint& point) { return point.kinetic; },
                               dataRange.minEnergy, dataRange.maxEnergy, false));

    painter.setPen(QPen(faded(palette.potentialCurve), 1.2));
    painter.drawPath(buildPath(history, dataRange, layout.energyRect,
                               [](const EnergyPoint& point) { return point.potential; },
                               dataRange.minEnergy, dataRange.maxEnergy, false));

    painter.setPen(QPen(palette.totalCurve, 2.1));
    painter.drawPath(buildPath(history, dataRange, layout.energyRect,
                               [](const EnergyPoint& point) { return point.total; },
                               dataRange.minEnergy, dataRange.maxEnergy, false));

    painter.setPen(QPen(faded(palette.thermalCurve), 1.0));
    painter.drawPath(buildPath(history, dataRange, layout.energyRect,
                               [](const EnergyPoint& point) { return point.thermal; },
                               dataRange.minEnergy, dataRange.maxEnergy, false));

    painter.setPen(QPen(faded(palette.radiatedCurve), 1.0));
    painter.drawPath(buildPath(history, dataRange, layout.energyRect,
                               [](const EnergyPoint& point) { return point.radiated; },
                               dataRange.minEnergy, dataRange.maxEnergy, false));

    if (dataRange.minEnergy < 0.0f && dataRange.maxEnergy > 0.0f) {
        const qreal zeroNorm = static_cast<qreal>((0.0f - dataRange.minEnergy) /
                                                  (dataRange.maxEnergy - dataRange.minEnergy));
        const qreal zeroY = layout.energyRect.top() + layout.energyRect.height() * (1.0 - zeroNorm);
        painter.setPen(palette.grid);
        painter.drawLine(QPointF(layout.energyRect.left(), zeroY),
                         QPointF(layout.energyRect.right(), zeroY));
    }

    painter.setPen(palette.grid);
    painter.drawLine(QPointF(layout.driftRect.left(), layout.driftRect.center().y()),
                     QPointF(layout.driftRect.right(), layout.driftRect.center().y()));

    painter.setPen(QPen(palette.driftCurve, 1.8));
    painter.drawPath(buildPath(history, dataRange, layout.driftRect,
                               [](const EnergyPoint& point) { return point.drift; },
                               -dataRange.maxAbsDrift, dataRange.maxAbsDrift, true));
    painter.setRenderHint(QPainter::Antialiasing, false);
}

void drawCurrentMarkers(QPainter& painter, const Layout& layout, const ColorPalette& palette,
                        const EnergyPoint& latest, const DataRange& dataRange)
{
    const qreal latestTimeNorm = static_cast<qreal>((latest.time - dataRange.minTime) /
                                                    (dataRange.maxTime - dataRange.minTime));
    const qreal latestX = layout.energyRect.left() + layout.energyRect.width() *
                          std::clamp(latestTimeNorm, 0.0, 1.0);
    const qreal latestEnergyY = layout.energyRect.top() + layout.energyRect.height() *
                                (1.0 - ((latest.total - dataRange.minEnergy) /
                                        (dataRange.maxEnergy - dataRange.minEnergy)));
    const qreal latestDriftY = layout.driftRect.top() + layout.driftRect.height() *
                               (1.0 - ((latest.drift / dataRange.maxAbsDrift) * 0.5f + 0.5f));

    painter.setPen(Qt::NoPen);
    painter.setBrush(palette.totalCurve);
    painter.drawEllipse(QPointF(latestX, latestEnergyY), 3.0, 3.0);
    painter.setBrush(palette.driftCurve);
    painter.drawEllipse(QPointF(latestX, latestDriftY), 3.0, 3.0);
}

void drawAxisValues(QPainter& painter, const Layout& layout, const ColorPalette& palette,
                    const DataRange& dataRange)
{
    painter.setPen(palette.label);
    painter.drawText(QRectF(layout.outerRect.left(), layout.energyRect.top() - 2.0, 36.0, 14.0),
                     Qt::AlignRight | Qt::AlignTop, QString::number(dataRange.maxEnergy, 'g', 4));
    painter.drawText(QRectF(layout.outerRect.left(), layout.energyRect.bottom() - 12.0, 36.0, 14.0),
                     Qt::AlignRight | Qt::AlignBottom, QString::number(dataRange.minEnergy, 'g', 4));
    painter.drawText(QRectF(layout.outerRect.left(), layout.driftRect.top() - 2.0, 36.0, 14.0),
                     Qt::AlignRight | Qt::AlignTop, QString::number(dataRange.maxAbsDrift, 'f', 2));
    painter.drawText(QRectF(layout.outerRect.left(), layout.driftRect.bottom() - 12.0, 36.0, 14.0),
                     Qt::AlignRight | Qt::AlignBottom, QString::number(-dataRange.maxAbsDrift, 'f', 2));
    painter.drawText(QRectF(layout.driftRect.left(), layout.driftRect.bottom() + 4.0, 100.0, 14.0),
                     Qt::AlignLeft | Qt::AlignVCenter, QString("%1 s").arg(dataRange.minTime, 0, 'f', 2));
    painter.drawText(QRectF(layout.driftRect.right() - 100.0, layout.driftRect.bottom() + 4.0, 100.0, 14.0),
                     Qt::AlignRight | Qt::AlignVCenter, QString("%1 s").arg(dataRange.maxTime, 0, 'f', 2));
    painter.drawText(QRectF(layout.driftRect.left(), layout.driftRect.top() + 2.0, 180.0, 14.0),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("Window %1 s").arg(QString::number(dataRange.maxTime - dataRange.minTime, 'f', 2)));
}

} // namespace bltzr_qt::energyGraph
