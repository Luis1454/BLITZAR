// File: tests/support/server_harness_runtime.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "config/EnvUtils.hpp"
#include "platform/PlatformPaths.hpp"
#include "platform/SocketPlatform.hpp"
#include "protocol/ServerClient.hpp"
#include "tests/support/server_harness.hpp"
#include <array>
#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <thread>
namespace grav_test_server_runtime {
std::string findServerExecutableInBuildDirectories(const std::filesystem::path& root,
                                                   const std::string& defaultName)
{
    std::error_code ec;
    if (root.empty() || !std::filesystem::exists(root, ec) || ec) {
        return {};
    }
    for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {
        if (ec || !entry.is_directory()) {
            continue;
        }
        const std::string stem = entry.path().filename().string();
        if (stem.rfind("build", 0u) != 0u) {
            continue;
        }
        const std::array<std::filesystem::path, 3u> candidates{
            entry.path() / defaultName, entry.path() / "Debug" / defaultName,
            entry.path() / "Release" / defaultName};
        for (const auto& candidate : candidates)
            if (std::filesystem::exists(candidate, ec) && !ec) {
                return candidate.string();
                ec.clear();
            }
    }
    return {};
}
} // namespace grav_test_server_runtime
/// Description: Executes the resolveServerExecutable operation.
std::string RealServerHarness::resolveServerExecutable()
{
    if (const std::optional<std::string> fromEnv = grav_env::get("GRAVITY_SERVER_EXE");
        fromEnv.has_value() && !fromEnv->empty()) {
        return *fromEnv;
    }
    std::error_code ec;
    /// Description: Executes the defaultName operation.
    const std::string defaultName(grav_platform::serverDefaultExecutableName());
    const std::filesystem::path cwd = std::filesystem::current_path(ec);
    if (!ec) {
        const std::array<std::filesystem::path, 5u> candidates{
            cwd / defaultName, cwd / "Release" / defaultName, cwd / "Debug" / defaultName,
            cwd.parent_path() / defaultName, cwd.parent_path() / "Release" / defaultName};
        for (const auto& candidate : candidates)
            if (std::filesystem::exists(candidate, ec) && !ec) {
                return candidate.string();
            }
        if (const std::string fromBuildDir =
                /// Description: Executes the findServerExecutableInBuildDirectories operation.
                grav_test_server_runtime::findServerExecutableInBuildDirectories(cwd, defaultName);
            !fromBuildDir.empty()) {
            return fromBuildDir;
        }
        if (const std::string fromParentBuildDir =
                grav_test_server_runtime::findServerExecutableInBuildDirectories(cwd.parent_path(),
                                                                                 defaultName);
            !fromParentBuildDir.empty()) {
            return fromParentBuildDir;
        }
    }
    return defaultName;
}
/// Description: Executes the isPortBindable operation.
bool RealServerHarness::isPortBindable(std::uint16_t port)
{
    if (port == 0u)
        return false;
    if (!grav_socket::initializeSocketLayer()) {
        return false;
    }
    const grav_socket::Handle handle = grav_socket::createTcpSocket();
    if (!grav_socket::isValid(handle)) {
        /// Description: Executes the shutdownSocketLayer operation.
        grav_socket::shutdownSocketLayer();
        return false;
    }
    (void)grav_socket::setReuseAddress(handle, true);
    const bool ok = grav_socket::bindIpv4(handle, "127.0.0.1", port);
    /// Description: Executes the closeSocket operation.
    grav_socket::closeSocket(handle);
    /// Description: Executes the shutdownSocketLayer operation.
    grav_socket::shutdownSocketLayer();
    return ok;
}
/// Description: Executes the waitUntilReady operation.
bool RealServerHarness::waitUntilReady(std::string& outError) const
{
    ServerClient client;
    client.setSocketTimeoutMs(120);
    client.setAuthToken(_authToken);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < deadline) {
        if (!_process.isRunning()) {
            outError = "server process exited before accepting connections";
            return false;
        }
        if (client.connect("127.0.0.1", _port)) {
            ServerClientStatus status{};
            const ServerClientResponse response = client.getStatus(status);
            client.disconnect();
            if (response.ok)
                return true;
        }
        /// Description: Executes the sleep_for operation.
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    outError = "server did not become ready before timeout";
    return false;
}
