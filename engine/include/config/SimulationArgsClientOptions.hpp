// File: engine/include/config/SimulationArgsClientOptions.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSCLIENTOPTIONS_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSCLIENTOPTIONS_HPP_
#include <iosfwd>
#include <string>
/// Description: Defines the SimulationConfig data or behavior contract.
struct SimulationConfig;
/// Description: Defines the SimulationArgsClientOptions data or behavior contract.
class SimulationArgsClientOptions final {
public:
    static bool apply(const std::string& key, const std::string& value, SimulationConfig& config,
                      std::ostream& warnings);
};
#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSCLIENTOPTIONS_HPP_
