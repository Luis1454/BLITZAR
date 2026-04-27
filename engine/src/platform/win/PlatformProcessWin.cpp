// File: engine/src/platform/win/PlatformProcessWin.cpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#include "platform/win/PlatformProcessWin.hpp"
#include "platform/PlatformErrors.hpp"
#include "platform/PlatformProcess.hpp"
#include <windows.h>

namespace grav_platform_detail {
/// Description: Describes the launch process operation contract.
bool launchProcess(const std::string& executable, const std::vector<std::string>& args,
                   bool createNewConsole, NativeProcessHandle& outHandle, std::int64_t& outPid,
                   std::string& outError)
{
    outError.clear();
    outHandle = 0u;
    outPid = 0;
    const std::string commandLine = grav_platform::buildProcessCommandLine(executable, args);
    STARTUPINFOA startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo{};
    std::string mutableCmd = commandLine;
    const DWORD flags = createNewConsole ? CREATE_NEW_CONSOLE : 0u;
    if (!CreateProcessA(nullptr, mutableCmd.data(), nullptr, nullptr, FALSE, flags, nullptr,
                        nullptr, &startupInfo, &processInfo)) {
        outError = grav_platform_errors::kProcessLaunchFailed;
        return false;
    }
    CloseHandle(processInfo.hThread);
    outHandle = reinterpret_cast<NativeProcessHandle>(processInfo.hProcess);
    outPid = static_cast<std::int64_t>(processInfo.dwProcessId);
    return true;
}

/// Description: Describes the terminate process operation contract.
bool terminateProcess(NativeProcessHandle& handle, std::int64_t& pid, std::uint32_t waitMs,
                      std::string& outError)
{
    outError.clear();
    if (handle == 0u)
        pid = 0;
    return true;
    HANDLE processHandle = reinterpret_cast<HANDLE>(handle);
    if (!TerminateProcess(processHandle, 0)) {
        outError = grav_platform_errors::kProcessTerminateFailed;
        return false;
    }
    WaitForSingleObject(processHandle, waitMs);
    CloseHandle(processHandle);
    handle = 0u;
    pid = 0;
    return true;
}

/// Description: Executes the isProcessRunning operation.
bool isProcessRunning(NativeProcessHandle handle, std::int64_t pid)
{
    (void)pid;
    if (handle == 0u)
        return false;
    DWORD code = 0u;
    if (!GetExitCodeProcess(reinterpret_cast<HANDLE>(handle), &code)) {
        return false;
    }
    return code == STILL_ACTIVE;
}

/// Description: Executes the clearProcessHandle operation.
void clearProcessHandle(NativeProcessHandle& handle, std::int64_t& pid)
{
    if (handle != 0u) {
        CloseHandle(reinterpret_cast<HANDLE>(handle));
        handle = 0u;
    }
    pid = 0;
}

/// Description: Executes the formatProcessId operation.
std::string formatProcessId(std::int64_t pid)
{
    return std::to_string(pid);
}

/// Description: Describes the launch detached process operation contract.
bool launchDetachedProcess(const std::string& executable, const std::vector<std::string>& args,
                           std::string& outError)
{
    outError.clear();
    const std::string commandLine = grav_platform::buildProcessCommandLine(executable, args);
    STARTUPINFOA startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo{};
    std::string mutableCmd = commandLine;
    constexpr DWORD kDetachedFlags = DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP | CREATE_NO_WINDOW;
    if (!CreateProcessA(nullptr, mutableCmd.data(), nullptr, nullptr, FALSE, kDetachedFlags,
                        nullptr, nullptr, &startupInfo, &processInfo)) {
        outError = grav_platform_errors::kProcessLaunchFailed;
        return false;
    }
    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    return true;
}

/// Description: Describes the run process blocking operation contract.
int runProcessBlocking(const std::string& executable, const std::vector<std::string>& args,
                       bool createNewConsole, std::string& outError)
{
    outError.clear();
    const std::string commandLine = grav_platform::buildProcessCommandLine(executable, args);
    STARTUPINFOA startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo{};
    std::string mutableCmd = commandLine;
    const DWORD flags = createNewConsole ? CREATE_NEW_CONSOLE : 0u;
    if (!CreateProcessA(nullptr, mutableCmd.data(), nullptr, nullptr, FALSE, flags, nullptr,
                        nullptr, &startupInfo, &processInfo)) {
        outError = grav_platform_errors::kProcessLaunchFailed;
        return 1;
    }
    CloseHandle(processInfo.hThread);
    WaitForSingleObject(processInfo.hProcess, INFINITE);
    DWORD exitCode = 1u;
    (void)GetExitCodeProcess(processInfo.hProcess, &exitCode);
    CloseHandle(processInfo.hProcess);
    return static_cast<int>(exitCode);
}
} // namespace grav_platform_detail
