/*
 * @file runtime/src/protocol/ServerClient.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime implementation for protocol, command, client, and FFI boundaries.
 */

#include "protocol/ServerClient.hpp"
#include "platform/SocketPlatform.hpp"
#include "protocol/ServerProtocol.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

/*
 * @brief Documents the as bytes operation contract.
 * @param text Input value used by this contract.
 * @return bltzr_socket::ConstBytes value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static bltzr_socket::ConstBytes asBytes(std::string_view text)
{
    return bltzr_socket::ConstBytes{reinterpret_cast<const std::byte*>(text.data()), text.size()};
}

/*
 * @brief Documents the send all operation contract.
 * @param socketHandle Input value used by this contract.
 * @param bytes Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static bool sendAll(bltzr_socket::Handle socketHandle, bltzr_socket::ConstBytes bytes)
{
    std::size_t offset = 0;
    while (offset < bytes.size) {
        const int sent = bltzr_socket::sendBytes(socketHandle, bytes.subview(offset));
        if (sent <= 0)
            return false;
        offset += static_cast<std::size_t>(sent);
    }
    return true;
}

/*
 * @brief Documents the clamp timeout ms operation contract.
 * @param timeoutMs Input value used by this contract.
 * @return int value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static int clampTimeoutMs(int timeoutMs)
{
    return bltzr_socket::clampTimeoutMs(timeoutMs);
}

/*
 * @brief Documents the server client error operation contract.
 * @param operation Input value used by this contract.
 * @param detail Input value used by this contract.
 * @return std::string value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static std::string serverClientError(std::string_view operation, std::string_view detail)
{
    return std::string("[server-client] ") + std::string(operation) + ": " + std::string(detail);
}

/*
 * @brief Documents the server client operation contract.
 * @param None This contract does not take explicit parameters.
 * @return ServerClient:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
ServerClient::ServerClient()
    /*
     * @brief Documents the socket operation contract.
     * @param invalidHandle Input value used by this contract.
     * @param _socketTimeoutMs Input value used by this contract.
     * @param false Input value used by this contract.
     * @param _recvBuffer Input value used by this contract.
     * @param _authToken Input value used by this contract.
     * @return : value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    : _socket(bltzr_socket::invalidHandle()),
      _socketTimeoutMs(3000),
      _networkInitialized(false),
      _recvBuffer(),
      _authToken()
{
}

/*
 * @brief Documents the ~server client operation contract.
 * @param None This contract does not take explicit parameters.
 * @return ServerClient:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
ServerClient::~ServerClient()
{
    disconnect();
}

/*
 * @brief Documents the connect operation contract.
 * @param host Input value used by this contract.
 * @param port Input value used by this contract.
 * @return bool ServerClient:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool ServerClient::connect(const std::string& host, std::uint16_t port)
{
    try {
        disconnect();
        if (!bltzr_socket::initializeSocketLayer()) {
            return false;
        }
        _networkInitialized = true;
        const bltzr_socket::Handle socketHandle = bltzr_socket::createTcpSocket();
        if (!bltzr_socket::isValid(socketHandle)) {
            disconnect();
            return false;
        }
        if (!bltzr_socket::connectIpv4(socketHandle, host, port, _socketTimeoutMs)) {
            bltzr_socket::closeSocket(socketHandle);
            disconnect();
            return false;
        }
        bltzr_socket::setSocketTimeoutMs(socketHandle, _socketTimeoutMs);
        _socket = socketHandle;
        _recvBuffer.clear();
        return true;
    }
    catch (const std::exception& ex) {
        std::cerr << serverClientError("connect", ex.what()) << "\n";
        disconnect();
        return false;
    }
    catch (...) {
        std::cerr << serverClientError("connect", "non-standard exception") << "\n";
        disconnect();
        return false;
    }
}

/*
 * @brief Documents the set socket timeout ms operation contract.
 * @param timeoutMs Input value used by this contract.
 * @return void ServerClient:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void ServerClient::setSocketTimeoutMs(int timeoutMs)
{
    _socketTimeoutMs = clampTimeoutMs(timeoutMs);
    if (isConnected()) {
        bltzr_socket::setSocketTimeoutMs(_socket, _socketTimeoutMs);
    }
}

/*
 * @brief Documents the socket timeout ms operation contract.
 * @param None This contract does not take explicit parameters.
 * @return int ServerClient:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
int ServerClient::socketTimeoutMs() const
{
    return _socketTimeoutMs;
}

/*
 * @brief Documents the set auth token operation contract.
 * @param token Input value used by this contract.
 * @return void ServerClient:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void ServerClient::setAuthToken(std::string token)
{
    _authToken = std::move(token);
}

/*
 * @brief Documents the disconnect operation contract.
 * @param None This contract does not take explicit parameters.
 * @return void ServerClient:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void ServerClient::disconnect()
{
    try {
        bltzr_socket::closeSocket(_socket);
        _socket = bltzr_socket::invalidHandle();
        _recvBuffer.clear();
        if (_networkInitialized) {
            bltzr_socket::shutdownSocketLayer();
            _networkInitialized = false;
        }
    }
    catch (const std::exception& ex) {
        std::cerr << serverClientError("disconnect", ex.what()) << "\n";
        _socket = bltzr_socket::invalidHandle();
        _recvBuffer.clear();
        _networkInitialized = false;
    }
    catch (...) {
        std::cerr << serverClientError("disconnect", "non-standard exception") << "\n";
        _socket = bltzr_socket::invalidHandle();
        _recvBuffer.clear();
        _networkInitialized = false;
    }
}

/*
 * @brief Documents the is connected operation contract.
 * @param None This contract does not take explicit parameters.
 * @return bool ServerClient:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool ServerClient::isConnected() const
{
    return bltzr_socket::isValid(_socket);
}

/*
 * @brief Documents the trim operation contract.
 * @param value Input value used by this contract.
 * @return std::string ServerClient:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
std::string ServerClient::trim(const std::string& value)
{
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) {
                         return std::isspace(c) != 0;
                     }).base();
    if (begin >= end)
        return {};
    return std::string(begin, end);
}

/*
 * @brief Documents the read line operation contract.
 * @param outLine Input value used by this contract.
 * @return bool ServerClient:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool ServerClient::readLine(std::string& outLine)
{
    try {
        if (!bltzr_socket::isValid(_socket)) {
            return false;
        }
        std::size_t pos = _recvBuffer.find('\n');
        while (pos == std::string::npos) {
            std::array<char, 2048> chunk{};
            const int received = bltzr_socket::recvBytes(
                _socket, bltzr_socket::MutableBytes{reinterpret_cast<std::byte*>(chunk.data()),
                                                    chunk.size()});
            if (received <= 0)
                return false;
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
    }
    catch (const std::exception& ex) {
        std::cerr << serverClientError("readLine", ex.what()) << "\n";
        return false;
    }
    catch (...) {
        std::cerr << serverClientError("readLine", "non-standard exception") << "\n";
        return false;
    }
}

/*
 * @brief Documents the send json operation contract.
 * @param jsonLine Input value used by this contract.
 * @return ServerClientResponse ServerClient:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
ServerClientResponse ServerClient::sendJson(const std::string& jsonLine)
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
        bltzr_protocol::ServerResponseEnvelope envelope{};
        if (!bltzr_protocol::ServerJsonCodec::parseResponseEnvelope(response.raw, envelope,
                                                                    response.error)) {
            response.error = serverClientError("sendJson", "invalid response");
            return response;
        }
        response.ok = envelope.ok;
        response.error = envelope.error;
        return response;
    }
    catch (const std::exception& ex) {
        response.ok = false;
        response.error = serverClientError("sendJson", ex.what());
        disconnect();
        return response;
    }
    catch (...) {
        response.ok = false;
        response.error = serverClientError("sendJson", "non-standard exception");
        disconnect();
        return response;
    }
}

/*
 * @brief Documents the send command operation contract.
 * @param cmd Input value used by this contract.
 * @param fieldsJson Input value used by this contract.
 * @return ServerClientResponse ServerClient:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
ServerClientResponse ServerClient::sendCommand(const std::string& cmd,
                                               const std::string& fieldsJson)
{
    bltzr_protocol::ServerCommandRequest request{};
    request.cmd = cmd;
    request.token = _authToken;
    return sendJson(bltzr_protocol::ServerJsonCodec::makeCommandRequest(request, fieldsJson));
}

/*
 * @brief Documents the get status operation contract.
 * @param outStatus Input value used by this contract.
 * @return ServerClientResponse ServerClient:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
ServerClientResponse ServerClient::getStatus(ServerClientStatus& outStatus)
{
    try {
        ServerClientResponse response = sendCommand(std::string(bltzr_protocol::Status));
        if (!response.ok)
            return response;
        bltzr_protocol::ServerStatusPayload parsed{};
        std::string parseError;
        if (!bltzr_protocol::ServerJsonCodec::parseStatusResponse(response.raw, parsed,
                                                                  parseError)) {
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
        outStatus.gpuTelemetryEnabled = parsed.gpuTelemetryEnabled;
        outStatus.gpuTelemetryAvailable = parsed.gpuTelemetryAvailable;
        outStatus.gpuKernelMs = parsed.gpuKernelMs;
        outStatus.gpuCopyMs = parsed.gpuCopyMs;
        outStatus.gpuVramUsedBytes = parsed.gpuVramUsedBytes;
        outStatus.gpuVramTotalBytes = parsed.gpuVramTotalBytes;
        outStatus.exportQueueDepth = parsed.exportQueueDepth;
        outStatus.exportActive = parsed.exportActive;
        outStatus.exportCompletedCount = parsed.exportCompletedCount;
        outStatus.exportFailedCount = parsed.exportFailedCount;
        outStatus.exportLastState = parsed.exportLastState;
        outStatus.exportLastPath = parsed.exportLastPath;
        outStatus.exportLastMessage = parsed.exportLastMessage;
        return response;
    }
    catch (const std::exception& ex) {
        ServerClientResponse response;
        response.error = serverClientError("getStatus", ex.what());
        return response;
    }
    catch (...) {
        ServerClientResponse response;
        response.error = serverClientError("getStatus", "non-standard exception");
        return response;
    }
}

/*
 * @brief Documents the get snapshot operation contract.
 * @param outSnapshot Input value used by this contract.
 * @param maxPoints Input value used by this contract.
 * @param outSourceSize Input value used by this contract.
 * @return ServerClientResponse ServerClient:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
ServerClientResponse ServerClient::getSnapshot(std::vector<RenderParticle>& outSnapshot,
                                               std::uint32_t maxPoints, std::size_t* outSourceSize)
{
    try {
        maxPoints = bltzr_protocol::clampSnapshotPoints(maxPoints);
        ServerClientResponse response = sendCommand(std::string(bltzr_protocol::GetSnapshot),
                                                    "\"max_points\":" + std::to_string(maxPoints));
        if (!response.ok)
            return response;
        bltzr_protocol::ServerSnapshotPayload parsed{};
        std::string parseError;
        if (!bltzr_protocol::ServerJsonCodec::parseSnapshotResponse(response.raw, parsed,
                                                                    parseError)) {
            response.ok = false;
            response.error = serverClientError("getSnapshot", parseError);
            return response;
        }
        if (outSourceSize != nullptr) {
            *outSourceSize = parsed.sourceSize;
        }
        outSnapshot = std::move(parsed.particles);
        return response;
    }
    catch (const std::exception& ex) {
        ServerClientResponse response;
        response.error = serverClientError("getSnapshot", ex.what());
        return response;
    }
    catch (...) {
        ServerClientResponse response;
        response.error = serverClientError("getSnapshot", "non-standard exception");
        return response;
    }
}
