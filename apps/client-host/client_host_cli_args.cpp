#include <iostream>

#include "apps/client-host/client_host_cli_args.hpp"

namespace grav_client_host {

class ClientHostCliArgsLocal final {
public:
    static bool parseArgs(int argc, char **argv, HostOptions &outOptions, std::string &outError)
    {
        outOptions = HostOptions{};
        for (int i = 1; i < argc; ++i) {
            const std::string arg(argv[i] ? argv[i] : "");
            if (arg == "--help") {
                outOptions.showHelp = true;
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

    static void printHelp(std::string_view programName)
    {
        std::cout
            << "Usage: " << programName << " [--config PATH] [--module <alias|path>]\n"
            << "[client-host] commands:\n"
            << "  help\n"
            << "  modules\n"
            << "  module\n"
            << "  quit | exit\n"
            << "  <any other line> -> forwarded to loaded module\n"
            << "[client-host] aliases: cli, gui, echo, qt\n";
        if (ClientHostCli::liveReloadEnabled()) {
            std::cout
                << "  reload\n"
                << "  switch <module_alias_or_path>\n"
                << "[client-host] dev profile: live module reload is enabled.\n";
        } else {
            std::cout << "[client-host] prod profile: manifest-verified startup only; live reload is disabled.\n";
        }
    }
};

bool ClientHostCliArgs::parseArgs(int argc, char **argv, HostOptions &outOptions, std::string &outError)
{
    return ClientHostCliArgsLocal::parseArgs(argc, argv, outOptions, outError);
}

void ClientHostCliArgs::printHelp(std::string_view programName)
{
    ClientHostCliArgsLocal::printHelp(programName);
}

} // namespace grav_client_host
