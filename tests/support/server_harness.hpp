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
    RealServerHarness() = default;
    /// Description: Releases resources owned by RealServerHarness.
    ~RealServerHarness();
    RealServerHarness(const RealServerHarness&) = delete;
    RealServerHarness& operator=(const RealServerHarness&) = delete;
    RealServerHarness(RealServerHarness&&) = delete;
    RealServerHarness& operator=(RealServerHarness&&) = delete;
    bool start(std::string& outError, std::uint16_t preferredPort = 0u,
               const std::string& authToken = {}, const std::vector<std::string>& extraArgs = {});
    /// Description: Executes the stop operation.
    void stop();
    /// Description: Executes the isRunning operation.
    bool isRunning() const;
    /// Description: Executes the port operation.
    std::uint16_t port() const;
    /// Description: Executes the executablePath operation.
    const std::string& executablePath() const;

private:
    /// Description: Executes the resolveServerExecutable operation.
    static std::string resolveServerExecutable();
    /// Description: Executes the isPortBindable operation.
    static bool isPortBindable(std::uint16_t port);
    /// Description: Executes the waitUntilReady operation.
    bool waitUntilReady(std::string& outError) const;
    grav_platform::ProcessHandle _process;
    std::uint16_t _port = 0u;
    std::string _executable;
    std::string _authToken;
};
#endif // GRAVITY_TESTS_SUPPORT_SERVER_HARNESS_HPP_
