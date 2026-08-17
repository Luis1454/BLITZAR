/*
 * @file apps/headless/src/main.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Headless simulation entry point.
 */

#include "args/Main.hpp"
#include "Constants.hpp"
#include "Runner.hpp"
#include "core/Config.hpp"
#include "directive/Config.hpp"
#include "env/Base.hpp"
#include "profile/Main.hpp"
#include "profile/Performance.hpp"
#include "validation/Scenario.hpp"
#include "SimulationInitConfig.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

static bool containsHelp(const std::vector<std::string_view>& args)
{
    return std::any_of(args.begin() + (args.empty() ? 0 : 1), args.end(), [](const auto value) {
        return value == "--help" || value == "-h";
    });
}

static void printResolvedCase(const std::string& configPath, const SimulationConfig& config,
                              const ResolvedInitialStatePlan& initPlan,
                              const bltzr_config::ScenarioValidationReport& report)
{
    const std::filesystem::path absoluteConfig =
        std::filesystem::absolute(std::filesystem::path(configPath));
    std::cout << "[headless] config path=" << absoluteConfig.string() << "\n";
    std::cout << "[headless] config source=file\n";
    std::cout << "[headless] simulation profile="
              << (config.simulationProfile.empty() ? "none" : config.simulationProfile) << "\n";
    std::cout << "[headless] " << initPlan.summary << "\n";
    if (initPlan.inputFile.empty()) {
        std::cout << "[headless] input source=generated seed=" << config.initSeed << "\n";
    }
    else {
        const std::filesystem::path inputPath(initPlan.inputFile);
        std::cout << "[headless] input path=" << std::filesystem::absolute(inputPath).string()
                  << " exists=" << (std::filesystem::exists(inputPath) ? "true" : "false")
                  << " format=" << initPlan.inputFormat << "\n";
    }
    std::cout << "[headless] effective configuration begin\n";
    bltzr_config::SimulationConfigDirective::write(std::cout, config);
    std::cout << "[headless] effective configuration end\n";
    if (!report.diagnostics.empty()) {
        std::cout << bltzr_config::SimulationScenarioValidation::renderText(report) << "\n";
    }
}

static void resolveCasePaths(const std::filesystem::path& configPath, SimulationConfig& config)
{
    const std::filesystem::path caseDirectory = std::filesystem::absolute(configPath).parent_path();
    const auto resolve = [&caseDirectory](std::string& value) {
        if (value.empty()) {
            return;
        }
        const std::filesystem::path path(value);
        if (path.is_relative()) {
            value = (caseDirectory / path).lexically_normal().string();
        }
    };
    resolve(config.exportDirectory);
    resolve(config.inputFile);
}

static void resolveExportPath(const std::filesystem::path& configPath, std::string& exportPath)
{
    if (exportPath.empty()) {
        return;
    }
    const std::filesystem::path path(exportPath);
    if (path.is_relative()) {
        exportPath = (std::filesystem::absolute(configPath).parent_path() / path)
                         .lexically_normal()
                         .string();
    }
}
int main(int argc, char** argv)
{
    std::vector<std::string_view> args;
    args.reserve(static_cast<std::size_t>(std::max(0, argc)));
    for (int i = 0; i < argc; ++i)
        args.emplace_back(argv[i] ? argv[i] : "");
    RuntimeArgs runtime;
    runtime.configPath = findConfigPathArg(args, "simulation.ini");
    if (containsHelp(args)) {
        printUsage(std::cout, args.empty() ? std::string_view("blitzar-headless") : args[0], true);
        return 0;
    }
    const std::filesystem::path configPath(runtime.configPath);
    if (!std::filesystem::is_regular_file(configPath)) {
        std::cerr << "[headless] configuration file not found: "
                  << std::filesystem::absolute(configPath).string() << "\n";
        std::cerr << "[headless] create a case file or pass --config <path>\n";
        return 3;
    }
    SimulationConfig config = SimulationConfig::loadOrCreate(runtime.configPath);
    bltzr_config::applySimulationProfile(config);
    bltzr_config::applyPerformanceProfile(config);
    applyArgsToConfig(args, config, runtime, std::cerr);
    if (runtime.hasArgumentError) {
        printUsage(std::cerr, args.empty() ? std::string_view("blitzar-headless") : args[0], true);
        return 2;
    }
    if (runtime.command == RuntimeCommand::Unspecified) {
        std::cerr
            << "[headless] no execution command selected; choose --inspect, --validate, or --run\n";
        printUsage(std::cerr, args.empty() ? std::string_view("blitzar-headless") : args[0], true);
        return 2;
    }
    resolveCasePaths(configPath, config);
    resolveExportPath(configPath, runtime.exportPath);
    if (runtime.saveConfig) {
        if (!config.save(runtime.configPath)) {
            std::cerr << "[headless] failed to save configuration: " << runtime.configPath << "\n";
            return 3;
        }
    }
    const ResolvedInitialStatePlan initPlan = resolveInitialStatePlan(config, std::cerr);
    const bltzr_config::ScenarioValidationReport validation =
        bltzr_config::SimulationScenarioValidation::evaluate(config);
    printResolvedCase(runtime.configPath, config, initPlan, validation);
    if (runtime.command == RuntimeCommand::Inspect) {
        return 0;
    }
    if (runtime.command == RuntimeCommand::Validate) {
        return validation.validForRun ? 0 : 4;
    }

    bool exportOnExit = runtime.exportOnExit;
    constexpr bool kDevProfile = BLITZAR_PROFILE_IS_DEV != 0;
    if (kDevProfile)
        exportOnExit = bltzr_env::getBool("BLITZAR_EXPORT_ON_EXIT", runtime.exportOnExit);
    std::cout << "[headless] batch runner=direct targetSteps=" << runtime.targetSteps
              << " dt=" << config.dt << " solver=" << config.solver
              << " integrator=" << config.integrator
              << " sph=" << (config.sphEnabled ? "on" : "off") << " export=" << config.exportFormat
              << " deterministic=" << (config.deterministicMode ? "on" : "off")
              << " treepm=" << (config.treePmEnabled ? config.treePmModel : "off")
              << " treepm_precision=" << config.treePmPrecision
              << " treepm_assignment=" << config.treePmAssignment
              << " pm_particles=all"
              << " adaptive_dt=" << (config.adaptiveTimeStepsEnabled ? "on" : "off")
              << " adaptive_max_level=" << config.adaptiveTimeStepMaxLevel
              << " adaptive_eta=" << config.adaptiveTimeStepEta
              << " adaptive_cost_guard=" << (config.adaptiveTimeStepCostGuard ? "on" : "off")
              << " export_on_exit=" << (exportOnExit ? "on" : "off") << "\n";
    if (exportOnExit) {
        std::cout << "[headless] export path="
                  << (runtime.exportPath.empty() ? "<timestamped>" : runtime.exportPath) << "\n";
    }
    std::cout << "[headless] " << initPlan.summary << "\n";
    bltzr_batch::RunRequest request{config, initPlan,
                                    static_cast<std::uint32_t>(runtime.targetSteps), exportOnExit,
                                    runtime.exportPath};
    const bltzr_batch::RunResult result = bltzr_batch::Runner().run(request);
    std::cout << "[headless] done particles=" << result.particleCount << " steps=" << result.steps
              << " faulted=" << (result.faulted ? "1" : "0")
              << " simulated_time=" << result.simulatedTime
              << " scale_factor=" << result.cosmologyScaleFactor
              << " solver=" << result.solver << " integrator=" << result.integrator
              << " backend=" << result.executionBackend
              << " init_ms=" << result.initializationMilliseconds
              << " integrate_ms=" << result.integrationMilliseconds
              << " export_ms=" << result.exportMilliseconds
              << " time_ms=" << result.elapsedMilliseconds;
    if (!result.exportPath.empty()) {
        std::cout << " export_path=" << result.exportPath;
    }
    std::cout << "\n";
    if (!result.error.empty()) {
        std::cerr << "[headless] batch error: " << result.error << "\n";
    }
    return result.faulted || !result.error.empty() ? 5 : 0;
}
