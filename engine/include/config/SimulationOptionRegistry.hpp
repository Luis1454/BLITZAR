#pragma once

#include "config/SimulationConfig.hpp"

#include <ostream>
#include <string>

namespace grav_config {

enum class SimulationOptionGroup {
    Core,
    Frontend,
    InitState,
    Fluid,
};

[[nodiscard]] bool applyCliOption(
    SimulationOptionGroup group,
    const std::string &key,
    const std::string &value,
    SimulationConfig &config,
    std::ostream &warnings
);

[[nodiscard]] bool applyIniOption(
    const std::string &key,
    const std::string &value,
    SimulationConfig &config,
    std::ostream &warnings
);
[[nodiscard]] bool applyEnvOption(
    const std::string &key,
    const std::string &value,
    SimulationConfig &config,
    std::ostream &warnings
);
void printCliUsage(std::ostream &out, SimulationOptionGroup group);

} // namespace grav_config

