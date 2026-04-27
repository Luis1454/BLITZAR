/*
 * @file engine/include/config/SimulationOptionRegistry.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Public configuration interfaces and validation contracts for simulation setup.
 */

#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONOPTIONREGISTRY_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONOPTIONREGISTRY_HPP_
#include <iosfwd>
#include <string>
/*
 * @brief Defines the simulation config type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct SimulationConfig;

namespace grav_config {
enum class SimulationOptionGroup {
    Core,
    Client,
    InitState,
    Fluid,
};
[[nodiscard]] bool applyCliOption(SimulationOptionGroup group, const std::string& key,
                                  const std::string& value, SimulationConfig& config,
                                  std::ostream& warnings);
[[nodiscard]] bool applyIniOption(const std::string& key, const std::string& value,
                                  SimulationConfig& config, std::ostream& warnings);
[[nodiscard]] bool applyEnvOption(const std::string& key, const std::string& value,
                                  SimulationConfig& config, std::ostream& warnings);
void printCliUsage(std::ostream& out, SimulationOptionGroup group);
} // namespace grav_config
#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONOPTIONREGISTRY_HPP_
