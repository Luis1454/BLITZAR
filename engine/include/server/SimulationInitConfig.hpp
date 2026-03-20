#ifndef GRAVITY_ENGINE_INCLUDE_SERVER_SIMULATIONINITCONFIG_HPP_
#define GRAVITY_ENGINE_INCLUDE_SERVER_SIMULATIONINITCONFIG_HPP_

#include "types/SimulationTypes.hpp"

#include <iosfwd>
#include <string>

struct SimulationConfig;

struct ResolvedInitialStatePlan {
    InitialStateConfig config;
    std::string inputFile;
    std::string inputFormat = "auto";
    std::string summary;
};

ResolvedInitialStatePlan resolveInitialStatePlan(const SimulationConfig &config, std::ostream &log);
InitialStateConfig buildInitialStateConfig(const SimulationConfig &config);

#endif // GRAVITY_ENGINE_INCLUDE_SERVER_SIMULATIONINITCONFIG_HPP_
