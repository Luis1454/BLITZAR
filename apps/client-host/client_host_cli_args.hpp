// File: apps/client-host/client_host_cli_args.hpp
// Purpose: Application entry point or host support for BLITZAR executables.

#ifndef GRAVITY_APPS_MODULE_HOST_MODULE_HOST_CLI_ARGS_HPP_
#define GRAVITY_APPS_MODULE_HOST_MODULE_HOST_CLI_ARGS_HPP_
#include "apps/client-host/client_host_cli.hpp"
#include <string>
#include <string_view>
namespace grav_client_host {
class ClientHostCliArgs final {
public:
    static bool parseArgs(int argc, char** argv, HostOptions& outOptions, std::string& outError);
    static void printHelp(std::string_view programName);
};
} // namespace grav_client_host
#endif // GRAVITY_APPS_MODULE_HOST_MODULE_HOST_CLI_ARGS_HPP_
