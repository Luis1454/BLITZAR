/*
 * @file tests/support/client_utils.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#ifndef BLITZAR_TESTS_SUPPORT_CLIENT_UTILS_HPP_
#define BLITZAR_TESTS_SUPPORT_CLIENT_UTILS_HPP_
#include "client/ClientServerBridge.hpp"
#include <cstdint>
#include <string>

namespace testsupport {
bltzr_client::ClientTransportArgs makeTransport(std::uint16_t port,
                                                const std::string& serverExecutable);
} // namespace testsupport
#endif // BLITZAR_TESTS_SUPPORT_CLIENT_UTILS_HPP_
