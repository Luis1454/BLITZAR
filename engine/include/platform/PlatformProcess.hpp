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
/// Description: Describes the build process command line operation contract.
std::string buildProcessCommandLine(const std::string& executable,
                                    const std::vector<std::string>& args);

/// Description: Defines the ProcessHandle data or behavior contract.
class ProcessHandle {
public:
    /// Description: Describes the process handle operation contract.
    ProcessHandle();
    /// Description: Releases resources owned by ProcessHandle.
    ~ProcessHandle();
    /// Description: Describes the process handle operation contract.
    ProcessHandle(const ProcessHandle&) = delete;
    /// Description: Describes the operator= operation contract.
    ProcessHandle& operator=(const ProcessHandle&) = delete;
    /// Description: Describes the process handle operation contract.
    ProcessHandle(ProcessHandle&& other) noexcept;
    /// Description: Describes the operator= operation contract.
    ProcessHandle& operator=(ProcessHandle&& other) noexcept;
    /// Description: Describes the launch operation contract.
    bool launch(const std::string& executable, const std::vector<std::string>& args,
                bool createNewConsole, std::string& outError);
    /// Description: Describes the terminate operation contract.
    bool terminate(std::uint32_t waitMs, std::string& outError);
    /// Description: Describes the is running operation contract.
    bool isRunning() const;
    /// Description: Describes the clear operation contract.
    void clear();
    /// Description: Describes the pid string operation contract.
    std::string pidString() const;
    const std::string& commandLine() const;

private:
    /// Description: Defines the Impl data or behavior contract.
    struct Impl;
    std::unique_ptr<Impl> _impl;
};

/// Description: Describes the launch detached process operation contract.
bool launchDetachedProcess(const std::string& executable, const std::vector<std::string>& args,
                           std::string& outError);
/// Description: Describes the run process blocking operation contract.
int runProcessBlocking(const std::string& executable, const std::vector<std::string>& args,
                       bool createNewConsole, std::string& outError);
} // namespace grav_platform
#endif // GRAVITY_ENGINE_INCLUDE_PLATFORM_PLATFORMPROCESS_HPP_
