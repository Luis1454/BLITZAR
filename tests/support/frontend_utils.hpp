#pragma once

#include "frontend/FrontendBackendBridge.hpp"

#include <cstdint>
#include <string>

namespace testsupport {

grav_frontend::FrontendTransportArgs makeTransport(std::uint16_t port, const std::string &backendExecutable);

} // namespace testsupport

