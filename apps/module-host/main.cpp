#include "apps/module-host/module_host_cli.hpp"

#include <exception>
#include <iostream>
#include <string>

int main(int argc, char **argv)
{
    try {
        const std::string programName = (argc > 0 && argv != nullptr && argv[0] != nullptr)
            ? std::string(argv[0])
            : std::string("myAppModuleHost");

        grav_module_host::HostOptions options{};
        std::string parseError;
        if (!grav_module_host::ModuleHostCli::parseArgs(argc, argv, options, parseError)) {
            std::cerr << "[module-host] " << parseError << "\n";
            grav_module_host::ModuleHostCli::printHelp(programName);
            return 2;
        }
        if (options.showHelp) {
            grav_module_host::ModuleHostCli::printHelp(programName);
            return 0;
        }
        return grav_module_host::ModuleHostCli::run(options, programName);
    } catch (const std::exception &ex) {
        std::cerr << "[module-host] fatal: " << ex.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "[module-host] fatal: unknown exception\n";
        return 1;
    }
}
