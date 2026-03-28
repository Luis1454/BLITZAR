#ifndef GRAVITY_TESTS_SUPPORT_SERVER_HARNESS_HPP_
#define GRAVITY_TESTS_SUPPORT_SERVER_HARNESS_HPP_
#include "platform/PlatformProcess.hpp"
#include <cstdint>
#include <string>
#include <vector>
class RealServerHarness {
public:
    RealServerHarness() = default;
    ~RealServerHarness();
    RealServerHarness(const RealServerHarness&) = delete;
    RealServerHarness& operator=(const RealServerHarness&) = delete;
    RealServerHarness(RealServerHarness&&) = delete;
    RealServerHarness& operator=(RealServerHarness&&) = delete;
    bool start(std::string& outError, std::uint16_t preferredPort = 0u,
               const std::string& authToken = {}, const std::vector<std::string>& extraArgs = {});
    void stop();
    bool isRunning() const;
    std::uint16_t port() const;
    const std::string& executablePath() const;

private:
    static std::string resolveServerExecutable();
    static bool isPortBindable(std::uint16_t port);
    bool waitUntilReady(std::string& outError) const;
    grav_platform::ProcessHandle _process;
    std::uint16_t _port = 0u;
    std::string _executable;
    std::string _authToken;
};
#endif // GRAVITY_TESTS_SUPPORT_SERVER_HARNESS_HPP_
