/*
 * @file engine/config/args/options/CfgInitOptions.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Public configuration interfaces and validation contracts for simulation setup.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSINITOPTIONS_HPP_
#define BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSINITOPTIONS_HPP_
#include "config/args/parsing/CfgMain.hpp"
#include <ostream>
#include <string>

bool applyInitOptions(const std::string& key, const std::string& value, SimulationConfig& config,
                      RuntimeArgs& runtime, std::ostream& warnings);
#endif // BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSINITOPTIONS_HPP_
