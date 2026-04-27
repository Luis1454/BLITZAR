// File: engine/include/server/SimulationInitConfig.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_SERVER_SIMULATIONINITCONFIG_HPP_
#define GRAVITY_ENGINE_INCLUDE_SERVER_SIMULATIONINITCONFIG_HPP_
#include "types/SimulationTypes.hpp"
#include <iosfwd>
#include <string>
/// Description: Defines the SimulationConfig data or behavior contract.
struct SimulationConfig;
/// Description: Defines the ResolvedInitialStatePlan data or behavior contract.
struct ResolvedInitialStatePlan {
    InitialStateConfig config;
    std::string inputFile;
    std::string inputFormat = "auto";
    std::string summary;
};
/// Description: Executes the resolveInitialStatePlan operation.
ResolvedInitialStatePlan resolveInitialStatePlan(const SimulationConfig& config, std::ostream& log);
/// Description: Executes the buildInitialStateConfig operation.
InitialStateConfig buildInitialStateConfig(const SimulationConfig& config);
#endif // GRAVITY_ENGINE_INCLUDE_SERVER_SIMULATIONINITCONFIG_HPP_
