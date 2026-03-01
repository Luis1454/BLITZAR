#include "config/SimulationArgs.hpp"
#include "config/SimulationArgsCoreOptions.hpp"
#include "config/SimulationArgsFrontendOptions.hpp"
#include "config/SimulationArgsInitOptions.hpp"
#include "config/SimulationArgsParse.hpp"

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
        if (SimulationArgsFrontendOptions::apply(key, value, config, warnings)) {
            continue;
        }
        if (SimulationArgsInitOptions::apply(key, value, config, runtime, warnings)) {
            continue;
        }

        runtime.hasArgumentError = true;
        warnings << "[args] unknown option: " << key << "\n";
    }
}

void printUsage(std::ostream &out, std::string_view programName, bool headlessMode)
{
    out << "Usage: " << programName << " [options]\n";
    out << "Common options:\n";
    out << "  --config <path>\n";
    out << "  --particle-count <n>\n";
    out << "  --dt <float>\n";
    out << "  --solver <pairwise_cuda|octree_gpu|octree_cpu>\n";
    out << "  --integrator <euler|rk4>\n";
    out << "  --octree-theta <float>\n";
    out << "  --octree-softening <float>\n";
    out << "  --frontend-particle-cap <n>\n";
    out << "  --zoom <float>\n";
    out << "  --luminosity <0..255>\n";
    out << "  --ui-fps <n>\n";
    out << "  --backend-timeout-ms <10..60000>\n";
    out << "  --backend-command-timeout-ms <10..60000>\n";
    out << "  --backend-status-timeout-ms <10..60000>\n";
    out << "  --backend-snapshot-timeout-ms <10..60000>\n";
    out << "  --export-directory <path>\n";
    out << "  --export-format <vtk|vtk_binary|xyz|bin>\n";
    out << "  --input-file <path>\n";
    out << "  --input-format <auto|vtk|vtk_binary|xyz|bin>\n";
    out << "  --init-config-style <preset|detailed>\n";
    out << "  --preset-structure <disk_orbit|random_cloud|file>\n";
    out << "  --structure <disk_orbit|random_cloud|file> (alias)\n";
    out << "  --preset-size <float>\n";
    out << "  --size <float> (alias)\n";
    out << "  --velocity-temperature <float>\n";
    out << "  --particle-temperature <float>\n";
    out << "  --thermal-ambient <float>\n";
    out << "  --thermal-specific-heat <float>\n";
    out << "  --thermal-heating <float>\n";
    out << "  --thermal-radiation <float>\n";
    out << "  --init-mode <disk_orbit|random_cloud|file>\n";
    out << "  --init-seed <n>\n";
    out << "  --init-include-central-body <true|false>\n";
    out << "  --init-central-mass <float>\n";
    out << "  --init-central-x <float>\n";
    out << "  --init-central-y <float>\n";
    out << "  --init-central-z <float>\n";
    out << "  --init-central-vx <float>\n";
    out << "  --init-central-vy <float>\n";
    out << "  --init-central-vz <float>\n";
    out << "  --init-disk-mass <float>\n";
    out << "  --init-disk-radius-min <float>\n";
    out << "  --init-disk-radius-max <float>\n";
    out << "  --init-disk-thickness <float>\n";
    out << "  --init-velocity-scale <float>\n";
    out << "  --init-cloud-half-extent <float>\n";
    out << "  --init-cloud-speed <float>\n";
    out << "  --init-particle-mass <float>\n";
    out << "  --sph <true|false>\n";
    out << "  --sph-h <float>\n";
    out << "  --sph-rest-density <float>\n";
    out << "  --sph-gas-constant <float>\n";
    out << "  --sph-viscosity <float>\n";
    out << "  --energy-every <n>\n";
    out << "  --energy-sample-limit <n>\n";
    if (headlessMode) {
        out << "  --target-steps <n>\n";
        out << "  --export-on-exit <true|false>\n";
        out << "  --no-export-on-exit\n";
        out << "  env: GRAVITY_AUTO_SOLVER_FALLBACK=1 to auto-switch pairwise->octree_gpu for huge N\n";
    }
    out << "  --save-config\n";
    out << "  --help\n";
}
