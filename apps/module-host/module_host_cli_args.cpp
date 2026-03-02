#include <iostream>

#include "apps/module-host/module_host_cli_args.hpp"

namespace grav_module_host {

class ModuleHostCliArgsLocal final {
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
            << "[module-host] commands:\n"
            << "  help\n"
            << "  modules\n"
            << "  module\n"
            << "  reload\n"
            << "  switch <module_alias_or_path>\n"
            << "  quit | exit\n"
            << "  <any other line> -> forwarded to loaded module\n"
            << "[module-host] aliases: cli, gui, echo, qt\n";
    }
};

bool ModuleHostCliArgs::parseArgs(int argc, char **argv, HostOptions &outOptions, std::string &outError)
{
    return ModuleHostCliArgsLocal::parseArgs(argc, argv, outOptions, outError);
}

void ModuleHostCliArgs::printHelp(std::string_view programName)
{
    ModuleHostCliArgsLocal::printHelp(programName);
}

} // namespace grav_module_host
