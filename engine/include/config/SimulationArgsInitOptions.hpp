#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSINITOPTIONS_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSINITOPTIONS_HPP_
#include "config/SimulationArgs.hpp"
#include <ostream>
#include <string>
class SimulationArgsInitOptions final {
public:
    static bool apply(const std::string& key, const std::string& value, SimulationConfig& config,
                      RuntimeArgs& runtime, std::ostream& warnings);
};
#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSINITOPTIONS_HPP_
