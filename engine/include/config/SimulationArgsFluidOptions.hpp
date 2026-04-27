/*
 * @file engine/include/config/SimulationArgsFluidOptions.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Public configuration interfaces and validation contracts for simulation setup.
 */

#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSFLUIDOPTIONS_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSFLUIDOPTIONS_HPP_
#include "config/SimulationArgs.hpp"
#include <ostream>
#include <string>

/*
 * @brief Defines the simulation args fluid options type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class SimulationArgsFluidOptions final {
public:
    /*
     * @brief Documents the apply operation contract.
     * @param key Input value used by this contract.
     * @param value Input value used by this contract.
     * @param config Input value used by this contract.
     * @param runtime Input value used by this contract.
     * @param warnings Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    static bool apply(const std::string& key, const std::string& value, SimulationConfig& config,
                      RuntimeArgs& runtime, std::ostream& warnings);
};
#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSFLUIDOPTIONS_HPP_
