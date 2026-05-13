/*
 * @file engine/include/platform/internal/ProcessOps.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Platform abstraction interfaces for portable runtime services.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_PLATFORM_INTERNAL_PROCESSOPS_HPP_
#define BLITZAR_ENGINE_INCLUDE_PLATFORM_INTERNAL_PROCESSOPS_HPP_
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace bltzr_platform_detail {
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
} // namespace bltzr_platform_detail
#endif // BLITZAR_ENGINE_INCLUDE_PLATFORM_INTERNAL_PROCESSOPS_HPP_
