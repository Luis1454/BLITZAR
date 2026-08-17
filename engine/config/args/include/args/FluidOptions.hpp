/*
 * @file engine/config/args/include/args/FluidOptions.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Public configuration interfaces and validation contracts for simulation setup.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSFLUIDOPTIONS_HPP_
#define BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSFLUIDOPTIONS_HPP_
#include "args/Main.hpp"
#include <ostream>
#include <string>

bool applyFluidOptions(const std::string& key, const std::string& value, SimulationConfig& config,
                       RuntimeArgs& runtime, std::ostream& warnings);
#endif // BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSFLUIDOPTIONS_HPP_
