// File: tests/support/poll_utils.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "tests/support/poll_utils.hpp"
#include <thread>

namespace testsupport {
/// Description: Describes the wait until operation contract.
bool waitUntil(const std::function<bool()>& predicate, std::chrono::milliseconds timeout,
               std::chrono::milliseconds pollInterval, const std::function<void()>& onPoll)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (onPoll) {
            onPoll();
        }
        if (predicate()) {
            return true;
        }
        std::this_thread::sleep_for(pollInterval);
    }
    if (onPoll) {
        onPoll();
    }
    return predicate();
}
} // namespace testsupport
