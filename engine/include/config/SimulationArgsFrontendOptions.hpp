#pragma once

#include "config/SimulationConfig.hpp"

#include <ostream>
#include <string>

class SimulationArgsFrontendOptions final {
  public:
    static bool apply(const std::string &key, const std::string &value, SimulationConfig &config, std::ostream &warnings);
};

