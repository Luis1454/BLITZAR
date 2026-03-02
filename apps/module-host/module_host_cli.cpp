#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "apps/module-host/module_host_cli.hpp"
#include "apps/module-host/module_host_cli_args.hpp"
#include "apps/module-host/module_host_cli_text.hpp"
#include "apps/module-host/module_host_module_ops.hpp"
#include "frontend/FrontendModuleHandle.hpp"

namespace grav_module_host {

class ModuleHostCliLocal final {
public:
    static int run(const HostOptions &options, std::string_view programName)
    {
        const std::vector<std::filesystem::path> searchRoots = ModuleHostModuleOps::buildSearchRoots(programName);
        grav_module::FrontendModuleHandle module{};
        std::string currentModuleSpecifier = options.moduleSpecifier;
        const std::string resolvedInitialModulePath =
            ModuleHostModuleOps::resolveModuleSpecifier(options.moduleSpecifier, searchRoots);
        std::string loadError;
        if (!module.load(resolvedInitialModulePath, options.configPath, loadError)) {
            std::cerr << "[module-host] failed to load module '" << options.moduleSpecifier
                      << "': " << loadError << "\n";
            return 1;
        }

        std::cout << "[module-host] loaded: " << module.moduleName()
                  << " (" << module.loadedPath() << ")\n";
        ModuleHostCliArgs::printHelp(programName);

        bool keepRunning = true;
        std::string line;
        while (keepRunning) {
            std::cout << "module-host> " << std::flush;
            if (!std::getline(std::cin, line)) {
                break;
            }
            if (!handleLine(line, programName, options, searchRoots, module, currentModuleSpecifier, keepRunning)) {
                continue;
            }
        }

        module.unload();
        return 0;
    }

private:
    static bool handleLine(
        const std::string &line,
        std::string_view programName,
        const HostOptions &options,
        const std::vector<std::filesystem::path> &searchRoots,
        grav_module::FrontendModuleHandle &module,
        std::string &currentModuleSpecifier,
        bool &keepRunning)
    {
        const std::string trimmed = ModuleHostCliText::trim(line);
        if (trimmed.empty()) {
            return false;
        }
        const std::vector<std::string> tokens = ModuleHostCliText::splitTokens(trimmed);
        if (tokens.empty()) {
            return false;
        }
        if (tokens[0] == "help") {
            ModuleHostCliArgs::printHelp(programName);
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
            if (currentModuleSpecifier.empty()) {
                std::cout << "[module-host] no module specifier to reload\n";
                return false;
            }
            ModuleHostModuleOps::reloadModule(currentModuleSpecifier, options.configPath, searchRoots, module);
            return false;
        }
        if (tokens[0] == "switch") {
            if (tokens.size() < 2u) {
                std::cout << "[module-host] usage: switch <module_alias_or_path>\n";
                return false;
            }
            if (ModuleHostModuleOps::switchModule(tokens[1], options.configPath, searchRoots, module)) {
                currentModuleSpecifier = tokens[1];
            }
            return false;
        }

        bool moduleKeepRunning = true;
        std::string commandError;
        if (!module.handleCommand(trimmed, moduleKeepRunning, commandError)) {
            std::cout << "[module-host] " << commandError << "\n";
            return false;
        }
        if (!moduleKeepRunning) {
            keepRunning = false;
        }
        return true;
    }

    static void printAvailableModules(const std::vector<std::filesystem::path> &searchRoots)
    {
        std::cout << "[module-host] available aliases:\n";
        std::cout << "  cli  -> " << ModuleHostModuleOps::resolveModuleSpecifier("cli", searchRoots) << "\n";
        std::cout << "  gui  -> " << ModuleHostModuleOps::resolveModuleSpecifier("gui", searchRoots) << "\n";
        std::cout << "  echo -> " << ModuleHostModuleOps::resolveModuleSpecifier("echo", searchRoots) << "\n";
        std::cout << "  qt   -> " << ModuleHostModuleOps::resolveModuleSpecifier("qt", searchRoots) << "\n";
    }

    static void printCurrentModule(
        const grav_module::FrontendModuleHandle &module,
        const std::string &currentModuleSpecifier)
    {
        std::cout << "[module-host] current module: " << module.moduleName()
                  << " (" << module.loadedPath() << ")";
        if (!currentModuleSpecifier.empty()) {
            std::cout << " [specifier=" << currentModuleSpecifier << "]";
        }
        std::cout << "\n";
    }
};

bool ModuleHostCli::parseArgs(int argc, char **argv, HostOptions &outOptions, std::string &outError)
{
    return ModuleHostCliArgs::parseArgs(argc, argv, outOptions, outError);
}

void ModuleHostCli::printHelp(std::string_view programName)
{
    ModuleHostCliArgs::printHelp(programName);
}

int ModuleHostCli::run(const HostOptions &options, std::string_view programName)
{
    return ModuleHostCliLocal::run(options, programName);
}

} // namespace grav_module_host
