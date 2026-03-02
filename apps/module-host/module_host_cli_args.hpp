#pragma once

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
