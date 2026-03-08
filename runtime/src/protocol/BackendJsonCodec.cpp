#include "protocol/BackendJsonCodec.hpp"

#include "protocol/BackendProtocol.hpp"

#include <iomanip>
#include <sstream>

namespace grav_protocol {

std::string BackendJsonCodec::escapeString(std::string_view value)
{
    std::string escaped;
    escaped.reserve(value.size() + 8u);
    for (unsigned char c : value) {
        switch (c) {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped.push_back(static_cast<char>(c));
                break;
        }
    }
    return escaped;
}

std::string BackendJsonCodec::makeCommandRequest(
    const BackendCommandRequest &request,
    std::string_view fieldsJson)
{
    std::string payload = std::string("{\"cmd\":\"") + escapeString(request.cmd) + "\"";
    if (!request.token.empty()) {
        payload += ",\"token\":\"";
        payload += escapeString(request.token);
        payload += "\"";
    }
    if (!fieldsJson.empty()) {
        payload += ",";
        payload += fieldsJson;
    }
    payload += "}";
    return payload;
}

std::string BackendJsonCodec::makeOkResponse(std::string_view cmd)
{
    return std::string("{\"ok\":true,\"cmd\":\"") + escapeString(cmd) + "\"}";
}

std::string BackendJsonCodec::makeErrorResponse(std::string_view cmd, std::string_view message)
{
    return std::string("{\"ok\":false,\"cmd\":\"") + escapeString(cmd)
        + "\",\"error\":\"" + escapeString(message) + "\"}";
}

std::string BackendJsonCodec::makeStatusResponse(const SimulationStats &stats)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(6)
        << "{\"ok\":true,\"cmd\":\"" << Status << "\""
        << ",\"steps\":" << stats.steps
        << ",\"dt\":" << stats.dt
        << ",\"paused\":" << (stats.paused ? "true" : "false")
        << ",\"faulted\":" << (stats.faulted ? "true" : "false")
        << ",\"fault_step\":" << stats.faultStep
        << ",\"fault_reason\":\"" << escapeString(stats.faultReason) << "\""
        << ",\"sph\":" << (stats.sphEnabled ? "true" : "false")
        << ",\"backend_fps\":" << stats.backendFps
        << ",\"particles\":" << stats.particleCount
        << ",\"solver\":\"" << escapeString(stats.solverName) << "\""
        << ",\"integrator\":\"" << escapeString(stats.integratorName) << "\""
        << ",\"ekin\":" << stats.kineticEnergy
        << ",\"epot\":" << stats.potentialEnergy
        << ",\"eth\":" << stats.thermalEnergy
        << ",\"erad\":" << stats.radiatedEnergy
        << ",\"etot\":" << stats.totalEnergy
        << ",\"drift_pct\":" << stats.energyDriftPct
        << ",\"estimated\":" << (stats.energyEstimated ? "true" : "false")
        << "}";
    return out.str();
}

std::string BackendJsonCodec::makeSnapshotResponse(
    bool hasSnapshot,
    const std::vector<RenderParticle> &snapshot)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(6)
        << "{\"ok\":true,\"cmd\":\"" << GetSnapshot << "\""
        << ",\"has_snapshot\":" << (hasSnapshot ? "true" : "false")
        << ",\"count\":" << snapshot.size()
        << ",\"particles\":[";
    for (std::size_t i = 0; i < snapshot.size(); ++i) {
        const RenderParticle &particle = snapshot[i];
        if (i > 0) {
            out << ",";
        }
        out << "["
            << particle.x << ","
            << particle.y << ","
            << particle.z << ","
            << particle.mass << ","
            << particle.pressureNorm << ","
            << particle.temperature << "]";
    }
    out << "]}";
    return out.str();
}

} // namespace grav_protocol
