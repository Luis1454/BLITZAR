#include "ui/EnergyGraphWidget.hpp"

#include <QPaintEvent>
#include <QPalette>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QRectF>
#include <QSizePolicy>
#include <QStringView>

#include <algorithm>
#include <cmath>
#include <limits>

namespace grav_qt {

static bool isDarkTheme(const QPalette &palette)
{
    return palette.color(QPalette::Window).lightness() < 128;
}

static QColor panelCurveColor(const QColor &darkColor, const QColor &lightColor, bool darkTheme)
{
    return darkTheme ? darkColor : lightColor;
}

EnergyGraphWidget::EnergyGraphWidget()
    : QWidget(nullptr)
{
    setMinimumHeight(130);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

QString EnergyGraphWidget::energyXAxisLabel()
{
    return QStringLiteral("Simulation time [s]");
}

QString EnergyGraphWidget::energyYAxisLabel()
{
    return QStringLiteral("Energy [J]");
}

QString EnergyGraphWidget::driftXAxisLabel()
{
    return QStringLiteral("Simulation time [s]");
}

QString EnergyGraphWidget::driftYAxisLabel()
{
    return QStringLiteral("Drift [%]");
}

QStringList EnergyGraphWidget::legendLabels()
{
    return {
        QStringLiteral("Kinetic [J]"),
        QStringLiteral("Potential [J]"),
        QStringLiteral("Thermal [J]"),
        QStringLiteral("Radiated [J]"),
        QStringLiteral("Total [J]"),
        QStringLiteral("Drift [%]")
    };
}

void EnergyGraphWidget::pushSample(const SimulationStats &stats)
{
    _history.push_back(EnergyPoint{
        stats.kineticEnergy,
        stats.potentialEnergy,
        stats.thermalEnergy,
        stats.radiatedEnergy,
        stats.totalEnergy,
        stats.energyDriftPct,
        stats.totalTime
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
    const QPalette widgetPalette = palette();
    const bool darkTheme = isDarkTheme(widgetPalette);
    const QColor background = widgetPalette.color(QPalette::Window);
    const QColor border = widgetPalette.color(QPalette::Mid);
    const QColor labelColor = widgetPalette.color(QPalette::WindowText);
    QColor gridColor = border;
    gridColor.setAlpha(darkTheme ? 190 : 120);

    const QColor kineticColor = panelCurveColor(QColor(92, 255, 140), QColor(0, 122, 52), darkTheme);
    const QColor potentialColor = panelCurveColor(QColor(255, 120, 108), QColor(180, 45, 35), darkTheme);
    const QColor thermalColor = panelCurveColor(QColor(255, 170, 90), QColor(176, 99, 10), darkTheme);
    const QColor radiatedColor = panelCurveColor(QColor(180, 120, 255), QColor(108, 52, 188), darkTheme);
    const QColor totalColor = panelCurveColor(QColor(120, 200, 255), QColor(0, 102, 170), darkTheme);
    const QColor driftColor = panelCurveColor(QColor(255, 230, 120), QColor(168, 132, 0), darkTheme);

    p.fillRect(rect(), background);
    p.setPen(border);
    p.drawRect(rect().adjusted(0, 0, -1, -1));

    if (_history.size() < 2) {
        return;
    }

    const QRectF fullRect = rect().adjusted(48, 12, -16, -28);
    const qreal splitY = fullRect.top() + fullRect.height() * 0.74;
    const QRectF energyRect(fullRect.left(), fullRect.top(), fullRect.width(), splitY - fullRect.top() - 6.0);
    const QRectF driftRect(fullRect.left(), splitY + 6.0, fullRect.width(), fullRect.bottom() - (splitY + 6.0));

    p.setPen(gridColor);
    p.drawLine(QPointF(fullRect.left(), splitY), QPointF(fullRect.right(), splitY));
    p.drawRect(energyRect);
    p.drawRect(driftRect);

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
    p.setPen(QPen(kineticColor, 1.5));
    p.drawPath(buildPath([](const EnergyPoint &e) { return e.kinetic; }, energyRect, minEnergy, maxEnergy, false));
    p.setPen(QPen(potentialColor, 1.5));
    p.drawPath(buildPath([](const EnergyPoint &e) { return e.potential; }, energyRect, minEnergy, maxEnergy, false));
    p.setPen(QPen(totalColor, 1.5));
    p.drawPath(buildPath([](const EnergyPoint &e) { return e.total; }, energyRect, minEnergy, maxEnergy, false));
    p.setPen(QPen(thermalColor, 1.4));
    p.drawPath(buildPath([](const EnergyPoint &e) { return e.thermal; }, energyRect, minEnergy, maxEnergy, false));
    p.setPen(QPen(radiatedColor, 1.4));
    p.drawPath(buildPath([](const EnergyPoint &e) { return e.radiated; }, energyRect, minEnergy, maxEnergy, false));

    p.setPen(gridColor);
    p.drawLine(QPointF(driftRect.left(), driftRect.center().y()), QPointF(driftRect.right(), driftRect.center().y()));

    p.setPen(QPen(driftColor, 1.5));
    p.drawPath(buildPath([&](const EnergyPoint &e) { return e.drift; }, driftRect, -maxAbsDrift, maxAbsDrift, true));
    p.setPen(labelColor);
    p.drawText(QRectF(energyRect.left(), energyRect.top() - 14.0, energyRect.width(), 14.0), Qt::AlignLeft | Qt::AlignVCenter, energyYAxisLabel());
    p.drawText(QRectF(driftRect.left(), driftRect.top() - 14.0, driftRect.width(), 14.0), Qt::AlignLeft | Qt::AlignVCenter, driftYAxisLabel());
    p.drawText(QRectF(energyRect.right() - 150.0, energyRect.bottom() + 4.0, 150.0, 14.0), Qt::AlignRight | Qt::AlignVCenter, energyXAxisLabel());
    p.drawText(QRectF(driftRect.right() - 150.0, driftRect.bottom() + 4.0, 150.0, 14.0), Qt::AlignRight | Qt::AlignVCenter, driftXAxisLabel());

    const qreal legendY = energyRect.top() + 18.0;
    const qreal legendX = energyRect.left() + 6.0;
    const qreal stepX = 116.0;
    const QStringList labels = legendLabels();
    const auto drawLegend = [&](qreal x, const QColor &color, QStringView label) {
        p.setPen(QPen(color, 2.0));
        p.drawLine(QPointF(x, legendY), QPointF(x + 16.0, legendY));
        p.setPen(labelColor);
        p.drawText(QPointF(x + 20.0, legendY + 4.0), label.toString());
    };
    drawLegend(legendX + stepX * 0, kineticColor, labels.at(0));
    drawLegend(legendX + stepX * 1, potentialColor, labels.at(1));
    drawLegend(legendX + stepX * 2, thermalColor, labels.at(2));
    drawLegend(legendX + stepX * 3, radiatedColor, labels.at(3));
    drawLegend(legendX + stepX * 4, totalColor, labels.at(4));
    drawLegend(legendX + stepX * 5, driftColor, labels.at(5));
    p.setRenderHint(QPainter::Antialiasing, false);
}

} // namespace grav_qt
