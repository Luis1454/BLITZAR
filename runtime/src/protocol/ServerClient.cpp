#include "protocol/ServerClient.hpp"
#include "protocol/ServerProtocol.hpp"
#include "platform/SocketPlatform.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

static grav_socket::ConstBytes asBytes(std::string_view text)
{
    return grav_socket::ConstBytes{
        reinterpret_cast<const std::byte *>(text.data()),
        text.size()};
}

static bool sendAll(grav_socket::Handle socketHandle, grav_socket::ConstBytes bytes)
{
    std::size_t offset = 0;
    while (offset < bytes.size) {
        const int sent = grav_socket::sendBytes(
            socketHandle,
            bytes.subview(offset));
        if (sent <= 0) {
            return false;
        }
        offset += static_cast<std::size_t>(sent);
    }
    return true;
}

static int clampTimeoutMs(int timeoutMs)
{
    return grav_socket::clampTimeoutMs(timeoutMs);
}

static std::string serverClientError(std::string_view operation, std::string_view detail)
{
    return std::string("[server-client] ") + std::string(operation) + ": " + std::string(detail);
}

ServerClient::ServerClient()
    : _socket(grav_socket::invalidHandle()),
      _socketTimeoutMs(3000),
      _networkInitialized(false),
      _recvBuffer(),
      _authToken()
{
}

ServerClient::~ServerClient()
{
    disconnect();
}

bool ServerClient::connect(const std::string &host, std::uint16_t port)
{
    try {
        disconnect();

        if (!grav_socket::initializeSocketLayer()) {
            return false;
        }
        _networkInitialized = true;

        const grav_socket::Handle socketHandle = grav_socket::createTcpSocket();
        if (!grav_socket::isValid(socketHandle)) {
            disconnect();
            return false;
        }

        if (!grav_socket::connectIpv4(socketHandle, host, port, _socketTimeoutMs)) {
            grav_socket::closeSocket(socketHandle);
            disconnect();
            return false;
        }

        grav_socket::setSocketTimeoutMs(socketHandle, _socketTimeoutMs);

        _socket = socketHandle;
        _recvBuffer.clear();
        return true;
    } catch (const std::exception &ex) {
        std::cerr << serverClientError("connect", ex.what()) << "\n";
        disconnect();
        return false;
    } catch (...) {
        std::cerr << serverClientError("connect", "non-standard exception") << "\n";
        disconnect();
        return false;
    }
}

void ServerClient::setSocketTimeoutMs(int timeoutMs)
{
    _socketTimeoutMs = clampTimeoutMs(timeoutMs);
    if (isConnected()) {
        grav_socket::setSocketTimeoutMs(_socket, _socketTimeoutMs);
    }
}

int ServerClient::socketTimeoutMs() const
{
    return _socketTimeoutMs;
}

void ServerClient::setAuthToken(std::string token)
{
    _authToken = std::move(token);
}

void ServerClient::disconnect()
{
    try {
        grav_socket::closeSocket(_socket);
        _socket = grav_socket::invalidHandle();
        _recvBuffer.clear();
        if (_networkInitialized) {
            grav_socket::shutdownSocketLayer();
            _networkInitialized = false;
        }
    } catch (const std::exception &ex) {
        std::cerr << serverClientError("disconnect", ex.what()) << "\n";
        _socket = grav_socket::invalidHandle();
        _recvBuffer.clear();
        _networkInitialized = false;
    } catch (...) {
        std::cerr << serverClientError("disconnect", "non-standard exception") << "\n";
        _socket = grav_socket::invalidHandle();
        _recvBuffer.clear();
        _networkInitialized = false;
    }
}

bool ServerClient::isConnected() const
{
    return grav_socket::isValid(_socket);
}

std::string ServerClient::trim(const std::string &value)
{
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) {
        return std::isspace(c) != 0;
    }).base();
    if (begin >= end) {
        return {};
    }
    return std::string(begin, end);
}

bool ServerClient::readLine(std::string &outLine)
{
    try {
        if (!grav_socket::isValid(_socket)) {
            return false;
        }

        std::size_t pos = _recvBuffer.find('\n');
        while (pos == std::string::npos) {
            std::array<char, 2048> chunk{};
            const int received = grav_socket::recvBytes(
                _socket,
                grav_socket::MutableBytes{
                    reinterpret_cast<std::byte *>(chunk.data()),
                    chunk.size()});
            if (received <= 0) {
                return false;
            }
            _recvBuffer.append(chunk.data(), static_cast<std::size_t>(received));
            if (_recvBuffer.size() > (512u * 1024u)) {
                return false;
            }
            pos = _recvBuffer.find('\n');
        }

        outLine = _recvBuffer.substr(0, pos);
        _recvBuffer.erase(0, pos + 1);
        if (!outLine.empty() && outLine.back() == '\r') {
            outLine.pop_back();
        }
        return true;
    } catch (const std::exception &ex) {
        std::cerr << serverClientError("readLine", ex.what()) << "\n";
        return false;
    } catch (...) {
        std::cerr << serverClientError("readLine", "non-standard exception") << "\n";
        return false;
    }
}

ServerClientResponse ServerClient::sendJson(const std::string &jsonLine)
{
    ServerClientResponse response;
    try {
        if (!isConnected()) {
            response.error = serverClientError("sendJson", "not connected");
            return response;
        }

        std::string line = trim(jsonLine);
        if (line.empty()) {
            response.error = serverClientError("sendJson", "empty request");
            return response;
        }
        line.push_back('\n');

        if (!sendAll(_socket, asBytes(line))) {
            response.error = serverClientError("sendJson", "send failed");
            disconnect();
            return response;
        }

        if (!readLine(response.raw)) {
            response.error = serverClientError("sendJson", "recv failed");
            disconnect();
            return response;
        }

        grav_protocol::ServerResponseEnvelope envelope{};
        if (!grav_protocol::ServerJsonCodec::parseResponseEnvelope(response.raw, envelope, response.error)) {
            response.error = serverClientError("sendJson", "invalid response");
            return response;
        }
        response.ok = envelope.ok;
        response.error = envelope.error;
        return response;
    } catch (const std::exception &ex) {
        response.ok = false;
        response.error = serverClientError("sendJson", ex.what());
        disconnect();
        return response;
    } catch (...) {
        response.ok = false;
        response.error = serverClientError("sendJson", "non-standard exception");
        disconnect();
        return response;
    }
}

ServerClientResponse ServerClient::sendCommand(const std::string &cmd, const std::string &fieldsJson)
{
    grav_protocol::ServerCommandRequest request{};
    request.cmd = cmd;
    request.token = _authToken;
    return sendJson(grav_protocol::ServerJsonCodec::makeCommandRequest(request, fieldsJson));
}

ServerClientResponse ServerClient::getStatus(ServerClientStatus &outStatus)
{
    try {
        ServerClientResponse response = sendCommand(std::string(grav_protocol::Status));
        if (!response.ok) {
            return response;
        }

        grav_protocol::ServerStatusPayload parsed{};
        std::string parseError;
        if (!grav_protocol::ServerJsonCodec::parseStatusResponse(response.raw, parsed, parseError)) {
            response.ok = false;
            response.error = serverClientError("getStatus", parseError);
            return response;
        }

        outStatus.steps = parsed.steps;
        outStatus.dt = parsed.dt;
        outStatus.totalTime = parsed.totalTime;
        outStatus.paused = parsed.paused;
        outStatus.faulted = parsed.faulted;
        outStatus.faultStep = parsed.faultStep;
        outStatus.faultReason = parsed.faultReason;
        outStatus.sphEnabled = parsed.sphEnabled;
        outStatus.serverFps = parsed.serverFps;
        outStatus.performanceProfile = parsed.performanceProfile;
        outStatus.substepTargetDt = parsed.substepTargetDt;
        outStatus.substepDt = parsed.substepDt;
        outStatus.substeps = parsed.substeps;
        outStatus.maxSubsteps = parsed.maxSubsteps;
        outStatus.snapshotPublishPeriodMs = parsed.snapshotPublishPeriodMs;
        outStatus.particleCount = parsed.particleCount;
        outStatus.solver = parsed.solver;
        outStatus.integrator = parsed.integrator;
        outStatus.kineticEnergy = parsed.kineticEnergy;
        outStatus.potentialEnergy = parsed.potentialEnergy;
        outStatus.thermalEnergy = parsed.thermalEnergy;
        outStatus.radiatedEnergy = parsed.radiatedEnergy;
        outStatus.totalEnergy = parsed.totalEnergy;
        outStatus.energyDriftPct = parsed.energyDriftPct;
        outStatus.energyEstimated = parsed.energyEstimated;
        return response;
    } catch (const std::exception &ex) {
        ServerClientResponse response;
        response.error = serverClientError("getStatus", ex.what());
        return response;
    } catch (...) {
        ServerClientResponse response;
        response.error = serverClientError("getStatus", "non-standard exception");
        return response;
    }
}

ServerClientResponse ServerClient::getSnapshot(std::vector<RenderParticle> &outSnapshot, std::uint32_t maxPoints)
{
    try {
        maxPoints = grav_protocol::clampSnapshotPoints(maxPoints);
        ServerClientResponse response = sendCommand(
            std::string(grav_protocol::GetSnapshot),
            "\"max_points\":" + std::to_string(maxPoints));
        if (!response.ok) {
            return response;
        }

        grav_protocol::ServerSnapshotPayload parsed{};
        std::string parseError;
        if (!grav_protocol::ServerJsonCodec::parseSnapshotResponse(response.raw, parsed, parseError)) {
            response.ok = false;
            response.error = serverClientError("getSnapshot", parseError);
            return response;
        }
        outSnapshot = std::move(parsed.particles);
        return response;
    } catch (const std::exception &ex) {
        ServerClientResponse response;
        response.error = serverClientError("getSnapshot", ex.what());
        return response;
    } catch (...) {
        ServerClientResponse response;
        response.error = serverClientError("getSnapshot", "non-standard exception");
        return response;
    }
}
