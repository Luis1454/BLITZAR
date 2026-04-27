// File: engine/include/config/SimulationArgsInitStateOptions.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSINITSTATEOPTIONS_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSINITSTATEOPTIONS_HPP_
#include "config/SimulationArgs.hpp"
#include <ostream>
#include <string>

/// Description: Defines the SimulationArgsInitStateOptions data or behavior contract.
class SimulationArgsInitStateOptions final {
public:
    /// Description: Describes the apply operation contract.
    static bool apply(const std::string& key, const std::string& value, SimulationConfig& config,
                      RuntimeArgs& runtime, std::ostream& warnings);
};
#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSINITSTATEOPTIONS_HPP_
