#include "src/cpp/platform/internal/ProcessOps.hpp"
#include "sim/PlatformErrors.hpp"
#include "sim/PlatformProcess.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace sim::platform::detail {

bool launchProcess(
    const std::string &executable,
    const std::vector<std::string> &args,
    bool createNewConsole,
    NativeProcessHandle &outHandle,
    std::int64_t &outPid,
    std::string &outError)
{
    outError.clear();
    outHandle = 0u;
    outPid = 0;

    const std::string commandLine = buildProcessCommandLine(executable, args);
    STARTUPINFOA startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo{};
    std::string mutableCmd = commandLine;
    const DWORD flags = createNewConsole ? CREATE_NEW_CONSOLE : 0u;
    if (!CreateProcessA(
            nullptr,
            mutableCmd.data(),
            nullptr,
            nullptr,
            FALSE,
            flags,
            nullptr,
            nullptr,
            &startupInfo,
            &processInfo)) {
        outError = sim::platform::errors::kProcessLaunchFailed;
        return false;
    }
    CloseHandle(processInfo.hThread);
    outHandle = reinterpret_cast<NativeProcessHandle>(processInfo.hProcess);
    outPid = static_cast<std::int64_t>(processInfo.dwProcessId);
    return true;
}

bool terminateProcess(
    NativeProcessHandle &handle,
    std::int64_t &pid,
    std::uint32_t waitMs,
    std::string &outError)
{
    outError.clear();
    if (handle == 0u) {
        pid = 0;
        return true;
    }
    HANDLE processHandle = reinterpret_cast<HANDLE>(handle);
    if (!TerminateProcess(processHandle, 0)) {
        outError = sim::platform::errors::kProcessTerminateFailed;
        return false;
    }
    WaitForSingleObject(processHandle, waitMs);
    CloseHandle(processHandle);
    handle = 0u;
    pid = 0;
    return true;
}

bool isProcessRunning(NativeProcessHandle handle, std::int64_t pid)
{
    (void)pid;
    if (handle == 0u) {
        return false;
    }
    DWORD code = 0u;
    if (!GetExitCodeProcess(reinterpret_cast<HANDLE>(handle), &code)) {
        return false;
    }
    return code == STILL_ACTIVE;
}

void clearProcessHandle(NativeProcessHandle &handle, std::int64_t &pid)
{
    if (handle != 0u) {
        CloseHandle(reinterpret_cast<HANDLE>(handle));
        handle = 0u;
    }
    pid = 0;
}

std::string formatProcessId(std::int64_t pid)
{
    return std::to_string(pid);
}

bool launchDetachedProcess(
    const std::string &executable,
    const std::vector<std::string> &args,
    std::string &outError)
{
    outError.clear();
    const std::string commandLine = buildProcessCommandLine(executable, args);
    STARTUPINFOA startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo{};
    std::string mutableCmd = commandLine;
    constexpr DWORD kDetachedFlags = DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP | CREATE_NO_WINDOW;
    if (!CreateProcessA(
            nullptr,
            mutableCmd.data(),
            nullptr,
            nullptr,
            FALSE,
            kDetachedFlags,
            nullptr,
            nullptr,
            &startupInfo,
            &processInfo)) {
        outError = sim::platform::errors::kProcessLaunchFailed;
        return false;
    }
    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    return true;
}

} // namespace sim::platform::detail
