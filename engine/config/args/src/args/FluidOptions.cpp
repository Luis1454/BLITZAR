/*
 * @file engine/config/args/src/args/FluidOptions.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Configuration parsing, validation, and serialization implementation.
 */

#include "args/FluidOptions.hpp"
#include "registry/Main.hpp"

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
bool applyFluidOptions(const std::string& key, const std::string& value,
                                       SimulationConfig& config, RuntimeArgs& runtime,
                                       std::ostream& warnings)
{
    static_cast<void>(runtime);
    return bltzr_config::applyCliOption(bltzr_config::SimulationOptionGroup::Fluid, key, value,
                                        config, warnings);
}
