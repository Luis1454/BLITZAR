#include "config/SimulationArgsFluidOptions.hpp"
#include "config/SimulationArgsInitOptions.hpp"
#include "config/SimulationArgsInitStateOptions.hpp"

bool SimulationArgsInitOptions::apply(
    const std::string &key,
    const std::string &value,
    SimulationConfig &config,
    RuntimeArgs &runtime,
    std::ostream &warnings
)
{
    if (SimulationArgsInitStateOptions::apply(key, value, config, runtime, warnings)) {
        return true;
    }
    if (SimulationArgsFluidOptions::apply(key, value, config, runtime, warnings)) {
        return true;
    }
    return false;
}
