/*
 * @file engine/src/config/SimulationArgs.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Configuration parsing, validation, and serialization implementation.
 */

#include "config/SimulationArgs.hpp"
#include "config/SimulationArgsClientOptions.hpp"
#include "config/SimulationArgsCoreOptions.hpp"
#include "config/SimulationArgsInitOptions.hpp"
#include "config/SimulationArgsParse.hpp"
#include "config/SimulationConfig.hpp"
#include "config/SimulationModes.hpp"
#include "config/SimulationOptionRegistry.hpp"
#include "config/SimulationScenarioValidation.hpp"

#include <ostream>
#include <string>

/*
 * @brief Documents the find config path arg operation contract.
 * @param args Input value used by this contract.
 * @param fallback Input value used by this contract.
 * @return std::string value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
std::string findConfigPathArg(const std::vector<std::string_view>& args,
                              const std::string& fallback)
{
    for (std::size_t i = 1; i < args.size(); ++i) {
        if (args[i].empty()) {
            continue;
        }
        std::string key;
        std::string value;
        if (!SimulationArgsParse::splitOption(std::string(args[i]), key, value)) {
            continue;
        }
        if (key != "--config") {
            continue;
        }
        if (!value.empty()) {
            return value;
        }
        if (i + 1 < args.size() && !args[i + 1].empty()) {
            return std::string(args[i + 1]);
        }
        return fallback;
    }
    return fallback;
}

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
                       RuntimeArgs& runtime, std::ostream& warnings)
{
    runtime.configPath = findConfigPathArg(args, runtime.configPath);
    for (std::size_t i = 1; i < args.size(); ++i) {
        if (args[i].empty()) {
            continue;
        }
        const std::string raw(args[i]);
        if (raw == "-h") {
            runtime.showHelp = true;
            continue;
        }

        std::string key;
        std::string inlineValue;
        if (!SimulationArgsParse::splitOption(raw, key, inlineValue)) {
            runtime.hasArgumentError = true;
            warnings << "[args] unexpected positional argument: " << raw << "\n";
            continue;
        }

        if (key == "--help" || key == "-h") {
            runtime.showHelp = true;
            continue;
        }
        if (key == "--save-config") {
            runtime.saveConfig = true;
            continue;
        }
        if (key == "--no-export-on-exit") {
            runtime.exportOnExit = false;
            continue;
        }
        if (key == "--export-on-exit") {
            bool parsed = true;
            bool value = true;
            if (!inlineValue.empty()) {
                parsed = SimulationArgsParse::parseBool(inlineValue, value);
            }
            else if (i + 1 < args.size() && !args[i + 1].empty() &&
                     std::string(args[i + 1]).rfind("--", 0) != 0) {
                std::string explicitValue(args[++i]);
                parsed = SimulationArgsParse::parseBool(explicitValue, value);
            }
            if (!parsed) {
                warnings << "[args] invalid bool for --export-on-exit\n";
            }
            else {
                runtime.exportOnExit = value;
            }
            continue;
        }

        std::string value;
        if (!SimulationArgsParse::readValue(args, i, inlineValue, value)) {
            runtime.hasArgumentError = true;
            warnings << "[args] missing value for " << key << "\n";
            continue;
        }

        if (SimulationArgsCoreOptions::apply(key, value, config, runtime, warnings)) {
            continue;
        }
        if (SimulationArgsClientOptions::apply(key, value, config, warnings)) {
            continue;
        }
        if (SimulationArgsInitOptions::apply(key, value, config, runtime, warnings)) {
            continue;
        }

        runtime.hasArgumentError = true;
        warnings << "[args] unknown option: " << key << "\n";
    }

    if (!bltzr_modes::isSupportedSolverIntegratorPair(config.solver, config.integrator)) {
        runtime.hasArgumentError = true;
        if (config.solver == bltzr_modes::kSolverOctreeGpu &&
            config.integrator == bltzr_modes::kIntegratorRk4) {
            warnings << "[args] unsupported solver/integrator combination: solver=octree_gpu does "
                        "not support rk4\n";
        }
        else {
            warnings << "[args] unsupported solver/integrator combination\n";
        }
    }

    const bltzr_config::ScenarioValidationReport report =
        bltzr_config::SimulationScenarioValidation::evaluate(config);
    if (report.errorCount != 0u || report.warningCount != 0u) {
        warnings << bltzr_config::SimulationScenarioValidation::renderText(report) << "\n";
        if (!report.validForRun) {
            runtime.hasArgumentError = true;
        }
    }
}

/*
 * @brief Documents the print usage operation contract.
 * @param out Input value used by this contract.
 * @param programName Input value used by this contract.
 * @param headlessMode Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void printUsage(std::ostream& out, std::string_view programName, bool headlessMode)
{
    out << "Usage: " << programName << " [options]\n";
    out << "Common options:\n";
    out << "  --config <path>\n";
    bltzr_config::printCliUsage(out, bltzr_config::SimulationOptionGroup::Core);
    bltzr_config::printCliUsage(out, bltzr_config::SimulationOptionGroup::Client);
    bltzr_config::printCliUsage(out, bltzr_config::SimulationOptionGroup::InitState);
    bltzr_config::printCliUsage(out, bltzr_config::SimulationOptionGroup::Fluid);
    if (headlessMode) {
        out << "  --target-steps <n>\n";
        out << "  --export-on-exit <true|false>\n";
        out << "  --no-export-on-exit\n";
        out << "  env: BLITZAR_AUTO_SOLVER_FALLBACK=1 to auto-switch pairwise->octree_gpu for huge "
               "N\n";
    }
    out << "  --save-config\n";
    out << "  --help\n";
}
