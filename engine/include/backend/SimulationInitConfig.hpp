#ifndef SIMULATIONINITCONFIG_HPP_
#define SIMULATIONINITCONFIG_HPP_

#include "backend/SimulationBackend.hpp"
#include "config/SimulationConfig.hpp"

#include <iosfwd>
#include <string>

struct ResolvedInitialStatePlan {
    InitialStateConfig config;
    std::string inputFile;
    std::string inputFormat = "auto";
    std::string summary;
};

ResolvedInitialStatePlan resolveInitialStatePlan(const SimulationConfig &config, std::ostream &log);
InitialStateConfig buildInitialStateConfig(const SimulationConfig &config);

#endif /* !SIMULATIONINITCONFIG_HPP_ */


