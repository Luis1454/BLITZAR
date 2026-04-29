/*
 * @file tests/support/poll_utils.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#ifndef BLITZAR_TESTS_SUPPORT_POLL_UTILS_HPP_
#define BLITZAR_TESTS_SUPPORT_POLL_UTILS_HPP_
#include <chrono>
#include <functional>

namespace testsupport {
bool waitUntil(const std::function<bool()>& predicate, std::chrono::milliseconds timeout,
               std::chrono::milliseconds pollInterval = std::chrono::milliseconds(10),
               const std::function<void()>& onPoll = {});
} // namespace testsupport
#endif // BLITZAR_TESTS_SUPPORT_POLL_UTILS_HPP_
