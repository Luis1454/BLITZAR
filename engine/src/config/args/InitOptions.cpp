/*
 * @file engine/src/config/args/InitOptions.cpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Configuration parsing, validation, and serialization implementation.
 */

#include "config/args/InitOptions.hpp"
#include "config/args/FluidOptions.hpp"
#include "config/args/InitStateOptions.hpp"

/*
 * @brief Documents the apply operation contract.
 * @param key Used to identify which option to apply.
 * @param value The raw string value to apply for the identified option.
 * @param config The simulation configuration to modify based on the identified option and value.
 * @param runtime The runtime arguments to modify based on the identified option and value.
 * @param warnings The stream to write any warnings produced during application.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool applyInitOptions(const std::string& key, const std::string& value,
                                      SimulationConfig& config, RuntimeArgs& runtime,
                                      std::ostream& warnings)
{
    return applyInitStateOptions(key, value, config, runtime, warnings)
        || applyFluidOptions(key, value, config, runtime, warnings);
}
