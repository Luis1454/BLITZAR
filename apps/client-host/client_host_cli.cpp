// File: apps/client-host/client_host_cli.cpp
// Purpose: Application entry point or host support for BLITZAR executables.

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "apps/client-host/client_host_cli.hpp"
#include "apps/client-host/client_host_cli_args.hpp"
#include "apps/client-host/client_host_cli_text.hpp"
#include "apps/client-host/client_host_module_ops.hpp"
#include "client/ClientModuleHandle.hpp"
#include "command/CommandBatchRunner.hpp"
#include "command/CommandContext.hpp"
#include "config/SimulationConfig.hpp"
#include "config/SimulationScenarioValidation.hpp"

namespace grav_client_host {
/// Description: Defines the ClientHostCliLocal data or behavior contract.
class ClientHostCliLocal final {
public:
    static constexpr bool kLiveReloadEnabled = GRAVITY_PROFILE_IS_PROD == 0;

    static int run(const HostOptions& options, std::string_view programName)
    {
        if (options.validateOnly) {
            const SimulationConfig config = SimulationConfig::loadOrCreate(options.configPath);
            const grav_config::ScenarioValidationReport report =
                grav_config::SimulationScenarioValidation::evaluate(config);
            std::cout << grav_config::SimulationScenarioValidation::renderText(report) << "\n";
            return report.validForRun ? 0 : 3;
        }
        if (!options.scriptPath.empty()) {
            grav_cmd::ServerClientCommandTransport transport(150);
            grav_cmd::CommandSessionState session{};
            session.configPath = options.configPath;
            session.config = SimulationConfig::loadOrCreate(options.configPath);
            grav_cmd::CommandExecutionContext context{
                transport, session, grav_cmd::CommandExecutionMode::Batch, std::cout};
            const grav_cmd::CommandResult result =
                grav_cmd::CommandBatchRunner::runScriptFile(options.scriptPath, context);
            if (!result.ok) {
                std::cerr << "[client-host] " << result.message << "\n";
                return 4;
            }
            return 0;
        }
        const std::vector<std::filesystem::path> searchRoots =
            ClientHostModuleOps::buildSearchRoots(programName);
        grav_module::ClientModuleHandle module{};
        std::string currentModuleSpecifier = options.moduleSpecifier;
        const std::string resolvedInitialModulePath =
            ClientHostModuleOps::resolveModuleSpecifier(options.moduleSpecifier, searchRoots);
        const std::string expectedInitialModuleId =
            ClientHostModuleOps::expectedModuleIdForSpecifier(options.moduleSpecifier);
        std::string loadError;
        if (!module.load(resolvedInitialModulePath, options.configPath, expectedInitialModuleId,
                         loadError)) {
            std::cerr << "[client-host] failed to load module '" << options.moduleSpecifier
                      << "': " << loadError << "\n";
            return 1;
        }
        std::cout << "[client-host] loaded: " << module.moduleName() << " (" << module.loadedPath()
                  << ")\n";
        ClientHostCliArgs::printHelp(programName);
        bool keepRunning = true;
        std::string line;
        while (keepRunning) {
            std::cout << "client-host> " << std::flush;
            if (!std::getline(std::cin, line)) {
                if (options.waitForModule) {
                    bool moduleKeepRunning = true;
                    std::string commandError;
                    if (!module.handleCommand("wait", moduleKeepRunning, commandError)) {
                        std::cerr << "[client-host] wait failed: " << commandError << "\n";
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
                           grav_module::ClientModuleHandle& module,
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
            ClientHostCliArgs::printHelp(programName);
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

    static void printCurrentModule(const grav_module::ClientModuleHandle& module,
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

/// Description: Executes the parseArgs operation.
bool ClientHostCli::parseArgs(int argc, char** argv, HostOptions& outOptions, std::string& outError)
{
    return ClientHostCliArgs::parseArgs(argc, argv, outOptions, outError);
}

/// Description: Describes the live reload enabled operation contract.
bool ClientHostCli::liveReloadEnabled() noexcept
{
    return ClientHostCliLocal::kLiveReloadEnabled;
}

/// Description: Executes the printHelp operation.
void ClientHostCli::printHelp(std::string_view programName)
{
    ClientHostCliArgs::printHelp(programName);
}

/// Description: Executes the run operation.
int ClientHostCli::run(const HostOptions& options, std::string_view programName)
{
    return ClientHostCliLocal::run(options, programName);
}

} // namespace grav_client_host
