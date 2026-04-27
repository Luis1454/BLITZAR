// File: engine/include/config/SimulationConfigDirective.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONCONFIGDIRECTIVE_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONCONFIGDIRECTIVE_HPP_
#include <ostream>
#include <string>
/// Description: Defines the SimulationConfig data or behavior contract.
struct SimulationConfig;
namespace grav_config {
/// Description: Defines the SimulationConfigDirective data or behavior contract.
class SimulationConfigDirective {
public:
    [[nodiscard]] static bool applyLine(const std::string& line, SimulationConfig& config,
                                        std::ostream& warnings);
    /// Description: Executes the write operation.
    static void write(std::ostream& out, const SimulationConfig& config);
};
} // namespace grav_config
#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONCONFIGDIRECTIVE_HPP_
