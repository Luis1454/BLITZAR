// File: engine/include/config/SimulationArgsInitOptions.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSINITOPTIONS_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSINITOPTIONS_HPP_
#include "config/SimulationArgs.hpp"
#include <ostream>
#include <string>

/// Description: Defines the SimulationArgsInitOptions data or behavior contract.
class SimulationArgsInitOptions final {
public:
    /// Description: Describes the apply operation contract.
    static bool apply(const std::string& key, const std::string& value, SimulationConfig& config,
                      RuntimeArgs& runtime, std::ostream& warnings);
};
#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSINITOPTIONS_HPP_
