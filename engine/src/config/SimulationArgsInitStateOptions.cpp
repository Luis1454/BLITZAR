#include "config/SimulationArgsInitStateOptions.hpp"
#include "config/SimulationArgsParse.hpp"
#include "config/SimulationOptionRegistry.hpp"

bool SimulationArgsInitStateOptions::apply(
    const std::string &key,
    const std::string &value,
    SimulationConfig &config,
    RuntimeArgs &runtime,
    std::ostream &warnings
)
{
    if (key == "--target-steps") {
        int parsedValue = runtime.targetSteps;
        if (SimulationArgsParse::parseInt(value, parsedValue) && parsedValue > 0) {
            runtime.targetSteps = parsedValue;
        } else {
            warnings << "[args] invalid --target-steps: " << value << "\n";
        }
        return true;
    }
    return grav_config::applyCliOption(grav_config::SimulationOptionGroup::InitState, key, value, config, warnings);
}
