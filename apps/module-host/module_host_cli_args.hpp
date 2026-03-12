#ifndef GRAVITY_APPS_MODULE_HOST_MODULE_HOST_CLI_ARGS_HPP_
#define GRAVITY_APPS_MODULE_HOST_MODULE_HOST_CLI_ARGS_HPP_

#include "apps/module-host/module_host_cli.hpp"

#include <string>
#include <string_view>

namespace grav_module_host {

class ModuleHostCliArgs final {
public:
    static bool parseArgs(int argc, char **argv, HostOptions &outOptions, std::string &outError);
    static void printHelp(std::string_view programName);
};

} // namespace grav_module_host

#endif // GRAVITY_APPS_MODULE_HOST_MODULE_HOST_CLI_ARGS_HPP_
