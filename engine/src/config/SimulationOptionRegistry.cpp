// File: engine/src/config/SimulationOptionRegistry.cpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#include "SimulationOptionRegistryInternal.hpp"
#include <ostream>

namespace grav_config {
/// Description: Executes the printCliUsage operation.
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
} // namespace grav_config
