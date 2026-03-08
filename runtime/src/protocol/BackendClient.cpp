#include "protocol/BackendClient.hpp"
#include "protocol/BackendProtocol.hpp"
#include "platform/SocketPlatform.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <exception>
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
BackendClient::BackendClient()
    : _socket(grav_socket::invalidHandle()),
      _socketTimeoutMs(3000),
      _networkInitialized(false),
      _recvBuffer(),
      _authToken()
{
}

BackendClient::~BackendClient()
{
    disconnect();
}

bool BackendClient::connect(const std::string &host, std::uint16_t port)
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
    } catch (...) {
        disconnect();
        return false;
    }
}

void BackendClient::setSocketTimeoutMs(int timeoutMs)
{
    _socketTimeoutMs = clampTimeoutMs(timeoutMs);
    if (isConnected()) {
        grav_socket::setSocketTimeoutMs(_socket, _socketTimeoutMs);
    }
}

int BackendClient::socketTimeoutMs() const
{
    return _socketTimeoutMs;
}

void BackendClient::setAuthToken(std::string token)
{
    _authToken = std::move(token);
}

void BackendClient::disconnect()
{
    try {
        grav_socket::closeSocket(_socket);
        _socket = grav_socket::invalidHandle();
        _recvBuffer.clear();
        if (_networkInitialized) {
            grav_socket::shutdownSocketLayer();
            _networkInitialized = false;
        }
    } catch (...) {
        _socket = grav_socket::invalidHandle();
        _recvBuffer.clear();
        _networkInitialized = false;
    }
}

bool BackendClient::isConnected() const
{
    return grav_socket::isValid(_socket);
}

std::string BackendClient::trim(const std::string &value)
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

bool BackendClient::readLine(std::string &outLine)
{
    try {
        if (!grav_socket::isValid(_socket)) {
            return false;
        }

        while (true) {
            const std::size_t pos = _recvBuffer.find('\n');
            if (pos != std::string::npos) {
                outLine = _recvBuffer.substr(0, pos);
                _recvBuffer.erase(0, pos + 1);
                if (!outLine.empty() && outLine.back() == '\r') {
                    outLine.pop_back();
                }
                return true;
            }

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
        }
    } catch (...) {
        return false;
    }
}

BackendClientResponse BackendClient::sendJson(const std::string &jsonLine)
{
    BackendClientResponse response;
    try {
        if (!isConnected()) {
            response.error = "not connected";
            return response;
        }

        std::string line = trim(jsonLine);
        if (line.empty()) {
            response.error = "empty request";
            return response;
        }
        line.push_back('\n');

        if (!sendAll(_socket, asBytes(line))) {
            response.error = "send failed";
            disconnect();
            return response;
        }

        if (!readLine(response.raw)) {
            response.error = "recv failed";
            disconnect();
            return response;
        }

        grav_protocol::BackendResponseEnvelope envelope{};
        if (!grav_protocol::BackendJsonCodec::parseResponseEnvelope(response.raw, envelope, response.error)) {
            response.error = "invalid response";
            return response;
        }
        response.ok = envelope.ok;
        response.error = envelope.error;
        return response;
    } catch (const std::exception &ex) {
        response.ok = false;
        response.error = ex.what();
        disconnect();
        return response;
    } catch (...) {
        response.ok = false;
        response.error = "unknown sendJson error";
        disconnect();
        return response;
    }
}

BackendClientResponse BackendClient::sendCommand(const std::string &cmd, const std::string &fieldsJson)
{
    grav_protocol::BackendCommandRequest request{};
    request.cmd = cmd;
    request.token = _authToken;
    return sendJson(grav_protocol::BackendJsonCodec::makeCommandRequest(request, fieldsJson));
}

BackendClientResponse BackendClient::getStatus(BackendClientStatus &outStatus)
{
    try {
        BackendClientResponse response = sendCommand(std::string(grav_protocol::Status));
        if (!response.ok) {
            return response;
        }

        grav_protocol::BackendStatusPayload parsed{};
        std::string parseError;
        if (!grav_protocol::BackendJsonCodec::parseStatusResponse(response.raw, parsed, parseError)) {
            response.ok = false;
            response.error = parseError;
            return response;
        }

        outStatus.steps = parsed.steps;
        outStatus.dt = parsed.dt;
        outStatus.paused = parsed.paused;
        outStatus.faulted = parsed.faulted;
        outStatus.faultStep = parsed.faultStep;
        outStatus.faultReason = parsed.faultReason;
        outStatus.sphEnabled = parsed.sphEnabled;
        outStatus.backendFps = parsed.backendFps;
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
        BackendClientResponse response;
        response.error = ex.what();
        return response;
    } catch (...) {
        BackendClientResponse response;
        response.error = "unknown getStatus error";
        return response;
    }
}

BackendClientResponse BackendClient::getSnapshot(std::vector<RenderParticle> &outSnapshot, std::uint32_t maxPoints)
{
    try {
        maxPoints = grav_protocol::clampSnapshotPoints(maxPoints);
        BackendClientResponse response = sendCommand(
            std::string(grav_protocol::GetSnapshot),
            "\"max_points\":" + std::to_string(maxPoints));
        if (!response.ok) {
            return response;
        }

        grav_protocol::BackendSnapshotPayload parsed{};
        std::string parseError;
        if (!grav_protocol::BackendJsonCodec::parseSnapshotResponse(response.raw, parsed, parseError)) {
            response.ok = false;
            response.error = parseError;
            return response;
        }
        outSnapshot = std::move(parsed.particles);
        return response;
    } catch (const std::exception &ex) {
        BackendClientResponse response;
        response.error = ex.what();
        return response;
    } catch (...) {
        BackendClientResponse response;
        response.error = "unknown getSnapshot error";
        return response;
    }
}
