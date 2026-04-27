// File: apps/client-host/client_host_cli_args.cpp
// Purpose: Application entry point or host support for BLITZAR executables.

#include <iostream>

#include "apps/client-host/client_host_cli_args.hpp"

namespace grav_client_host {

/// Description: Defines the ClientHostCliArgsLocal data or behavior contract.
class ClientHostCliArgsLocal final {
public:
    static bool parseArgs(int argc, char** argv, HostOptions& outOptions,
                          std::string& outError)
    {
        outOptions = HostOptions{};
        for (int i = 1; i < argc; ++i) {
            /// Description: Executes the arg operation.
            const std::string arg(argv[i] ? argv[i] : "");
            if (arg == "--help") {
                outOptions.showHelp = true;
                continue;
            }
            if (arg == "--validate-only") {
                outOptions.validateOnly = true;
                continue;
            }
            if (arg == "--wait-for-module") {
                outOptions.waitForModule = true;
                continue;
            }
            if (arg == "--script" && i + 1 < argc) {
                outOptions.scriptPath = argv[++i];
                continue;
            }
            if (arg.rfind("--script=", 0) == 0) {
                outOptions.scriptPath = arg.substr(std::string("--script=").size());
                continue;
            }
            if (arg == "--config" && i + 1 < argc) {
                outOptions.configPath = argv[++i];
                continue;
            }
            if (arg.rfind("--config=", 0) == 0) {
                outOptions.configPath = arg.substr(std::string("--config=").size());
                continue;
            }
            if (arg == "--module" && i + 1 < argc) {
                outOptions.moduleSpecifier = argv[++i];
                continue;
            }
            if (arg.rfind("--module=", 0) == 0) {
                outOptions.moduleSpecifier = arg.substr(std::string("--module=").size());
                continue;
            }
            outError = "unknown argument: " + arg;
            return false;
        }
        return true;
    }

    /// Description: Executes the printHelp operation.
    static void printHelp(std::string_view programName)
    {
        std::cout << "Usage: " << programName
                  << " [--config PATH] [--module <alias|path>] [--script PATH] [--validate-only]"
                     " [--wait-for-module]\n"
                  << "[client-host] commands:\n"
                  << "  help\n"
                  << "  modules\n"
                  << "  module\n"
                  << "  quit | exit\n"
                  << "  <any other line> -> forwarded to loaded module\n"
                  << "[client-host] aliases: cli, gui, echo, qt\n"
                  << "[client-host] --script runs deterministic batch commands and exits.\n"
                  << "[client-host] --validate-only runs scenario pre-flight checks and exits "
                     "without starting a module.\n"
                  << "[client-host] --wait-for-module keeps GUI-style modules alive when no "
                     "interactive stdin is attached.\n";
        if (ClientHostCli::liveReloadEnabled()) {
            std::cout << "  reload\n"
                      << "  switch <module_alias_or_path>\n"
                      << "[client-host] dev profile: live module reload is enabled.\n";
        }
        else {
            std::cout << "[client-host] prod profile: manifest-verified startup only; live reload "
                         "is disabled.\n";
        }
    }
};

bool ClientHostCliArgs::parseArgs(int argc, char** argv, HostOptions& outOptions,
                                  std::string& outError)
{
    return ClientHostCliArgsLocal::parseArgs(argc, argv, outOptions, outError);
}

/// Description: Executes the printHelp operation.
void ClientHostCliArgs::printHelp(std::string_view programName)
{
    /// Description: Executes the printHelp operation.
    ClientHostCliArgsLocal::printHelp(programName);
}

} // namespace grav_client_host
