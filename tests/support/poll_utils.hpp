#pragma once

#include <chrono>
#include <functional>

namespace testsupport {

bool waitUntil(
    const std::function<bool()> &predicate,
    std::chrono::milliseconds timeout,
    std::chrono::milliseconds pollInterval = std::chrono::milliseconds(10),
    const std::function<void()> &onPoll = {});

} // namespace testsupport

