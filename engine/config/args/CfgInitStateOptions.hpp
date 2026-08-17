/*
 * @file engine/config/args/CfgInitStateOptions.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Public configuration interfaces and validation contracts for simulation setup.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSINITSTATEOPTIONS_HPP_
#define BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSINITSTATEOPTIONS_HPP_
#include "config/args/CfgMain.hpp"
#include <ostream>
#include <string>

bool applyInitStateOptions(const std::string& key, const std::string& value,
                           SimulationConfig& config, RuntimeArgs& runtime,
                           std::ostream& warnings);
#endif // BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSINITSTATEOPTIONS_HPP_
