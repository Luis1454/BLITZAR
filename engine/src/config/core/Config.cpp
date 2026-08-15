/*
 * @file engine/src/config/SimulationConfig.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Configuration parsing, validation, and serialization implementation.
 */

#include "config/core/Config.hpp"
#include "config/directive/Config.hpp"
#include "config/modes/Normalize.hpp"
#include "config/registry/Main.hpp"
#include "config/profile/Performance.hpp"
#include "config/validation/Scenario.hpp"
#include "protocol/Protocol.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>
static_assert(bltzr_protocol::kSnapshotDefaultPoints == 4096u);

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

static void normalizeSceneObjects(SceneConfig& scene)
{
    const auto addProperty = [](SceneObjectConfig& object, const std::string& property) {
        if (property.empty() ||
            std::find(object.properties.begin(), object.properties.end(), property) !=
                object.properties.end())
            return;
        object.properties.push_back(property);
    };
    for (std::size_t index = 0u; index < scene.objects.size(); ++index) {
        SceneObjectConfig& object = scene.objects[index];
        if (object.id.empty()) {
            object.id = "object_" + std::to_string(index + 1u);
        }
        if (object.isAsset)
            addProperty(object, "asset");
        const auto hadProperty = [&object](const std::string& property) {
            return std::find(object.properties.begin(), object.properties.end(), property) !=
                   object.properties.end();
        };
        if (hadProperty("transform")) {
            object.properties.erase(
                std::remove(object.properties.begin(), object.properties.end(), "transform"),
                object.properties.end());
            addProperty(object, "offset");
            addProperty(object, "rotation");
            addProperty(object, "copies");
            addProperty(object, "mirror");
            addProperty(object, "pivot");
        }
        if (object.offsetX != 0.0f || object.offsetY != 0.0f || object.offsetZ != 0.0f)
            addProperty(object, "offset");
        if (object.rotationX != 0.0f || object.rotationY != 0.0f || object.rotationZ != 0.0f)
            addProperty(object, "rotation");
        if (object.axis != "z" || object.copies != 1u)
            addProperty(object, "copies");
        if (object.mirrorX || object.mirrorY || object.mirrorZ)
            addProperty(object, "mirror");
        if (object.pivot != "world" || object.pivotX != 0.0f || object.pivotY != 0.0f ||
            object.pivotZ != 0.0f)
            addProperty(object, "pivot");
        if (object.type == "particle_system")
            addProperty(object, "particle_system");
    }
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
    bltzr_config::applyPerformanceProfile(config);
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
        if (bltzr_config::SimulationConfigDirective::applyLine(stripped, config, std::cerr)) {
            continue;
        }
        const std::size_t eq = stripped.find('=');
        if (eq == std::string::npos) {
            std::cerr << "[config] invalid line ignored: " << stripped << "\n";
            continue;
        }
        const std::string key = trim(stripped.substr(0, eq));
        const std::string value = trim(stripped.substr(eq + 1));
        if (!bltzr_config::applyIniOption(key, value, config, std::cerr)) {
            std::cerr << "[config] unknown key ignored: " << key << "\n";
        }
    }
    if (!bltzr_modes::isSupportedSolverIntegratorPair(config.solver, config.integrator)) {
        config.integrator = std::string(bltzr_modes::kIntegratorEuler);
        std::cerr << "[config] unsupported solver/integrator combination: solver=octree_gpu "
                     "does not support integrator=rk4\n";
    }
    normalizeSceneObjects(config.scene);
    const bltzr_config::ScenarioValidationReport report =
        bltzr_config::SimulationScenarioValidation::evaluate(config);
    if (report.errorCount != 0u || report.warningCount != 0u) {
        std::cerr << bltzr_config::SimulationScenarioValidation::renderText(report) << "\n";
    }
    return config;
}

bool SimulationConfig::loadStrict(const std::string& path, SimulationConfig& outConfig,
                                  std::string& outError)
{
    std::ifstream in(path);
    if (!in.is_open()) {
        outError = "cannot open INI file: " + path;
        return false;
    }

    SimulationConfig config = defaults();
    std::ostringstream diagnostics;
    std::string line;
    std::size_t lineNumber = 0u;
    while (std::getline(in, line)) {
        ++lineNumber;
        const std::string stripped = trim(line);
        if (stripped.empty() || stripped[0] == '#' || stripped[0] == ';') {
            continue;
        }
        if (bltzr_config::SimulationConfigDirective::applyLine(stripped, config, diagnostics)) {
            continue;
        }
        const std::size_t eq = stripped.find('=');
        if (eq == std::string::npos) {
            diagnostics << "line " << lineNumber
                        << ": expected key=value or directive\n";
            continue;
        }
        const std::string key = trim(stripped.substr(0, eq));
        const std::string value = trim(stripped.substr(eq + 1));
        if (!bltzr_config::applyIniOption(key, value, config, diagnostics)) {
            diagnostics << "line " << lineNumber << ": unknown or invalid option '" << key
                        << "'\n";
        }
    }
    if (!in.eof() && in.fail()) {
        diagnostics << "read failure while loading: " << path << "\n";
    }
    normalizeSceneObjects(config.scene);
    if (!bltzr_modes::isSupportedSolverIntegratorPair(config.solver, config.integrator)) {
        diagnostics << "unsupported solver/integrator combination: solver=" << config.solver
                    << ", integrator=" << config.integrator << "\n";
    }
    const bltzr_config::ScenarioValidationReport report =
        bltzr_config::SimulationScenarioValidation::evaluate(config);
    if (!report.validForRun) {
        diagnostics << bltzr_config::SimulationScenarioValidation::renderText(report) << "\n";
    }
    outError = diagnostics.str();
    if (!outError.empty()) {
        return false;
    }
    outConfig = std::move(config);
    return true;
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
    SimulationConfig serializable = *this;
    normalizeSceneObjects(serializable.scene);
    bltzr_config::SimulationConfigDirective::write(out, serializable);
    return true;
}
