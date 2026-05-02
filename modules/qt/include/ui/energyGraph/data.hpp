/*
 * @file modules/qt/include/ui/energyGraph/data.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Data filtering and range calculation for energy graph painting.
 */

#ifndef BLTZR_QT_ENERGY_GRAPH_DATA_HPP
#define BLTZR_QT_ENERGY_GRAPH_DATA_HPP

#include "ui/EnergyGraphWidget.hpp"
#include <cstddef>

namespace bltzr_qt::energyGraph {

/**
 * @brief Computed data ranges for graph rendering.
 */
struct DataRange {
    std::size_t visibleStart = 0;
    float minEnergy = 0.0f;
    float maxEnergy = 1.0f;
    float maxAbsDrift = 0.01f;
    float minTime = 0.0f;
    float maxTime = 1.0f;
    const EnergyPoint* latestPoint = nullptr;
};

/**
 * @brief Calculate visible data range and window for graph rendering.
 * @param history Complete energy point history
 * @return Computed data range with visible sample indices and ranges
 */
DataRange computeDataRange(const std::vector<EnergyPoint>& history);

} // namespace bltzr_qt::energyGraph

#endif // BLTZR_QT_ENERGY_GRAPH_DATA_HPP
