#ifndef SIMULATIONARGS_HPP_
#define SIMULATIONARGS_HPP_

#include "sim/SimulationConfig.hpp"

#include <ostream>
#include <string>
#include <vector>

struct RuntimeArgs {
    std::string configPath = "simulation.ini";
    int targetSteps = 1000;
    bool exportOnExit = true;
    bool saveConfig = false;
    bool showHelp = false;
    std::vector<std::string> positional;
};

std::string findConfigPathArg(int argc, char **argv, const std::string &fallback = "simulation.ini");
void applyArgsToConfig(int argc, char **argv, SimulationConfig &config, RuntimeArgs &runtime, std::ostream &warnings);
void printUsage(std::ostream &out, const char *programName, bool headlessMode);

#endif /* !SIMULATIONARGS_HPP_ */

