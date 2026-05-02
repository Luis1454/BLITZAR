/*
 * @file modules/qt/ui/energyGraph/data.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Data filtering and range calculation for energy graph painting.
 */

#include "ui/energyGraph/data.hpp"
#include "ui/EnergyGraphWidget.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace bltzr_qt::energyGraph {

DataRange computeDataRange(const std::vector<EnergyPoint>& history)
{
    DataRange range;

    if (history.empty()) {
        range.minEnergy = 0.0f;
        range.maxEnergy = 1.0f;
        range.minTime = 0.0f;
        range.maxTime = 1.0f;
        range.maxAbsDrift = 0.01f;
        return range;
    }

    constexpr std::size_t visibleSampleLimit = 160u;
    constexpr float visibleTimeSpanSec = 12.0f;

    range.visibleStart =
        (history.size() > visibleSampleLimit) ? (history.size() - visibleSampleLimit) : 0u;

    const EnergyPoint& latest = history.back();
    range.latestPoint = &latest;

    for (std::size_t i = history.size(); i > 0u; --i) {
        const std::size_t index = i - 1u;
        if (latest.time - history[index].time > visibleTimeSpanSec && index >= range.visibleStart) {
            range.visibleStart = index + 1u;
            break;
        }
    }

    if (history.size() - range.visibleStart < 2u) {
        range.visibleStart = history.size() >= 2u ? (history.size() - 2u) : 0u;
    }

    range.minEnergy = std::numeric_limits<float>::infinity();
    range.maxEnergy = -std::numeric_limits<float>::infinity();
    range.maxAbsDrift = 0.01f;
    range.minTime = std::numeric_limits<float>::infinity();
    range.maxTime = -std::numeric_limits<float>::infinity();

    for (std::size_t i = range.visibleStart; i < history.size(); ++i) {
        const EnergyPoint& sample = history[i];
        range.minEnergy = std::min(
            range.minEnergy, std::min(std::min(sample.kinetic, sample.potential),
                                      std::min(sample.thermal, std::min(sample.radiated, sample.total))));
        range.maxEnergy = std::max(
            range.maxEnergy, std::max(std::max(sample.kinetic, sample.potential),
                                      std::max(sample.thermal, std::max(sample.radiated, sample.total))));
        range.maxAbsDrift = std::max(range.maxAbsDrift, std::fabs(sample.drift));
        range.minTime = std::min(range.minTime, sample.time);
        range.maxTime = std::max(range.maxTime, sample.time);
    }

    if (range.maxEnergy <= range.minEnergy + 1e-9f)
        range.maxEnergy = range.minEnergy + 1.0f;
    if (range.maxTime <= range.minTime + 1e-6f)
        range.maxTime = range.minTime + 1.0f;

    return range;
}

} // namespace bltzr_qt::energyGraph
