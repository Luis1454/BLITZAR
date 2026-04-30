/*
 * @file apps/client-host/client_host_cli_args.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Application entry points and host executables for BLITZAR.
 */

#ifndef BLITZAR_APPS_MODULE_HOST_MODULE_HOST_CLI_ARGS_HPP_
#define BLITZAR_APPS_MODULE_HOST_MODULE_HOST_CLI_ARGS_HPP_
#include "apps/client-host/client_host_cli.hpp"
#include <string>
#include <string_view>

namespace bltzr_client_host {
class ClientHostCliArgs final {
public:
    static bool parseArgs(int argc, char** argv, HostOptions& outOptions, std::string& outError);
    static void printHelp(std::string_view programName);
};
} // namespace bltzr_client_host
#endif // BLITZAR_APPS_MODULE_HOST_MODULE_HOST_CLI_ARGS_HPP_
