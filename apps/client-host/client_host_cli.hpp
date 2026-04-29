/*
 * @file apps/client-host/client_host_cli.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Application entry points and host executables for BLITZAR.
 */

#ifndef BLITZAR_APPS_MODULE_HOST_MODULE_HOST_CLI_HPP_
#define BLITZAR_APPS_MODULE_HOST_MODULE_HOST_CLI_HPP_
#include <string>
#include <string_view>

namespace bltzr_client_host {
struct HostOptions {
    std::string configPath = "simulation.ini";
    std::string moduleSpecifier = "qt";
    std::string scriptPath;
    bool showHelp = false;
    bool validateOnly = false;
    bool exitAfterModule = false; // Exit when module closes (default: wait for GUI modules)
};

class ClientHostCli final {
public:
    static bool parseArgs(int argc, char** argv, HostOptions& outOptions, std::string& outError);
    static void printHelp(std::string_view programName);
    static int run(const HostOptions& options, std::string_view programName);
    [[nodiscard]] static bool liveReloadEnabled() noexcept;
};
} // namespace bltzr_client_host
#endif // BLITZAR_APPS_MODULE_HOST_MODULE_HOST_CLI_HPP_
