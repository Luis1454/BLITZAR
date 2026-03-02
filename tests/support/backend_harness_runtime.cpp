#include "tests/support/backend_harness.hpp"

#include "platform/PlatformPaths.hpp"
#include "platform/SocketPlatform.hpp"
#include "protocol/BackendClient.hpp"

#include <array>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <thread>

namespace grav_test_backend_harness_runtime {

std::optional<std::string> readEnvVar(const char *name)
{
#if defined(_WIN32)
    char *rawValue = nullptr;
    std::size_t rawSize = 0;
    if (_dupenv_s(&rawValue, &rawSize, name) != 0 || rawValue == nullptr) {
        return std::nullopt;
    }
    std::string value(rawValue);
    std::free(rawValue);
    if (value.empty()) {
        return std::nullopt;
    }
    return value;
#else
    const char *rawValue = std::getenv(name);
    if (rawValue == nullptr || *rawValue == '\0') {
        return std::nullopt;
    }
    return std::string(rawValue);
#endif
}

} // namespace grav_test_backend_harness_runtime

std::string RealBackendHarness::resolveBackendExecutable()
{
    if (const std::optional<std::string> fromEnv = grav_test_backend_harness_runtime::readEnvVar("GRAVITY_BACKEND_EXE"); fromEnv.has_value()) {
        return *fromEnv;
    }

    std::error_code ec;
    const std::string defaultName(grav_platform::backendDefaultExecutableName());
    const std::filesystem::path cwd = std::filesystem::current_path(ec);
    if (!ec) {
        const std::array<std::filesystem::path, 5u> candidates{
            cwd / defaultName,
            cwd / "Release" / defaultName,
            cwd / "Debug" / defaultName,
            cwd.parent_path() / defaultName,
            cwd.parent_path() / "Release" / defaultName
        };
        for (const auto &candidate : candidates) {
            if (std::filesystem::exists(candidate, ec) && !ec) {
                return candidate.string();
            }
        }
    }

    return defaultName;
}

bool RealBackendHarness::isPortBindable(std::uint16_t port)
{
    if (port == 0u) {
        return false;
    }
    if (!grav_socket::initializeSocketLayer()) {
        return false;
    }
    const grav_socket::Handle handle = grav_socket::createTcpSocket();
    if (!grav_socket::isValid(handle)) {
        grav_socket::shutdownSocketLayer();
        return false;
    }

    (void)grav_socket::setReuseAddress(handle, true);
    const bool ok = grav_socket::bindIpv4(handle, "127.0.0.1", port);
    grav_socket::closeSocket(handle);
    grav_socket::shutdownSocketLayer();
    return ok;
}

bool RealBackendHarness::waitUntilReady(std::string &outError) const
{
    BackendClient client;
    client.setSocketTimeoutMs(120);
    client.setAuthToken(_authToken);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < deadline) {
        if (!_process.isRunning()) {
            outError = "backend process exited before accepting connections";
            return false;
        }
        if (client.connect("127.0.0.1", _port)) {
            BackendClientStatus status{};
            const BackendClientResponse response = client.getStatus(status);
            client.disconnect();
            if (response.ok) {
                return true;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    outError = "backend did not become ready before timeout";
    return false;
}
