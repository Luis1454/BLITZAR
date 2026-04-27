/*
 * @file engine/src/config/SimulationConfig.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Configuration parsing, validation, and serialization implementation.
 */

#include "config/SimulationConfig.hpp"
#include "config/SimulationConfigDirective.hpp"
#include "config/SimulationModes.hpp"
#include "config/SimulationOptionRegistry.hpp"
#include "config/SimulationPerformanceProfile.hpp"
#include "config/SimulationScenarioValidation.hpp"
#include "protocol/ServerProtocol.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
static_assert(grav_protocol::kSnapshotDefaultPoints == 4096u);

/*
 * @brief Documents the trim operation contract.
 * @param value Input value used by this contract.
 * @return std::string value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static std::string trim(const std::string& value)
{
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) {
                         return std::isspace(c) != 0;
                     }).base();
    if (begin >= end)
        return {};
    return std::string(begin, end);
}

/*
 * @brief Documents the defaults operation contract.
 * @param None This contract does not take explicit parameters.
 * @return SimulationConfig SimulationConfig:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
SimulationConfig SimulationConfig::defaults()
{
    SimulationConfig config{};
    grav_config::applyPerformanceProfile(config);
    return config;
}

/*
 * @brief Documents the load or create operation contract.
 * @param path Input value used by this contract.
 * @return SimulationConfig SimulationConfig:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
SimulationConfig SimulationConfig::loadOrCreate(const std::string& path)
{
    SimulationConfig config = defaults();
    std::ifstream in(path);
    if (!in.is_open()) {
        config.save(path);
        return config;
    }
    std::string line;
    while (std::getline(in, line)) {
        const std::string stripped = trim(line);
        if (stripped.empty() || stripped[0] == '#' || stripped[0] == ';') {
            continue;
        }
        if (grav_config::SimulationConfigDirective::applyLine(stripped, config, std::cerr)) {
            continue;
        }
        const std::size_t eq = stripped.find('=');
        if (eq == std::string::npos) {
            std::cerr << "[config] invalid line ignored: " << stripped << "\n";
            continue;
        }
        const std::string key = trim(stripped.substr(0, eq));
        const std::string value = trim(stripped.substr(eq + 1));
        if (!grav_config::applyIniOption(key, value, config, std::cerr)) {
            std::cerr << "[config] unknown key ignored: " << key << "\n";
        }
    }
    if (!grav_modes::isSupportedSolverIntegratorPair(config.solver, config.integrator)) {
        config.integrator = std::string(grav_modes::kIntegratorEuler);
        std::cerr << "[config] unsupported solver/integrator combination: solver=octree_gpu "
                     "requires integrator=euler\n";
    }
    const grav_config::ScenarioValidationReport report =
        grav_config::SimulationScenarioValidation::evaluate(config);
    if (report.errorCount != 0u || report.warningCount != 0u) {
        std::cerr << grav_config::SimulationScenarioValidation::renderText(report) << "\n";
    }
    return config;
}

/*
 * @brief Documents the save operation contract.
 * @param path Input value used by this contract.
 * @return bool SimulationConfig:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool SimulationConfig::save(const std::string& path) const
{
    std::filesystem::path fsPath(path);
    if (fsPath.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(fsPath.parent_path(), ec);
    }
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }
    grav_config::SimulationConfigDirective::write(out, *this);
    return true;
}
