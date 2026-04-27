/*
 * @file modules/qt/ui/EnergyGraphWidget.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "ui/EnergyGraphWidget.hpp"
#include "ui/EnergyGraphWidgetPaint.hpp"
#include <QSizePolicy>
#include <algorithm>

namespace grav_qt {
EnergyGraphWidget::EnergyGraphWidget() : QWidget(nullptr)
{
    setMinimumHeight(128);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void EnergyGraphWidget::clearHistory()
{
    _history.clear();
    update();
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
    return {QStringLiteral("Kinetic [J]"), QStringLiteral("Potential [J]"),
            QStringLiteral("Thermal [J]"), QStringLiteral("Radiated [J]"),
            QStringLiteral("Total [J]"),   QStringLiteral("Drift [%]")};
}

std::size_t EnergyGraphWidget::sampleCount() const
{
    return _history.size();
}

void EnergyGraphWidget::pushSample(const SimulationStats& stats)
{
    const float sampleTime =
        std::max(stats.totalTime, static_cast<float>(stats.steps) * std::max(0.0f, stats.dt));
    _history.push_back(EnergyPoint{stats.kineticEnergy, stats.potentialEnergy, stats.thermalEnergy,
                                   stats.radiatedEnergy, stats.totalEnergy, stats.energyDriftPct,
                                   sampleTime});
    constexpr std::size_t maxHistory = 720;
    if (_history.size() > maxHistory) {
        _history.erase(_history.begin(),
                       _history.begin() + static_cast<long long>(_history.size() - maxHistory));
    }
    update();
}

void EnergyGraphWidget::paintEvent(PaintEventHandle event)
{
    EnergyGraphWidgetPaint::paint(*this, _history, event);
}
} // namespace grav_qt
