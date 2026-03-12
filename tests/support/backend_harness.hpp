#ifndef GRAVITY_TESTS_SUPPORT_BACKEND_HARNESS_HPP_
#define GRAVITY_TESTS_SUPPORT_BACKEND_HARNESS_HPP_

#include "platform/PlatformProcess.hpp"

#include <cstdint>
#include <string>

class RealBackendHarness {
    public:
        RealBackendHarness() = default;
        ~RealBackendHarness();

        RealBackendHarness(const RealBackendHarness &) = delete;
        RealBackendHarness &operator=(const RealBackendHarness &) = delete;

        RealBackendHarness(RealBackendHarness &&) = delete;
        RealBackendHarness &operator=(RealBackendHarness &&) = delete;

        bool start(
            std::string &outError,
            std::uint16_t preferredPort = 0u,
            const std::string &authToken = {}
        );
        void stop();

        bool isRunning() const;
        std::uint16_t port() const;
        const std::string &executablePath() const;

    private:
        static std::string resolveBackendExecutable();
        static bool isPortBindable(std::uint16_t port);
        bool waitUntilReady(std::string &outError) const;

        grav_platform::ProcessHandle _process;
        std::uint16_t _port = 0u;
        std::string _executable;
        std::string _authToken;
};


#endif // GRAVITY_TESTS_SUPPORT_BACKEND_HARNESS_HPP_
