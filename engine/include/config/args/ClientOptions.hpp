/*
 * @file engine/include/config/args/ClientOptions.hpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Public configuration interfaces and validation contracts for simulation setup.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSCLIENTOPTIONS_HPP_
#define BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSCLIENTOPTIONS_HPP_
#include <iosfwd>
#include <string>
/*
 * @brief Defines the simulation config type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct SimulationConfig;

bool applyClientOptions(const std::string& key, const std::string& value,
                        SimulationConfig& config, std::ostream& warnings);
#endif // BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSCLIENTOPTIONS_HPP_
