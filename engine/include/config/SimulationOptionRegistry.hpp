#ifndef GRAVITY_SIM_SIMULATIONOPTIONREGISTRY_HPP
#define GRAVITY_SIM_SIMULATIONOPTIONREGISTRY_HPP

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

bool applyCliOption(
    SimulationOptionGroup group,
    const std::string &key,
    const std::string &value,
    SimulationConfig &config,
    std::ostream &warnings
);

bool applyIniOption(const std::string &key, const std::string &value, SimulationConfig &config, std::ostream &warnings);
bool applyEnvOption(const std::string &key, const std::string &value, SimulationConfig &config, std::ostream &warnings);
void printCliUsage(std::ostream &out, SimulationOptionGroup group);

} // namespace grav_config

#endif // GRAVITY_SIM_SIMULATIONOPTIONREGISTRY_HPP
