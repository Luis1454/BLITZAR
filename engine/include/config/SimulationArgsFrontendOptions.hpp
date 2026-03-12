#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSFRONTENDOPTIONS_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSFRONTENDOPTIONS_HPP_

#include "config/SimulationConfig.hpp"

#include <ostream>
#include <string>

class SimulationArgsFrontendOptions final {
  public:
    static bool apply(const std::string &key, const std::string &value, SimulationConfig &config, std::ostream &warnings);
};


#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSFRONTENDOPTIONS_HPP_
