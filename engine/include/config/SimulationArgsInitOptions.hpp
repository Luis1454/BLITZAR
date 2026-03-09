#pragma once

#include "config/SimulationArgs.hpp"

#include <ostream>
#include <string>

class SimulationArgsInitOptions final {
  public:
    static bool apply(const std::string &key, const std::string &value, SimulationConfig &config, RuntimeArgs &runtime, std::ostream &warnings);
};

