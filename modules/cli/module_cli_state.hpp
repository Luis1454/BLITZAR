#pragma once

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
