/*
 * @file apps/client-host/include/CliArgs.hpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Application entry points and host executables for BLITZAR.
 */

#ifndef BLITZAR_APPS_CLIENT_HOST_CLIARGS_HPP_
#define BLITZAR_APPS_CLIENT_HOST_CLIARGS_HPP_
#include "Cli.hpp"
#include <string>
#include <string_view>

namespace bltzr_client_host {
bool parseArgs(int argc, char** argv, HostOptions& outOptions, std::string& outError);
void printHelp(std::string_view programName);
} // namespace bltzr_client_host
#endif // BLITZAR_APPS_CLIENT_HOST_CLIARGS_HPP_
