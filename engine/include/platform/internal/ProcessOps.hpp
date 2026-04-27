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
bool launchProcess(const std::string& executable, const std::vector<std::string>& args,
                   bool createNewConsole, NativeProcessHandle& outHandle, std::int64_t& outPid,
                   std::string& outError);
bool terminateProcess(NativeProcessHandle& handle, std::int64_t& pid, std::uint32_t waitMs,
                      std::string& outError);
bool isProcessRunning(NativeProcessHandle handle, std::int64_t pid);
void clearProcessHandle(NativeProcessHandle& handle, std::int64_t& pid);
std::string formatProcessId(std::int64_t pid);
bool launchDetachedProcess(const std::string& executable, const std::vector<std::string>& args,
                           std::string& outError);
int runProcessBlocking(const std::string& executable, const std::vector<std::string>& args,
                       bool createNewConsole, std::string& outError);
} // namespace grav_platform_detail
#endif // GRAVITY_ENGINE_INCLUDE_PLATFORM_INTERNAL_PROCESSOPS_HPP_
