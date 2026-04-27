// File: engine/src/config/SimulationArgsClientOptions.cpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#include "config/SimulationArgsClientOptions.hpp"
#include "config/SimulationOptionRegistry.hpp"

/// Description: Describes the apply operation contract.
bool SimulationArgsClientOptions::apply(const std::string& key, const std::string& value,
                                        SimulationConfig& config, std::ostream& warnings)
{
    return grav_config::applyCliOption(grav_config::SimulationOptionGroup::Client, key, value,
                                       config, warnings);
}
