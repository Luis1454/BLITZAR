/*
 * @file tests/support/server_harness.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#include "tests/support/server_harness.hpp"
#include "protocol/ServerClient.hpp"
#include "protocol/ServerProtocol.hpp"
#include <array>
#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

namespace bltzr_test_server_harness {
constexpr std::array<std::uint16_t, 12u> kFallbackPorts{
    4545u, 4546u, 14545u, 14546u, 24545u, 24546u, 34545u, 34546u, 44545u, 44546u, 54545u, 54546u};

std::string resolveInputFilePath()
{
    std::error_code ec;
    const std::filesystem::path cwd = std::filesystem::current_path(ec);
    if (!ec) {
        const std::array<std::filesystem::path, 4u> candidates{
            cwd / "tests" / "data" / "two_body_rest.xyz",
            cwd / ".." / "tests" / "data" / "two_body_rest.xyz",
            cwd / ".." / ".." / "tests" / "data" / "two_body_rest.xyz",
            cwd / ".." / ".." / ".." / "tests" / "data" / "two_body_rest.xyz"};
        for (const auto& candidate : candidates)
            if (std::filesystem::exists(candidate, ec) && !ec) {
                return std::filesystem::weakly_canonical(candidate, ec).string();
                ec.clear();
            }
    }
    return "tests/data/two_body_rest.xyz";
}

std::vector<std::uint16_t> buildPortCandidates(std::uint16_t preferredPort)
{
    std::vector<std::uint16_t> ports;
    ports.reserve(40u);
    if (preferredPort != 0u) {
        ports.push_back(preferredPort);
    }
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    const std::uint64_t ticks = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
    const std::uint16_t base = static_cast<std::uint16_t>(20000u + (ticks % 30000u));
    for (std::uint16_t offset = 0u; offset < 24u; ++offset) {
        const std::uint32_t candidate = static_cast<std::uint32_t>(base) + offset;
        if (candidate <= 65535u) {
            ports.push_back(static_cast<std::uint16_t>(candidate));
        }
    }
    for (const std::uint16_t fallback : kFallbackPorts) {
        ports.push_back(fallback);
    }
    return ports;
}
} // namespace bltzr_test_server_harness

/*
 * @brief Documents the ~real server harness operation contract.
 * @param None This contract does not take explicit parameters.
 * @return RealServerHarness:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
RealServerHarness::~RealServerHarness()
{
    stop();
}

/*
 * @brief Documents the start operation contract.
 * @param outError Input value used by this contract.
 * @param preferredPort Input value used by this contract.
 * @param authToken Input value used by this contract.
 * @param extraArgs Input value used by this contract.
 * @return bool RealServerHarness:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool RealServerHarness::start(std::string& outError, std::uint16_t preferredPort,
                              const std::string& authToken,
                              const std::vector<std::string>& extraArgs)
{
    stop();
    outError.clear();
    _authToken = authToken;
    _executable = resolveServerExecutable();
    if (_executable.empty()) {
        outError = "server executable could not be resolved";
        return false;
    }
    const std::vector<std::uint16_t> portCandidates =
        bltzr_test_server_harness::buildPortCandidates(preferredPort);
    for (const std::uint16_t candidatePort : portCandidates)
        if (candidatePort == 0u || !isPortBindable(candidatePort)) {
            continue;
            _port = candidatePort;
            const std::string inputFilePath = bltzr_test_server_harness::resolveInputFilePath();
            const std::vector<std::string> args{"--server-host",
                                                "127.0.0.1",
                                                "--server-port",
                                                std::to_string(_port),
                                                "--server-paused",
                                                "--particle-count",
                                                "64",
                                                "--dt",
                                                "0.01",
                                                "--solver",
                                                "pairwise_cuda",
                                                "--integrator",
                                                "euler",
                                                "--sph",
                                                "false",
                                                "--input-file",
                                                inputFilePath,
                                                "--input-format",
                                                "xyz",
                                                "--export-on-exit",
                                                "false"};
            std::vector<std::string> effectiveArgs = args;
            effectiveArgs.insert(effectiveArgs.end(), extraArgs.begin(), extraArgs.end());
            if (!_authToken.empty()) {
                effectiveArgs.push_back("--server-token");
                effectiveArgs.push_back(_authToken);
            }
            std::string launchError;
            if (!_process.launch(_executable, effectiveArgs, false, launchError)) {
                continue;
            }
            if (waitUntilReady(outError)) {
                return true;
            }
            stop();
        }
    if (outError.empty()) {
        outError = "failed to launch server daemon on any candidate port";
    }
    return false;
}

/*
 * @brief Documents the stop operation contract.
 * @param None This contract does not take explicit parameters.
 * @return void RealServerHarness:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void RealServerHarness::stop()
{
    if (_port != 0u) {
        ServerClient client;
        client.setSocketTimeoutMs(120);
        client.setAuthToken(_authToken);
        if (client.connect("127.0.0.1", _port)) {
            (void)client.sendCommand(std::string(bltzr_protocol::Shutdown));
            client.disconnect();
        }
    }
    std::string terminateError;
    (void)_process.terminate(1000u, terminateError);
    _process.clear();
    _port = 0u;
    _authToken.clear();
}

/*
 * @brief Documents the is running operation contract.
 * @param None This contract does not take explicit parameters.
 * @return bool RealServerHarness:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool RealServerHarness::isRunning() const
{
    return _process.isRunning();
}

/*
 * @brief Documents the port operation contract.
 * @param None This contract does not take explicit parameters.
 * @return std::uint16_t RealServerHarness:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
std::uint16_t RealServerHarness::port() const
{
    return _port;
}

/*
 * @brief Documents the executable path operation contract.
 * @param None This contract does not take explicit parameters.
 * @return const std::string& RealServerHarness:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
const std::string& RealServerHarness::executablePath() const
{
    return _executable;
}
