// File: tests/support/server_harness.hpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#ifndef GRAVITY_TESTS_SUPPORT_SERVER_HARNESS_HPP_
#define GRAVITY_TESTS_SUPPORT_SERVER_HARNESS_HPP_
#include "platform/PlatformProcess.hpp"
#include <cstdint>
#include <string>
#include <vector>

/// Description: Defines the RealServerHarness data or behavior contract.
class RealServerHarness {
public:
    /// Description: Describes the real server harness operation contract.
    RealServerHarness() = default;
    /// Description: Releases resources owned by RealServerHarness.
    ~RealServerHarness();
    /// Description: Describes the real server harness operation contract.
    RealServerHarness(const RealServerHarness&) = delete;
    /// Description: Describes the operator= operation contract.
    RealServerHarness& operator=(const RealServerHarness&) = delete;
    /// Description: Describes the real server harness operation contract.
    RealServerHarness(RealServerHarness&&) = delete;
    /// Description: Describes the operator= operation contract.
    RealServerHarness& operator=(RealServerHarness&&) = delete;
    /// Description: Describes the start operation contract.
    bool start(std::string& outError, std::uint16_t preferredPort = 0u,
               const std::string& authToken = {}, const std::vector<std::string>& extraArgs = {});
    /// Description: Describes the stop operation contract.
    void stop();
    /// Description: Describes the is running operation contract.
    bool isRunning() const;
    /// Description: Describes the port operation contract.
    std::uint16_t port() const;
    const std::string& executablePath() const;

private:
    /// Description: Describes the resolve server executable operation contract.
    static std::string resolveServerExecutable();
    /// Description: Describes the is port bindable operation contract.
    static bool isPortBindable(std::uint16_t port);
    /// Description: Describes the wait until ready operation contract.
    bool waitUntilReady(std::string& outError) const;
    grav_platform::ProcessHandle _process;
    std::uint16_t _port = 0u;
    std::string _executable;
    std::string _authToken;
};
#endif // GRAVITY_TESTS_SUPPORT_SERVER_HARNESS_HPP_
