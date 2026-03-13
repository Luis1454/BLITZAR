#ifndef GRAVITY_APPS_MODULE_HOST_MODULE_HOST_CLI_HPP_
#define GRAVITY_APPS_MODULE_HOST_MODULE_HOST_CLI_HPP_

#include <string>
#include <string_view>

namespace grav_client_host {

struct HostOptions {
    std::string configPath = "simulation.ini";
    std::string moduleSpecifier = "cli";
    bool showHelp = false;
};

class ClientHostCli final {
public:
    static bool parseArgs(int argc, char **argv, HostOptions &outOptions, std::string &outError);
    static void printHelp(std::string_view programName);
    static int run(const HostOptions &options, std::string_view programName);
    [[nodiscard]] static bool liveReloadEnabled() noexcept;
};

} // namespace grav_client_host

#endif // GRAVITY_APPS_MODULE_HOST_MODULE_HOST_CLI_HPP_
