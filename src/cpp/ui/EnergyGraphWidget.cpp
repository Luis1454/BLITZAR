#include "ui/EnergyGraphWidget.hpp"

#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QRectF>
#include <QSizePolicy>
#include <QStringView>

#include <algorithm>
#include <cmath>
#include <limits>

namespace qtui {

EnergyGraphWidget::EnergyGraphWidget()
    : QWidget(nullptr)
{
    setMinimumHeight(130);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void EnergyGraphWidget::pushSample(const SimulationStats &stats)
{
    _history.push_back(EnergyPoint{
        stats.kineticEnergy,
        stats.potentialEnergy,
        stats.thermalEnergy,
        stats.radiatedEnergy,
        stats.totalEnergy,
        stats.energyDriftPct
    });
    constexpr std::size_t maxHistory = 720;
    if (_history.size() > maxHistory) {
        _history.erase(_history.begin(), _history.begin() + static_cast<long long>(_history.size() - maxHistory));
    }
    update();
}

void EnergyGraphWidget::paintEvent(PaintEventHandle event)
{
    (void)event;
    QPainter p(this);
    p.fillRect(rect(), QColor(15, 15, 22));
    p.setPen(QColor(45, 45, 58));
    p.drawRect(rect().adjusted(0, 0, -1, -1));

    if (_history.size() < 2) {
        return;
    }

    const QRectF fullRect = rect().adjusted(8, 8, -8, -8);
    const qreal splitY = fullRect.top() + fullRect.height() * 0.74;
    const QRectF energyRect(fullRect.left(), fullRect.top(), fullRect.width(), splitY - fullRect.top() - 6.0);
    const QRectF driftRect(fullRect.left(), splitY + 6.0, fullRect.width(), fullRect.bottom() - (splitY + 6.0));

    p.setPen(QColor(40, 40, 52));
    p.drawLine(QPointF(fullRect.left(), splitY), QPointF(fullRect.right(), splitY));

    float minEnergy = std::numeric_limits<float>::infinity();
    float maxEnergy = -std::numeric_limits<float>::infinity();
    float maxAbsDrift = 0.01f;
    for (const EnergyPoint &e : _history) {
        minEnergy = std::min(minEnergy, std::min(std::min(e.kinetic, e.potential), std::min(e.thermal, e.total)));
        maxEnergy = std::max(maxEnergy, std::max(std::max(e.kinetic, e.potential), std::max(e.thermal, e.total)));
        maxAbsDrift = std::max(maxAbsDrift, std::fabs(e.drift));
    }
    if (maxEnergy <= minEnergy + 1e-9f) {
        maxEnergy = minEnergy + 1.0f;
    }

    auto buildPath = [&](auto valueAccessor, const QRectF &targetRect, float vMin, float vMax, bool centered) {
        QPainterPath path;
        for (std::size_t i = 0; i < _history.size(); ++i) {
            const qreal x = targetRect.left() + targetRect.width() * static_cast<qreal>(i) / static_cast<qreal>(_history.size() - 1);
            const float value = valueAccessor(_history[i]);
            qreal y = 0.0;
            if (centered) {
                const float norm = value / vMax;
                y = targetRect.top() + targetRect.height() * (1.0 - (norm * 0.5 + 0.5));
            } else {
                const float norm = (value - vMin) / (vMax - vMin);
                y = targetRect.top() + targetRect.height() * (1.0 - norm);
            }
            if (i == 0) {
                path.moveTo(x, y);
            } else {
                path.lineTo(x, y);
            }
        }
        return path;
    };

    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(QColor(92, 255, 140), 1.5));
    p.drawPath(buildPath([](const EnergyPoint &e) { return e.kinetic; }, energyRect, minEnergy, maxEnergy, false));
    p.setPen(QPen(QColor(255, 120, 108), 1.5));
    p.drawPath(buildPath([](const EnergyPoint &e) { return e.potential; }, energyRect, minEnergy, maxEnergy, false));
    p.setPen(QPen(QColor(120, 200, 255), 1.5));
    p.drawPath(buildPath([](const EnergyPoint &e) { return e.total; }, energyRect, minEnergy, maxEnergy, false));
    p.setPen(QPen(QColor(255, 170, 90), 1.4));
    p.drawPath(buildPath([](const EnergyPoint &e) { return e.thermal; }, energyRect, minEnergy, maxEnergy, false));
    p.setPen(QPen(QColor(180, 120, 255), 1.4));
    p.drawPath(buildPath([](const EnergyPoint &e) { return e.radiated; }, energyRect, minEnergy, maxEnergy, false));

    p.setPen(QColor(70, 70, 84));
    p.drawLine(QPointF(driftRect.left(), driftRect.center().y()), QPointF(driftRect.right(), driftRect.center().y()));

    p.setPen(QPen(QColor(255, 230, 120), 1.5));
    p.drawPath(buildPath([&](const EnergyPoint &e) { return e.drift; }, driftRect, -maxAbsDrift, maxAbsDrift, true));
    p.setPen(QColor(180, 180, 190));
    p.drawText(QRectF(energyRect.left(), energyRect.top(), energyRect.width(), 16.0), Qt::AlignLeft | Qt::AlignVCenter, "Energy");
    p.drawText(QRectF(driftRect.left(), driftRect.top(), driftRect.width(), 16.0), Qt::AlignLeft | Qt::AlignVCenter, "Drift %");

    const qreal legendY = energyRect.top() + 18.0;
    const qreal legendX = energyRect.left() + 6.0;
    const qreal stepX = 82.0;
    const auto drawLegend = [&](qreal x, const QColor &color, QStringView label) {
        p.setPen(QPen(color, 2.0));
        p.drawLine(QPointF(x, legendY), QPointF(x + 16.0, legendY));
        p.setPen(QColor(190, 190, 204));
        p.drawText(QPointF(x + 20.0, legendY + 4.0), label.toString());
    };
    drawLegend(legendX + stepX * 0, QColor(92, 255, 140), QStringLiteral("Ekin"));
    drawLegend(legendX + stepX * 1, QColor(255, 120, 108), QStringLiteral("Epot"));
    drawLegend(legendX + stepX * 2, QColor(255, 170, 90), QStringLiteral("Eth"));
    drawLegend(legendX + stepX * 3, QColor(180, 120, 255), QStringLiteral("Erad"));
    drawLegend(legendX + stepX * 4, QColor(120, 200, 255), QStringLiteral("Etot"));
    drawLegend(legendX + stepX * 5, QColor(255, 230, 120), QStringLiteral("dE%"));
    p.setRenderHint(QPainter::Antialiasing, false);
}

} // namespace qtui
