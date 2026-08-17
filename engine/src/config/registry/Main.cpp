/*
 * @file engine/src/config/SimulationOptionRegistry.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Configuration parsing, validation, and serialization implementation.
 */

#include "Internal.hpp"
#include <ostream>

namespace bltzr_config {
void printCliUsage(std::ostream& out, SimulationOptionGroup group)
{
    for (std::size_t rangeIndex = 0; rangeIndex < kSimulationOptionRangeCount; ++rangeIndex) {
        const SimulationOptionRange& range = kSimulationOptionRanges[rangeIndex];
        for (std::size_t entryIndex = 0; entryIndex < range.count; ++entryIndex) {
            const SimulationOptionEntry& entry = *(&range.first + entryIndex);
            if (entry.group != group || entry.usage.empty()) {
                continue;
            }
            out << entry.usage;
            if (!entry.aliasUsage.empty()) {
                out << entry.aliasUsage;
            }
        }
    }
}
} // namespace bltzr_config
