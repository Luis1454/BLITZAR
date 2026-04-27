// File: modules/qt/ui/EnergyGraphWidget.cpp
// Purpose: Client module implementation for BLITZAR extension workflows.

#include "ui/EnergyGraphWidget.hpp"
#include "ui/EnergyGraphWidgetPaint.hpp"
#include <QSizePolicy>
#include <algorithm>
namespace grav_qt {
/// Description: Executes the EnergyGraphWidget operation.
EnergyGraphWidget::EnergyGraphWidget() : QWidget(nullptr)
{
    /// Description: Executes the setMinimumHeight operation.
    setMinimumHeight(128);
    /// Description: Executes the setSizePolicy operation.
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}
/// Description: Executes the clearHistory operation.
void EnergyGraphWidget::clearHistory()
{
    _history.clear();
    /// Description: Executes the update operation.
    update();
}
/// Description: Executes the energyXAxisLabel operation.
QString EnergyGraphWidget::energyXAxisLabel()
{
    return QStringLiteral("Simulation time [s]");
}
/// Description: Executes the energyYAxisLabel operation.
QString EnergyGraphWidget::energyYAxisLabel()
{
    return QStringLiteral("Energy [J]");
}
/// Description: Executes the driftXAxisLabel operation.
QString EnergyGraphWidget::driftXAxisLabel()
{
    return QStringLiteral("Simulation time [s]");
}
/// Description: Executes the driftYAxisLabel operation.
QString EnergyGraphWidget::driftYAxisLabel()
{
    return QStringLiteral("Drift [%]");
}
/// Description: Executes the legendLabels operation.
QStringList EnergyGraphWidget::legendLabels()
{
    return {QStringLiteral("Kinetic [J]"), QStringLiteral("Potential [J]"),
            QStringLiteral("Thermal [J]"), QStringLiteral("Radiated [J]"),
            QStringLiteral("Total [J]"),   QStringLiteral("Drift [%]")};
}
/// Description: Executes the sampleCount operation.
std::size_t EnergyGraphWidget::sampleCount() const
{
    return _history.size();
}
/// Description: Executes the pushSample operation.
void EnergyGraphWidget::pushSample(const SimulationStats& stats)
{
    const float sampleTime =
        /// Description: Executes the max operation.
        std::max(stats.totalTime, static_cast<float>(stats.steps) * std::max(0.0f, stats.dt));
    _history.push_back(EnergyPoint{stats.kineticEnergy, stats.potentialEnergy, stats.thermalEnergy,
                                   stats.radiatedEnergy, stats.totalEnergy, stats.energyDriftPct,
                                   sampleTime});
    constexpr std::size_t maxHistory = 720;
    if (_history.size() > maxHistory) {
        _history.erase(_history.begin(),
                       _history.begin() + static_cast<long long>(_history.size() - maxHistory));
    }
    /// Description: Executes the update operation.
    update();
}
/// Description: Executes the paintEvent operation.
void EnergyGraphWidget::paintEvent(PaintEventHandle event)
{
    /// Description: Executes the paint operation.
    EnergyGraphWidgetPaint::paint(*this, _history, event);
}
} // namespace grav_qt
