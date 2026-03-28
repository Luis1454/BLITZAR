#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSCOREOPTIONS_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSCOREOPTIONS_HPP_
#include "config/SimulationArgs.hpp"
#include <ostream>
#include <string>
class SimulationArgsCoreOptions final {
public:
    static bool apply(const std::string& key, const std::string& value, SimulationConfig& config,
                      RuntimeArgs& runtime, std::ostream& warnings);
};
#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSCOREOPTIONS_HPP_
