/*
 * @file modules/qt/ui/mainWindow/presenter/formatters.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Display formatting utility implementations.
 */

#include "ui/mainWindow/presenter/formatters.hpp"
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>

namespace bltzr_qt::mainWindow::presenter {

std::string Formatters::fixedLabel(float value, int precision)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << value;
    return stream.str();
}

std::string Formatters::bytesLabel(long bytes)
{
    constexpr double kMiB = 1024.0 * 1024.0;
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(1) << (static_cast<double>(bytes) / kMiB) << " MiB";
    return stream.str();
}

std::string Formatters::ageLabel(std::uint32_t ageMs)
{
    return (ageMs != std::numeric_limits<std::uint32_t>::max()) ?
               std::to_string(ageMs) + "ms" :
               "n/a";
}

std::string Formatters::latencyLabel(std::uint32_t snapshotLatencyMs,
                                     std::uint32_t pipelineLatencyMs)
{
    if (snapshotLatencyMs != std::numeric_limits<std::uint32_t>::max()) {
        return std::to_string(snapshotLatencyMs) + "ms";
    }
    if (pipelineLatencyMs != std::numeric_limits<std::uint32_t>::max()) {
        return std::to_string(pipelineLatencyMs) + "ms";
    }
    return "n/a";
}

std::string Formatters::stateLabel(int stateValue)
{
    return std::to_string(stateValue);
}

}  // namespace bltzr_qt::mainWindow::presenter
