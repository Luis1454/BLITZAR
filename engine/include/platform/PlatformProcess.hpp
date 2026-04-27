// File: engine/include/platform/PlatformProcess.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_PLATFORM_PLATFORMPROCESS_HPP_
#define GRAVITY_ENGINE_INCLUDE_PLATFORM_PLATFORMPROCESS_HPP_
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
namespace grav_platform {
/// Description: Executes the quoteProcessArg operation.
std::string quoteProcessArg(const std::string& arg);
std::string buildProcessCommandLine(const std::string& executable,
                                    const std::vector<std::string>& args);
/// Description: Defines the ProcessHandle data or behavior contract.
class ProcessHandle {
public:
    /// Description: Executes the ProcessHandle operation.
    ProcessHandle();
    /// Description: Releases resources owned by ProcessHandle.
    ~ProcessHandle();
    ProcessHandle(const ProcessHandle&) = delete;
    ProcessHandle& operator=(const ProcessHandle&) = delete;
    ProcessHandle(ProcessHandle&& other) noexcept;
    ProcessHandle& operator=(ProcessHandle&& other) noexcept;
    bool launch(const std::string& executable, const std::vector<std::string>& args,
                bool createNewConsole, std::string& outError);
    /// Description: Executes the terminate operation.
    bool terminate(std::uint32_t waitMs, std::string& outError);
    /// Description: Executes the isRunning operation.
    bool isRunning() const;
    /// Description: Executes the clear operation.
    void clear();
    /// Description: Executes the pidString operation.
    std::string pidString() const;
    /// Description: Executes the commandLine operation.
    const std::string& commandLine() const;

private:
    /// Description: Defines the Impl data or behavior contract.
    struct Impl;
    std::unique_ptr<Impl> _impl;
};
bool launchDetachedProcess(const std::string& executable, const std::vector<std::string>& args,
                           std::string& outError);
int runProcessBlocking(const std::string& executable, const std::vector<std::string>& args,
                       bool createNewConsole, std::string& outError);
} // namespace grav_platform
#endif // GRAVITY_ENGINE_INCLUDE_PLATFORM_PLATFORMPROCESS_HPP_
