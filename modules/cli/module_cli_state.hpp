#ifndef GRAVITY_MODULES_CLI_MODULE_CLI_STATE_HPP_
#define GRAVITY_MODULES_CLI_MODULE_CLI_STATE_HPP_

#include <cstdint>
#include <string>

#include "protocol/BackendClient.hpp"

namespace grav_module_cli {

struct ModuleState {
    ModuleState();

    BackendClient client;
    std::string host;
    std::uint16_t port;
};

} // namespace grav_module_cli

#endif // GRAVITY_MODULES_CLI_MODULE_CLI_STATE_HPP_
