// File: engine/include/config/SimulationOptionRegistry.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONOPTIONREGISTRY_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONOPTIONREGISTRY_HPP_
#include <iosfwd>
#include <string>
/// Description: Defines the SimulationConfig data or behavior contract.
struct SimulationConfig;

namespace grav_config {
/// Description: Enumerates the supported SimulationOptionGroup values.
enum class SimulationOptionGroup {
    Core,
    Client,
    InitState,
    Fluid,
};
/// Description: Describes the apply cli option operation contract.
[[nodiscard]] bool applyCliOption(SimulationOptionGroup group, const std::string& key,
                                  const std::string& value, SimulationConfig& config,
                                  std::ostream& warnings);
/// Description: Describes the apply ini option operation contract.
[[nodiscard]] bool applyIniOption(const std::string& key, const std::string& value,
                                  SimulationConfig& config, std::ostream& warnings);
/// Description: Describes the apply env option operation contract.
[[nodiscard]] bool applyEnvOption(const std::string& key, const std::string& value,
                                  SimulationConfig& config, std::ostream& warnings);
/// Description: Executes the printCliUsage operation.
void printCliUsage(std::ostream& out, SimulationOptionGroup group);
} // namespace grav_config
#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONOPTIONREGISTRY_HPP_
