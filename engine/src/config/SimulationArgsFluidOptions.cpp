// File: engine/src/config/SimulationArgsFluidOptions.cpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#include "config/SimulationArgsFluidOptions.hpp"
#include "config/SimulationOptionRegistry.hpp"
bool SimulationArgsFluidOptions::apply(const std::string& key, const std::string& value,
                                       SimulationConfig& config, RuntimeArgs& runtime,
                                       std::ostream& warnings)
{
    static_cast<void>(runtime);
    return grav_config::applyCliOption(grav_config::SimulationOptionGroup::Fluid, key, value,
                                       config, warnings);
}
