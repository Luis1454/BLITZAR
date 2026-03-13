#ifndef GRAVITY_TESTS_SUPPORT_CLIENT_UTILS_HPP_
#define GRAVITY_TESTS_SUPPORT_CLIENT_UTILS_HPP_

#include "client/ClientServerBridge.hpp"

#include <cstdint>
#include <string>

namespace testsupport {

grav_client::ClientTransportArgs makeTransport(std::uint16_t port, const std::string &serverExecutable);

} // namespace testsupport


#endif // GRAVITY_TESTS_SUPPORT_CLIENT_UTILS_HPP_
