#include "config/SimulationArgsFrontendOptions.hpp"
#include "config/SimulationOptionRegistry.hpp"

bool SimulationArgsFrontendOptions::apply(
    const std::string &key,
    const std::string &value,
    SimulationConfig &config,
    std::ostream &warnings
)
{
    return grav_config::applyCliOption(grav_config::SimulationOptionGroup::Frontend, key, value, config, warnings);
}
