// File: runtime/src/protocol/ServerJsonCodec.cpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#include "protocol/ServerJsonCodec.hpp"
#include "protocol/ServerProtocol.hpp"
#include <iomanip>
#include <sstream>
namespace grav_protocol {
class ServerJsonObjectWriter final {
public:
    explicit ServerJsonObjectWriter(std::ostringstream& out) : _out(out), _hasField(false)
    {
        _out << "{";
    }
    void writeBool(std::string_view key, bool value)
    {
        writeKey(key);
        _out << (value ? "true" : "false");
    }
    template <typename NumberType> void writeNumber(std::string_view key, NumberType value)
    {
        writeKey(key);
        _out << value;
    }
    void writeString(std::string_view key, std::string_view value)
    {
        writeKey(key);
        _out << "\"" << ServerJsonCodec::escapeString(value) << "\"";
    }
    void beginArray(std::string_view key)
    {
        writeKey(key);
        _out << "[";
    }
    void writeArraySeparator()
    {
        _out << ",";
    }
    void writeRaw(std::string_view raw)
    {
        _out << raw;
    }
    void endArray()
    {
        _out << "]";
    }
    void finish()
    {
        _out << "}";
    }

private:
    void writeKey(std::string_view key)
    {
        if (_hasField) {
            _out << ",";
        }
        _out << "\"" << key << "\":";
        _hasField = true;
    }
    std::ostringstream& _out;
    bool _hasField;
};
std::string ServerJsonCodec::escapeString(std::string_view value)
{
    std::string escaped;
    escaped.reserve(value.size() + 8u);
    for (unsigned char c : value)
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
    return escaped;
}
std::string ServerJsonCodec::makeCommandRequest(const ServerCommandRequest& request,
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
std::string ServerJsonCodec::makeOkResponse(std::string_view cmd)
{
    std::ostringstream out;
    ServerJsonObjectWriter writer(out);
    writer.writeBool("ok", true);
    writer.writeString("cmd", cmd);
    writer.finish();
    return out.str();
}
std::string ServerJsonCodec::makeErrorResponse(std::string_view cmd, std::string_view message)
{
    std::ostringstream out;
    ServerJsonObjectWriter writer(out);
    writer.writeBool("ok", false);
    writer.writeString("cmd", cmd);
    writer.writeString("error", message);
    writer.finish();
    return out.str();
}
std::string ServerJsonCodec::makeStatusResponse(const SimulationStats& stats)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(6);
    ServerJsonObjectWriter writer(out);
    writer.writeBool("ok", true);
    writer.writeString("cmd", Status);
    writer.writeNumber("steps", stats.steps);
    writer.writeNumber("dt", stats.dt);
    writer.writeNumber("total_time", stats.totalTime);
    writer.writeBool("paused", stats.paused);
    writer.writeBool("faulted", stats.faulted);
    writer.writeNumber("fault_step", stats.faultStep);
    writer.writeString("fault_reason", stats.faultReason);
    writer.writeBool("sph", stats.sphEnabled);
    writer.writeNumber("server_fps", stats.serverFps);
    writer.writeString("performance_profile", stats.performanceProfile);
    writer.writeNumber("substep_target_dt", stats.substepTargetDt);
    writer.writeNumber("substep_dt", stats.substepDt);
    writer.writeNumber("substeps", stats.substeps);
    writer.writeNumber("max_substeps", stats.maxSubsteps);
    writer.writeNumber("snapshot_publish_period_ms", stats.snapshotPublishPeriodMs);
    writer.writeNumber("particles", stats.particleCount);
    writer.writeString("solver", stats.solverName);
    writer.writeString("integrator", stats.integratorName);
    writer.writeNumber("ekin", stats.kineticEnergy);
    writer.writeNumber("epot", stats.potentialEnergy);
    writer.writeNumber("eth", stats.thermalEnergy);
    writer.writeNumber("erad", stats.radiatedEnergy);
    writer.writeNumber("etot", stats.totalEnergy);
    writer.writeNumber("drift_pct", stats.energyDriftPct);
    writer.writeBool("estimated", stats.energyEstimated);
    writer.writeBool("gpu_telemetry_enabled", stats.gpuTelemetryEnabled);
    writer.writeBool("gpu_telemetry_available", stats.gpuTelemetryAvailable);
    writer.writeNumber("gpu_kernel_ms", stats.gpuKernelMs);
    writer.writeNumber("gpu_copy_ms", stats.gpuCopyMs);
    writer.writeNumber("gpu_vram_used_bytes", stats.gpuVramUsedBytes);
    writer.writeNumber("gpu_vram_total_bytes", stats.gpuVramTotalBytes);
    writer.writeNumber("export_queue_depth", stats.exportQueueDepth);
    writer.writeBool("export_active", stats.exportActive);
    writer.writeNumber("export_completed_count", stats.exportCompletedCount);
    writer.writeNumber("export_failed_count", stats.exportFailedCount);
    writer.writeString("export_last_state", stats.exportLastState);
    writer.writeString("export_last_path", stats.exportLastPath);
    writer.writeString("export_last_message", stats.exportLastMessage);
    writer.finish();
    return out.str();
}
std::string ServerJsonCodec::makeSnapshotResponse(bool hasSnapshot,
                                                  const std::vector<RenderParticle>& snapshot,
                                                  std::size_t sourceSize)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(6);
    ServerJsonObjectWriter writer(out);
    writer.writeBool("ok", true);
    writer.writeString("cmd", GetSnapshot);
    writer.writeBool("has_snapshot", hasSnapshot);
    writer.writeNumber("count", snapshot.size());
    writer.writeNumber("source_count", sourceSize == 0u ? snapshot.size() : sourceSize);
    writer.beginArray("particles");
    for (std::size_t i = 0; i < snapshot.size(); ++i) {
        const RenderParticle& particle = snapshot[i];
        if (i > 0) {
            writer.writeArraySeparator();
        }
        writer.writeRaw("[");
        writer.writeRaw(std::to_string(particle.x));
        writer.writeArraySeparator();
        writer.writeRaw(std::to_string(particle.y));
        writer.writeArraySeparator();
        writer.writeRaw(std::to_string(particle.z));
        writer.writeArraySeparator();
        writer.writeRaw(std::to_string(particle.mass));
        writer.writeArraySeparator();
        writer.writeRaw(std::to_string(particle.pressureNorm));
        writer.writeArraySeparator();
        writer.writeRaw(std::to_string(particle.temperature));
        writer.writeRaw("]");
    }
    writer.endArray();
    writer.finish();
    return out.str();
}
} // namespace grav_protocol
