/*
 * @file engine/src/config/SimulationOptionRegistry.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Configuration parsing, validation, and serialization implementation.
 */

#include "SimulationOptionRegistryInternal.hpp"
#include <ostream>

namespace bltzr_config {
void printCliUsage(std::ostream& out, SimulationOptionGroup group)
{
    for (std::size_t index = 0; index < kSimulationOptionCount; ++index) {
        const SimulationOptionEntry& entry = kSimulationOptions[index];
        if (entry.group != group || entry.usage.empty()) {
            continue;
        }
        out << entry.usage;
        if (!entry.aliasUsage.empty()) {
            out << entry.aliasUsage;
        }
    }
}
} // namespace bltzr_config
