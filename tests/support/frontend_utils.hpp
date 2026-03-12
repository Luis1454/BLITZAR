#ifndef GRAVITY_TESTS_SUPPORT_FRONTEND_UTILS_HPP_
#define GRAVITY_TESTS_SUPPORT_FRONTEND_UTILS_HPP_

#include "frontend/FrontendBackendBridge.hpp"

#include <cstdint>
#include <string>

namespace testsupport {

grav_frontend::FrontendTransportArgs makeTransport(std::uint16_t port, const std::string &backendExecutable);

} // namespace testsupport


#endif // GRAVITY_TESTS_SUPPORT_FRONTEND_UTILS_HPP_
