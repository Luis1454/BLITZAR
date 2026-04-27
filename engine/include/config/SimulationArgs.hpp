/*
 * @file engine/include/config/SimulationArgs.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Public configuration interfaces and validation contracts for simulation setup.
 */

#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGS_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGS_HPP_
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>
/*
 * @brief Defines the simulation config type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct SimulationConfig;

/*
 * @brief Defines the runtime args type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct RuntimeArgs {
    std::string configPath = "simulation.ini";
    int targetSteps = 1000;
    bool exportOnExit = false;
    bool saveConfig = false;
    bool showHelp = false;
    bool hasArgumentError = false;
};

/*
 * @brief Documents the find config path arg operation contract.
 * @param args Input value used by this contract.
 * @param fallback Input value used by this contract.
 * @return std::string value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
std::string findConfigPathArg(const std::vector<std::string_view>& args,
                              const std::string& fallback = "simulation.ini");
/*
 * @brief Documents the apply args to config operation contract.
 * @param args Input value used by this contract.
 * @param config Input value used by this contract.
 * @param runtime Input value used by this contract.
 * @param warnings Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void applyArgsToConfig(const std::vector<std::string_view>& args, SimulationConfig& config,
                       RuntimeArgs& runtime, std::ostream& warnings);
/*
 * @brief Documents the print usage operation contract.
 * @param out Input value used by this contract.
 * @param programName Input value used by this contract.
 * @param headlessMode Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void printUsage(std::ostream& out, std::string_view programName, bool headlessMode);
#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGS_HPP_
