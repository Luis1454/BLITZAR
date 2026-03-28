#include "config/SimulationArgsClientOptions.hpp"
#include "config/SimulationOptionRegistry.hpp"
bool SimulationArgsClientOptions::apply(const std::string& key, const std::string& value,
                                        SimulationConfig& config, std::ostream& warnings)
{
    return grav_config::applyCliOption(grav_config::SimulationOptionGroup::Client, key, value,
                                       config, warnings);
}
