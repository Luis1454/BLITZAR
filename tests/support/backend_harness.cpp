#include "tests/support/backend_harness.hpp"

#include "protocol/BackendClient.hpp"
#include "protocol/BackendProtocol.hpp"

#include <array>
#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

namespace {

constexpr std::array<std::uint16_t, 12u> kFallbackPorts{
    4545u, 4546u, 14545u, 14546u, 24545u, 24546u, 34545u, 34546u, 44545u, 44546u, 54545u, 54546u
};

std::string resolveInputFilePath()
{
    std::error_code ec;
    const std::filesystem::path cwd = std::filesystem::current_path(ec);
    if (!ec) {
        const std::array<std::filesystem::path, 4u> candidates{
            cwd / "tests" / "data" / "two_body_rest.xyz",
            cwd / ".." / "tests" / "data" / "two_body_rest.xyz",
            cwd / ".." / ".." / "tests" / "data" / "two_body_rest.xyz",
            cwd / ".." / ".." / ".." / "tests" / "data" / "two_body_rest.xyz"
        };
        for (const auto &candidate : candidates) {
            if (std::filesystem::exists(candidate, ec) && !ec) {
                return std::filesystem::weakly_canonical(candidate, ec).string();
            }
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

} // namespace

RealBackendHarness::~RealBackendHarness()
{
    stop();
}

bool RealBackendHarness::start(std::string &outError, std::uint16_t preferredPort, const std::string &authToken)
{
    stop();
    outError.clear();
    _authToken = authToken;
    _executable = resolveBackendExecutable();
    if (_executable.empty()) {
        outError = "backend executable could not be resolved";
        return false;
    }

    const std::vector<std::uint16_t> portCandidates = buildPortCandidates(preferredPort);
    for (const std::uint16_t candidatePort : portCandidates) {
        if (candidatePort == 0u || !isPortBindable(candidatePort)) {
            continue;
        }

        _port = candidatePort;
        const std::string inputFilePath = resolveInputFilePath();
        const std::vector<std::string> args{
            "--backend-host",
            "127.0.0.1",
            "--backend-port",
            std::to_string(_port),
            "--backend-paused",
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
            "false"
        };
        std::vector<std::string> effectiveArgs = args;
        if (!_authToken.empty()) {
            effectiveArgs.push_back("--backend-token");
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
        outError = "failed to launch backend daemon on any candidate port";
    }
    return false;
}

void RealBackendHarness::stop()
{
    if (_port != 0u) {
        BackendClient client;
        client.setSocketTimeoutMs(120);
        client.setAuthToken(_authToken);
        if (client.connect("127.0.0.1", _port)) {
            (void)client.sendCommand(std::string(grav_protocol::Shutdown));
            client.disconnect();
        }
    }

    std::string terminateError;
    (void)_process.terminate(1000u, terminateError);
    _process.clear();
    _port = 0u;
    _authToken.clear();
}

bool RealBackendHarness::isRunning() const
{
    return _process.isRunning();
}

std::uint16_t RealBackendHarness::port() const
{
    return _port;
}

const std::string &RealBackendHarness::executablePath() const
{
    return _executable;
}

