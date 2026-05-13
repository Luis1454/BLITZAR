/*
 * @file engine/include/server/SimulationInitConfig.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Source artifact for the BLITZAR simulation project.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_SERVER_SIMULATIONINITCONFIG_HPP_
#define BLITZAR_ENGINE_INCLUDE_SERVER_SIMULATIONINITCONFIG_HPP_
#include "types/SimulationTypes.hpp"
#include <iosfwd>
#include <string>
/*
 * @brief Defines the simulation config type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct SimulationConfig;

/*
 * @brief Defines the resolved initial state plan type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct ResolvedInitialStatePlan {
    InitialStateConfig config;
    std::string inputFile;
    std::string inputFormat = "auto";
    std::string summary;
};

/*
 * @brief Documents the resolve initial state plan operation contract.
 * @param config Input value used by this contract.
 * @param log Input value used by this contract.
 * @return ResolvedInitialStatePlan value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
ResolvedInitialStatePlan resolveInitialStatePlan(const SimulationConfig& config, std::ostream& log);
/*
 * @brief Documents the build initial state config operation contract.
 * @param config Input value used by this contract.
 * @return InitialStateConfig value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
InitialStateConfig buildInitialStateConfig(const SimulationConfig& config);
#endif // BLITZAR_ENGINE_INCLUDE_SERVER_SIMULATIONINITCONFIG_HPP_
