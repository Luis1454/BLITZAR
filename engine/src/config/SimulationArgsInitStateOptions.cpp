/*
 * @file engine/src/config/SimulationArgsInitStateOptions.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Configuration parsing, validation, and serialization implementation.
 */

#include "config/SimulationArgsInitStateOptions.hpp"
#include "config/SimulationArgsParse.hpp"
#include "config/SimulationOptionRegistry.hpp"

/*
 * @brief Documents the apply operation contract.
 * @param key Input value used by this contract.
 * @param value Input value used by this contract.
 * @param config Input value used by this contract.
 * @param runtime Input value used by this contract.
 * @param warnings Input value used by this contract.
 * @return bool SimulationArgsInitStateOptions:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool SimulationArgsInitStateOptions::apply(const std::string& key, const std::string& value,
                                           SimulationConfig& config, RuntimeArgs& runtime,
                                           std::ostream& warnings)
{
    if (key == "--target-steps") {
        int parsedValue = runtime.targetSteps;
        if (SimulationArgsParse::parseInt(value, parsedValue) && parsedValue > 0) {
            runtime.targetSteps = parsedValue;
        }
        else {
            warnings << "[args] invalid --target-steps: " << value << "\n";
        }
        return true;
    }
    return bltzr_config::applyCliOption(bltzr_config::SimulationOptionGroup::InitState, key, value,
                                        config, warnings);
}
