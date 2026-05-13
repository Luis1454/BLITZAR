/*
 * @file modules/qt/src/widgets/graphs/Graph.cpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "widgets/graphs/Graph.hpp"
#include "widgets/graphs/Paint.hpp"
#include <QSizePolicy>
#include <algorithm>

namespace bltzr_qt {
Graph::Graph() : QWidget(nullptr)
{
    setMinimumHeight(128);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void Graph::clearHistory()
{
    _history.clear();
    update();
}

QString Graph::energyXAxisLabel()
{
    return QStringLiteral("Simulation time [s]");
}

QString Graph::energyYAxisLabel()
{
    return QStringLiteral("Energy [J]");
}

QString Graph::driftXAxisLabel()
{
    return QStringLiteral("Simulation time [s]");
}

QString Graph::driftYAxisLabel()
{
    return QStringLiteral("Drift [%]");
}

QStringList Graph::legendLabels()
{
    return {QStringLiteral("Kinetic [J]"), QStringLiteral("Potential [J]"),
            QStringLiteral("Thermal [J]"), QStringLiteral("Radiated [J]"),
            QStringLiteral("Total [J]"),   QStringLiteral("Drift [%]")};
}

std::size_t Graph::sampleCount() const
{
    return _history.size();
}

void Graph::pushSample(const SimulationStats& stats)
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

void Graph::paintEvent(PaintEventHandle event)
{
    paintGraph(*this, _history, event);
}
} // namespace bltzr_qt
