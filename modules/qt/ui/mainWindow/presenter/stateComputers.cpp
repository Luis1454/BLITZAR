/*
 * @file modules/qt/ui/mainWindow/presenter/stateComputers.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief State derivation implementations.
 */

#include "ui/mainWindow/presenter/stateComputers.hpp"
#include <algorithm>
#include <limits>
#include <sstream>

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
    if (input.stats.faulted) {
        return "faulted";
    }
    if (input.stats.paused) {
        return "paused";
    }
    return "running";
}

std::string StateComputers::viewportStateLabel(const MainWindowPresentationInput& input)
{
    if (input.snapshotPipeline.queueDepth == 0u) {
        return input.displayedParticles == 0u ? "empty" : "synced";
    }
    return "streaming";
}

std::string StateComputers::progressLabel(const MainWindowPresentationInput& input)
{
    std::ostringstream stream;
    if (input.clientDrawCap == 0u) {
        stream << "0%";
        return stream.str();
    }

    std::uint32_t percent = static_cast<std::uint32_t>((input.displayedParticles * 100u) / input.clientDrawCap);
    if (percent > 100u) {
        percent = 100u;
    }
    stream << percent << "%";
    return stream.str();
}

std::string StateComputers::etaLabel(const MainWindowPresentationInput& input)
{
    if (input.simulationHorizonSeconds <= 0.0f || input.stats.totalTime >= input.simulationHorizonSeconds) {
        return "complete";
    }

    std::ostringstream stream;
    stream << (input.simulationHorizonSeconds - input.stats.totalTime) << "s";
    return stream.str();
}

std::string StateComputers::exportStateLabel(const MainWindowPresentationInput& input)
{
    if (input.stats.exportActive) {
        return input.stats.exportLastState.empty() ? "exporting" : input.stats.exportLastState;
    }
    return input.stats.exportLastState.empty() ? "idle" : input.stats.exportLastState;
}

float StateComputers::simulatedSecondsPerSecond(const MainWindowPresentationInput& input)
{
    return input.stats.dt * input.stats.serverFps;
}

}  // namespace bltzr_qt::mainWindow::presenter
