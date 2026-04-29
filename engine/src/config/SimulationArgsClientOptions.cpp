/*
 * @file engine/src/config/SimulationArgsClientOptions.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Configuration parsing, validation, and serialization implementation.
 */

#include "config/SimulationArgsClientOptions.hpp"
#include "config/SimulationOptionRegistry.hpp"

/*
 * @brief Documents the apply operation contract.
 * @param key Input value used by this contract.
 * @param value Input value used by this contract.
 * @param config Input value used by this contract.
 * @param warnings Input value used by this contract.
 * @return bool SimulationArgsClientOptions:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool SimulationArgsClientOptions::apply(const std::string& key, const std::string& value,
                                        SimulationConfig& config, std::ostream& warnings)
{
    return bltzr_config::applyCliOption(bltzr_config::SimulationOptionGroup::Client, key, value,
                                        config, warnings);
}
