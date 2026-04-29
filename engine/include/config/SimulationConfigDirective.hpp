/*
 * @file engine/include/config/SimulationConfigDirective.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Public configuration interfaces and validation contracts for simulation setup.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONCONFIGDIRECTIVE_HPP_
#define BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONCONFIGDIRECTIVE_HPP_
#include <ostream>
#include <string>
/*
 * @brief Defines the simulation config type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct SimulationConfig;

namespace bltzr_config {
class SimulationConfigDirective {
public:
    [[nodiscard]] static bool applyLine(const std::string& line, SimulationConfig& config,
                                        std::ostream& warnings);
    static void write(std::ostream& out, const SimulationConfig& config);
};
} // namespace bltzr_config
#endif // BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONCONFIGDIRECTIVE_HPP_
