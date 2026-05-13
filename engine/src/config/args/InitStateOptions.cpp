/*
 * @file engine/src/config/args/InitStateOptions.cpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Configuration parsing, validation, and serialization implementation.
 */

#include "config/args/InitStateOptions.hpp"
#include "config/args/Parse.hpp"
#include "config/registry/Main.hpp"

/*
 * @brief Documents the apply operation contract.
 * @param key Input value used by this contract.
 * @param value Input value used by this contract.
 * @param config Input value used by this contract.
 * @param runtime Input value used by this contract.
 * @param warnings Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool applyInitStateOptions(const std::string& key, const std::string& value,
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
