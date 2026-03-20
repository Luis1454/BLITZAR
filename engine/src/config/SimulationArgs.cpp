#include "config/SimulationArgs.hpp"
#include "config/SimulationArgsCoreOptions.hpp"
#include "config/SimulationArgsClientOptions.hpp"
#include "config/SimulationArgsInitOptions.hpp"
#include "config/SimulationArgsParse.hpp"
#include "config/SimulationConfig.hpp"
#include "config/SimulationOptionRegistry.hpp"
#include "config/SimulationModes.hpp"

#include <ostream>
#include <string>

std::string findConfigPathArg(const std::vector<std::string_view> &args, const std::string &fallback)
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

void applyArgsToConfig(
    const std::vector<std::string_view> &args,
    SimulationConfig &config,
    RuntimeArgs &runtime,
    std::ostream &warnings
)
{
    runtime.configPath = findConfigPathArg(args, runtime.configPath);
    const std::string initialSolver = config.solver;
    const std::string initialIntegrator = config.integrator;

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
            } else if (i + 1 < args.size() && !args[i + 1].empty() && std::string(args[i + 1]).rfind("--", 0) != 0) {
                std::string explicitValue(args[++i]);
                parsed = SimulationArgsParse::parseBool(explicitValue, value);
            }
            if (!parsed) {
                warnings << "[args] invalid bool for --export-on-exit\n";
            } else {
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

    if (!grav_modes::isSupportedSolverIntegratorPair(config.solver, config.integrator)) {
        runtime.hasArgumentError = true;
        warnings << "[args] unsupported solver/integrator combination: solver=octree_gpu requires integrator=euler\n";
        config.solver = initialSolver;
        config.integrator = initialIntegrator;
    }
}

void printUsage(std::ostream &out, std::string_view programName, bool headlessMode)
{
    out << "Usage: " << programName << " [options]\n";
    out << "Common options:\n";
    out << "  --config <path>\n";
    grav_config::printCliUsage(out, grav_config::SimulationOptionGroup::Core);
    grav_config::printCliUsage(out, grav_config::SimulationOptionGroup::Client);
    grav_config::printCliUsage(out, grav_config::SimulationOptionGroup::InitState);
    grav_config::printCliUsage(out, grav_config::SimulationOptionGroup::Fluid);
    if (headlessMode) {
        out << "  --target-steps <n>\n";
        out << "  --export-on-exit <true|false>\n";
        out << "  --no-export-on-exit\n";
        out << "  env: GRAVITY_AUTO_SOLVER_FALLBACK=1 to auto-switch pairwise->octree_gpu for huge N\n";
    }
    out << "  --save-config\n";
    out << "  --help\n";
}
