/*
 * @file modules/qt/ui/mainWindow/presenter/stateComputers.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief State derivation implementations.
 */

#include "ui/mainWindow/presenter/stateComputers.hpp"
#include <algorithm>
#include <limits>

namespace bltzr_qt::mainWindow::presenter {

bool StateComputers::isStale(std::uint32_t ageMs)
{
    return (ageMs != std::numeric_limits<std::uint32_t>::max()) && (ageMs > 1000u);
}

bool StateComputers::hasAge(std::uint32_t ageMs)
{
    return ageMs != std::numeric_limits<std::uint32_t>::max();
}

std::string StateComputers::backendStateLabel(const MainWindowPresentationInput& input)
{
    if (input.stats.faulted)
        return "faulted";
    if (input.stats.paused)
        return "paused";
    return "running";
}

std::string StateComputers::viewportStateLabel(const MainWindowPresentationInput& input)
{
    return "viewport_ok";
}

std::string StateComputers::progressLabel(const MainWindowPresentationInput& input)
{
    return "progress";
}

std::string StateComputers::etaLabel(const MainWindowPresentationInput& input)
{
    return "eta_unknown";
}

std::string StateComputers::exportStateLabel(const MainWindowPresentationInput& input)
{
    return input.stats.exportActive ? "exporting" : "idle";
}

float StateComputers::simulatedSecondsPerSecond(const MainWindowPresentationInput& input)
{
    return input.stats.dt * input.stats.serverFps;
}

}  // namespace bltzr_qt::mainWindow::presenter
