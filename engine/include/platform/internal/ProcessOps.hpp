// File: engine/include/platform/internal/ProcessOps.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_PLATFORM_INTERNAL_PROCESSOPS_HPP_
#define GRAVITY_ENGINE_INCLUDE_PLATFORM_INTERNAL_PROCESSOPS_HPP_
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace grav_platform_detail {
typedef std::uintptr_t NativeProcessHandle;
/// Description: Describes the launch process operation contract.
bool launchProcess(const std::string& executable, const std::vector<std::string>& args,
                   bool createNewConsole, NativeProcessHandle& outHandle, std::int64_t& outPid,
                   std::string& outError);
/// Description: Describes the terminate process operation contract.
bool terminateProcess(NativeProcessHandle& handle, std::int64_t& pid, std::uint32_t waitMs,
                      std::string& outError);
/// Description: Executes the isProcessRunning operation.
bool isProcessRunning(NativeProcessHandle handle, std::int64_t pid);
/// Description: Executes the clearProcessHandle operation.
void clearProcessHandle(NativeProcessHandle& handle, std::int64_t& pid);
/// Description: Executes the formatProcessId operation.
std::string formatProcessId(std::int64_t pid);
/// Description: Describes the launch detached process operation contract.
bool launchDetachedProcess(const std::string& executable, const std::vector<std::string>& args,
                           std::string& outError);
/// Description: Describes the run process blocking operation contract.
int runProcessBlocking(const std::string& executable, const std::vector<std::string>& args,
                       bool createNewConsole, std::string& outError);
} // namespace grav_platform_detail
#endif // GRAVITY_ENGINE_INCLUDE_PLATFORM_INTERNAL_PROCESSOPS_HPP_
