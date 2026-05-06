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
    std::ostringstream stream;
    stream << "t=" << input.stats.totalTime << "s step=" << input.stats.steps;
    return stream.str();
}

std::string TelemetryAggregator::buildRuntimeSection(const MainWindowPresentationInput& input)
{
    std::ostringstream stream;
    stream << "Runtime: " << input.stats.serverFps << " fps";
    return stream.str();
}

std::string TelemetryAggregator::buildQueueSection(const MainWindowPresentationInput& input)
{
    std::ostringstream stream;
    stream << "Queue depth: " << input.snapshotPipeline.queueDepth << "/"
           << input.snapshotPipeline.queueCapacity;
    return stream.str();
}

std::string TelemetryAggregator::buildEnergySection(const MainWindowPresentationInput& input)
{
    std::ostringstream stream;
    stream << "Energy: K=" << input.stats.kineticEnergy << " P=" << input.stats.potentialEnergy
           << " T=" << input.stats.thermalEnergy << " R=" << input.stats.radiatedEnergy
           << " Σ=" << input.stats.totalEnergy;
    return stream.str();
}

std::string TelemetryAggregator::buildGpuSection(const MainWindowPresentationInput& input)
{
    std::ostringstream stream;
    stream << "GPU mem: " << input.stats.gpuVramUsedBytes << " / " << input.stats.gpuVramTotalBytes;
    return stream.str();
}

std::string TelemetryAggregator::buildStatusSection(const MainWindowPresentationInput& input)
{
    if (input.stats.faulted) {
        return "Status: FAULTED";
    }
    return input.stats.paused ? "Status: PAUSED" : "Status: RUNNING";
}

std::string TelemetryAggregator::buildTraceSection(const MainWindowPresentationInput& input)
{
    std::ostringstream stream;
    stream << "Trace: " << input.stats.exportLastState;
    if (!input.stats.exportLastMessage.empty()) {
        stream << " | " << input.stats.exportLastMessage;
    }
    return stream.str();
}

}  // namespace bltzr_qt::mainWindow::presenter
