#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONCONFIGDIRECTIVE_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONCONFIGDIRECTIVE_HPP_

#include "config/SimulationConfig.hpp"

#include <ostream>
#include <string>

namespace grav_config {

class SimulationConfigDirective {
public:
    [[nodiscard]] static bool applyLine(
        const std::string &line,
        SimulationConfig &config,
        std::ostream &warnings
    );

    static void write(std::ostream &out, const SimulationConfig &config);
};

} // namespace grav_config

#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONCONFIGDIRECTIVE_HPP_
