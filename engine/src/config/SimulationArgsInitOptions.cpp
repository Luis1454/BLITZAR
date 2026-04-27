/*
 * @file engine/src/config/SimulationArgsInitOptions.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Configuration parsing, validation, and serialization implementation.
 */

#include "config/SimulationArgsInitOptions.hpp"
#include "config/SimulationArgsFluidOptions.hpp"
#include "config/SimulationArgsInitStateOptions.hpp"

/*
 * @brief Documents the apply operation contract.
 * @param key Input value used by this contract.
 * @param value Input value used by this contract.
 * @param config Input value used by this contract.
 * @param runtime Input value used by this contract.
 * @param warnings Input value used by this contract.
 * @return bool SimulationArgsInitOptions:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool SimulationArgsInitOptions::apply(const std::string& key, const std::string& value,
                                      SimulationConfig& config, RuntimeArgs& runtime,
                                      std::ostream& warnings)
{
    if (SimulationArgsInitStateOptions::apply(key, value, config, runtime, warnings)) {
        return true;
    }
    if (SimulationArgsFluidOptions::apply(key, value, config, runtime, warnings)) {
        return true;
    }
    return false;
}
