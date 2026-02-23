#ifndef SIMULATIONARGS_HPP_
#define SIMULATIONARGS_HPP_

#include "sim/SimulationConfig.hpp"

#include <ostream>
#include <string>
#include <string_view>
#include <vector>

struct RuntimeArgs {
    std::string configPath = "simulation.ini";
    int targetSteps = 1000;
    bool exportOnExit = false;
    bool saveConfig = false;
    bool showHelp = false;
    bool hasArgumentError = false;
};

std::string findConfigPathArg(const std::vector<std::string_view> &args, const std::string &fallback = "simulation.ini");
void applyArgsToConfig(
    const std::vector<std::string_view> &args,
    SimulationConfig &config,
    RuntimeArgs &runtime,
    std::ostream &warnings
);
void printUsage(std::ostream &out, std::string_view programName, bool headlessMode);

#endif /* !SIMULATIONARGS_HPP_ */

