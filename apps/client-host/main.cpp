/*
 * @file apps/client-host/main.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Application entry points and host executables for BLITZAR.
 */

#include "apps/client-host/client_host_cli.hpp"
#include <exception>
#include <iostream>
#include <string>

/*
 * @brief Documents the main operation contract.
 * @param argc Input value used by this contract.
 * @param argv Input value used by this contract.
 * @return int value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
int main(int argc, char** argv)
{
    try {
        const std::string programName = (argc > 0 && argv != nullptr && argv[0] != nullptr)
                                            ? std::string(argv[0])
                                            : std::string("blitzar-client");
        grav_client_host::HostOptions options{};
        std::string parseError;
        if (!grav_client_host::ClientHostCli::parseArgs(argc, argv, options, parseError)) {
            std::cerr << "[client-host] " << parseError << "\n";
            grav_client_host::ClientHostCli::printHelp(programName);
            return 2;
        }
        if (options.showHelp) {
            grav_client_host::ClientHostCli::printHelp(programName);
            return 0;
        }
        return grav_client_host::ClientHostCli::run(options, programName);
    }
    catch (const std::exception& ex) {
        std::cerr << "[client-host] fatal: " << ex.what() << "\n";
        return 1;
    }
    catch (...) {
        std::cerr << "[client-host] fatal: unknown exception\n";
        return 1;
    }
}
