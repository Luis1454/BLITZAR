#include "config/SimulationArgsCoreOptions.hpp"
#include "config/SimulationOptionRegistry.hpp"

bool SimulationArgsCoreOptions::apply(
    const std::string &key,
    const std::string &value,
    SimulationConfig &config,
    RuntimeArgs &runtime,
    std::ostream &warnings
)
{
    if (key == "--config") {
        runtime.configPath = value;
        return true;
    }
    return grav_config::applyCliOption(grav_config::SimulationOptionGroup::Core, key, value, config, warnings);
}
