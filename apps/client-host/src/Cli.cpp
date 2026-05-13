/*
 * @file apps/client-host/src/Cli.cpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Application entry points and host executables for BLITZAR.
 */

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "Cli.hpp"
#include "CliArgs.hpp"
#include "CliText.hpp"
#include "ModuleOps.hpp"
#include "client/module/Handle.hpp"
#include "command/core/Context.hpp"
#include "command/execution/BatchRunner.hpp"
#include "config/core/Config.hpp"
#include "config/validation/Scenario.hpp"

namespace bltzr_client_host {
class ClientHostCliImpl final {
public:
    static constexpr bool kLiveReloadEnabled = BLITZAR_PROFILE_IS_PROD == 0;

    static int run(const HostOptions& options, std::string_view programName)
    {
        if (options.validateOnly) {
            const SimulationConfig config = SimulationConfig::loadOrCreate(options.configPath);
            const bltzr_config::ScenarioValidationReport report =
                bltzr_config::SimulationScenarioValidation::evaluate(config);
            std::cout << bltzr_config::SimulationScenarioValidation::renderText(report) << "\n";
            return report.validForRun ? 0 : 3;
        }
        if (!options.scriptPath.empty()) {
            bltzr_cmd::ServerTransport transport(150);
            bltzr_cmd::SessionState session{};
            session.configPath = options.configPath;
            session.config = SimulationConfig::loadOrCreate(options.configPath);
            bltzr_cmd::ExecutionContext context{
                transport, session, bltzr_cmd::ExecutionMode::Batch, std::cout};
            const bltzr_cmd::Result result =
                bltzr_cmd::runScriptFile(options.scriptPath, context);
            if (!result.ok) {
                std::cerr << "[client-host] " << result.message << "\n";
                return 4;
            }
            return 0;
        }
        const std::vector<std::filesystem::path> searchRoots =
            ClientHostModuleOps::buildSearchRoots(programName);
        bltzr_module::Handle module{};
        std::string currentModuleSpecifier = options.moduleSpecifier;
        const std::string resolvedInitialModulePath =
            ClientHostModuleOps::resolveModuleSpecifier(options.moduleSpecifier, searchRoots);
        const std::string expectedInitialModuleId =
            ClientHostModuleOps::expectedModuleIdForResolvedSpecifier(options.moduleSpecifier,
                                                                      resolvedInitialModulePath);
        std::string loadError;
        if (!module.load(resolvedInitialModulePath, options.configPath, expectedInitialModuleId,
                         loadError)) {
            std::cerr << "[client-host] failed to load module '" << options.moduleSpecifier
                      << "': " << loadError << "\n";
            return 1;
        }
        std::cout << "[client-host] loaded: " << module.moduleName() << " (" << module.loadedPath()
                  << ")\n";
        printHelp(programName);
        bool keepRunning = true;
        std::string line;
        while (keepRunning) {
            std::cout << "client-host> " << std::flush;
            if (!std::getline(std::cin, line)) {
                if (!options.exitAfterModule) {
                    bool moduleKeepRunning = true;
                    std::string commandError;
                    if (!module.handleCommand("wait", moduleKeepRunning, commandError)) {
                        std::cerr << "[client-host] wait failed: " << commandError << "\n";
                        module.unload();
                        return 1;
                    }
                }
                break;
            }
            if (!handleLine(line, programName, options, searchRoots, module, currentModuleSpecifier,
                            keepRunning)) {
                continue;
            }
        }
        module.unload();
        return 0;
    }

private:
    static bool handleLine(const std::string& line, std::string_view programName,
                           const HostOptions& options,
                           const std::vector<std::filesystem::path>& searchRoots,
                           bltzr_module::Handle& module,
                           std::string& currentModuleSpecifier, bool& keepRunning)
    {
        const std::string trimmed = ClientHostCliText::trim(line);
        if (trimmed.empty()) {
            return false;
        }
        const std::vector<std::string> tokens = ClientHostCliText::splitTokens(trimmed);
        if (tokens.empty()) {
            return false;
        }
        if (tokens[0] == "help") {
            printHelp(programName);
            return false;
        }
        if (tokens[0] == "modules") {
            printAvailableModules(searchRoots);
            return false;
        }
        if (tokens[0] == "module") {
            printCurrentModule(module, currentModuleSpecifier);
            return false;
        }
        if (tokens[0] == "quit" || tokens[0] == "exit") {
            keepRunning = false;
            return false;
        }
        if (tokens[0] == "reload") {
            if (!kLiveReloadEnabled) {
                std::cout << "[client-host] reload is disabled in prod profile\n";
                return false;
            }
            if (currentModuleSpecifier.empty()) {
                std::cout << "[client-host] no module specifier to reload\n";
                return false;
            }
            ClientHostModuleOps::reloadModule(currentModuleSpecifier, options.configPath,
                                              searchRoots, module);
            return false;
        }
        if (tokens[0] == "switch") {
            if (!kLiveReloadEnabled) {
                std::cout << "[client-host] switch is disabled in prod profile\n";
                return false;
            }
            if (tokens.size() < 2u) {
                std::cout << "[client-host] usage: switch <module_alias_or_path>\n";
                return false;
            }
            if (ClientHostModuleOps::switchModule(tokens[1], options.configPath, searchRoots,
                                                  module)) {
                currentModuleSpecifier = tokens[1];
            }
            return false;
        }
        bool moduleKeepRunning = true;
        std::string commandError;
        if (!module.handleCommand(trimmed, moduleKeepRunning, commandError)) {
            std::cout << "[client-host] " << commandError << "\n";
            return false;
        }
        if (!moduleKeepRunning) {
            keepRunning = false;
        }
        return true;
    }

    static void printAvailableModules(const std::vector<std::filesystem::path>& searchRoots)
    {
        std::cout << "[client-host] available aliases:\n";
        std::cout << "  cli  -> " << ClientHostModuleOps::resolveModuleSpecifier("cli", searchRoots)
                  << "\n";
        std::cout << "  gui  -> " << ClientHostModuleOps::resolveModuleSpecifier("gui", searchRoots)
                  << "\n";
        std::cout << "  echo -> "
                  << ClientHostModuleOps::resolveModuleSpecifier("echo", searchRoots) << "\n";
        std::cout << "  qt   -> " << ClientHostModuleOps::resolveModuleSpecifier("qt", searchRoots)
                  << "\n";
    }

    static void printCurrentModule(const bltzr_module::Handle& module,
                                   const std::string& currentModuleSpecifier)
    {
        std::cout << "[client-host] current module: " << module.moduleName() << " ("
                  << module.loadedPath() << ")";
        if (!currentModuleSpecifier.empty()) {
            std::cout << " [specifier=" << currentModuleSpecifier << "]";
        }
        std::cout << "\n";
    }
};

bool ClientHostCli::parseArgs(int argc, char** argv, HostOptions& outOptions, std::string& outError)
{
    return bltzr_client_host::parseArgs(argc, argv, outOptions, outError);
}

bool ClientHostCli::liveReloadEnabled() noexcept
{
    return ClientHostCliImpl::kLiveReloadEnabled;
}

void ClientHostCli::printHelp(std::string_view programName)
{
    bltzr_client_host::printHelp(programName);
}

int ClientHostCli::run(const HostOptions& options, std::string_view programName)
{
    return ClientHostCliImpl::run(options, programName);
}

} // namespace bltzr_client_host
