#include "protocol/BackendClientRequests.hpp"
#include "protocol/BackendProtocol.hpp"
#include "platform/SocketPlatform.hpp"

#include <exception>
#include <string>
#include <string_view>
#include <utility>

static std::string backendClientError(std::string_view operation, std::string_view detail)
{
    return std::string("[backend-client] ") + std::string(operation) + ": " + std::string(detail);
}

int BackendClient::socketTimeoutMs() const
{
    return _socketTimeoutMs;
}

void BackendClient::setSocketTimeoutMs(int timeoutMs)
{
    _socketTimeoutMs = grav_socket::clampTimeoutMs(timeoutMs);
    if (grav_socket::isValid(_socket)) {
        grav_socket::setSocketTimeoutMs(_socket, _socketTimeoutMs);
    }
}

void BackendClient::setAuthToken(std::string token)
{
    _authToken = std::move(token);
}

bool BackendClient::isConnected() const
{
    return grav_socket::isValid(_socket);
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
            response.error = backendClientError("getStatus", parseError);
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
        response.error = backendClientError("getStatus", ex.what());
        return response;
    } catch (...) {
        BackendClientResponse response;
        response.error = backendClientError("getStatus", "non-standard exception");
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
            response.error = backendClientError("getSnapshot", parseError);
            return response;
        }

        outSnapshot = std::move(parsed.particles);
        return response;
    } catch (const std::exception &ex) {
        BackendClientResponse response;
        response.error = backendClientError("getSnapshot", ex.what());
        return response;
    } catch (...) {
        BackendClientResponse response;
        response.error = backendClientError("getSnapshot", "non-standard exception");
        return response;
    }
}
