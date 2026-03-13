#include "config/SimulationConfig.hpp"
#include "config/SimulationConfigDirective.hpp"
#include "config/SimulationPerformanceProfile.hpp"
#include "config/SimulationModes.hpp"
#include "config/SimulationOptionRegistry.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

static std::string trim(const std::string &value)
{
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c) != 0; });
    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) { return std::isspace(c) != 0; }).base();
    if (begin >= end) {
        return {};
    }
    return std::string(begin, end);
}

SimulationConfig SimulationConfig::defaults()
{
    SimulationConfig config{};
    grav_config::applyPerformanceProfile(config);
    return config;
}
SimulationConfig SimulationConfig::loadOrCreate(const std::string &path)
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
        std::cerr << "[config] unsupported solver/integrator combination: solver=octree_gpu requires integrator=euler\n";
    }
    return config;
}

bool SimulationConfig::save(const std::string &path) const
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
