// File: tests/support/poll_utils.hpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#ifndef GRAVITY_TESTS_SUPPORT_POLL_UTILS_HPP_
#define GRAVITY_TESTS_SUPPORT_POLL_UTILS_HPP_
#include <chrono>
#include <functional>
namespace testsupport {
bool waitUntil(const std::function<bool()>& predicate, std::chrono::milliseconds timeout,
               std::chrono::milliseconds pollInterval = std::chrono::milliseconds(10),
               const std::function<void()>& onPoll = {});
} // namespace testsupport
#endif // GRAVITY_TESTS_SUPPORT_POLL_UTILS_HPP_
