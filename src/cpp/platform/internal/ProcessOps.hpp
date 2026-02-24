#ifndef GRAVITY_PLATFORM_INTERNAL_PROCESSOPS_HPP
#define GRAVITY_PLATFORM_INTERNAL_PROCESSOPS_HPP

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace sim::platform::detail {

using NativeProcessHandle = std::uintptr_t;

bool launchProcess(
    const std::string &executable,
    const std::vector<std::string> &args,
    bool createNewConsole,
    NativeProcessHandle &outHandle,
    std::int64_t &outPid,
    std::string &outError);

bool terminateProcess(
    NativeProcessHandle &handle,
    std::int64_t &pid,
    std::uint32_t waitMs,
    std::string &outError);

bool isProcessRunning(NativeProcessHandle handle, std::int64_t pid);
void clearProcessHandle(NativeProcessHandle &handle, std::int64_t &pid);
std::string formatProcessId(std::int64_t pid);

bool launchDetachedProcess(
    const std::string &executable,
    const std::vector<std::string> &args,
    std::string &outError);

int runProcessBlocking(
    const std::string &executable,
    const std::vector<std::string> &args,
    bool createNewConsole,
    std::string &outError);

} // namespace sim::platform::detail

#endif // GRAVITY_PLATFORM_INTERNAL_PROCESSOPS_HPP
