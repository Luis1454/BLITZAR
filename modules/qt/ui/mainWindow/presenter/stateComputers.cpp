/*
 * @file modules/qt/ui/mainWindow/presenter/telemetryAggregator.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Telemetry aggregation implementations.
 */

#include "ui/mainWindow/presenter/telemetryAggregator.hpp"
#include <sstream>

namespace bltzr_qt::mainWindow::presenter {

std::string TelemetryAggregator::buildHeadline(const MainWindowPresentationInput& input)
{
    std::ostringstream ss;
    ss << "t=" << input.stats.totalTime << "s step=" << input.stats.totalSteps;
    return ss.str();
}

std::string TelemetryAggregator::buildRuntimeSection(const MainWindowPresentationInput& input)
{
    std::ostringstream ss;
    ss << "Runtime: " << input.stats.serverFps << " fps";
    return ss.str();
}

std::string TelemetryAggregator::buildQueueSection(const MainWindowPresentationInput& input)
{
    std::ostringstream ss;
    ss << "Queue depth: " << input.stats.queueDepth;
    return ss.str();
}

std::string TelemetryAggregator::buildEnergySection(const MainWindowPresentationInput& input)
{
    std::ostringstream ss;
    ss << "Energy: " << input.stats.totalEnergy;
    return ss.str();
}

std::string TelemetryAggregator::buildGpuSection(const MainWindowPresentationInput& input)
{
    std::ostringstream ss;
    ss << "GPU mem: " << input.stats.gpuMemoryUsed << " / " << input.stats.gpuMemoryTotal;
    return ss.str();
}

std::string TelemetryAggregator::buildStatusSection(const MainWindowPresentationInput& input)
{
    return input.stats.paused ? "Status: PAUSED" : "Status: RUNNING";
}

std::string TelemetryAggregator::buildTraceSection(const MainWindowPresentationInput& input)
{
    return "Trace: [ready]";
}

}  // namespace bltzr_qt::mainWindow::presenter
