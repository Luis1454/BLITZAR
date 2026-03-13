#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSCLIENTOPTIONS_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSCLIENTOPTIONS_HPP_

#include "config/SimulationConfig.hpp"

#include <ostream>
#include <string>

class SimulationArgsClientOptions final {
  public:
    static bool apply(const std::string &key, const std::string &value, SimulationConfig &config, std::ostream &warnings);
};


#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSCLIENTOPTIONS_HPP_
