#include "protocol/ServerJsonCodec.hpp"

namespace grav_protocol {

bool ServerJsonCodec::parseStatusResponse(std::string_view raw, ServerStatusPayload &out, std::string &error)
{
    ServerStatusPayload parsed{};
    if (!parseResponseEnvelope(raw, parsed.envelope, error)) {
        return false;
    }
    if (!parsed.envelope.ok) {
        out = parsed;
        return true;
    }
    readNumber(raw, "steps", parsed.steps);
    readNumber(raw, "dt", parsed.dt);
    readBool(raw, "paused", parsed.paused);
    readBool(raw, "faulted", parsed.faulted);
    readNumber(raw, "fault_step", parsed.faultStep);
    readString(raw, "fault_reason", parsed.faultReason);
    readBool(raw, "sph", parsed.sphEnabled);
    readNumber(raw, "server_fps", parsed.serverFps);
    readNumber(raw, "particles", parsed.particleCount);
    readString(raw, "solver", parsed.solver);
    readString(raw, "integrator", parsed.integrator);
    readNumber(raw, "ekin", parsed.kineticEnergy);
    readNumber(raw, "epot", parsed.potentialEnergy);
    readNumber(raw, "eth", parsed.thermalEnergy);
    readNumber(raw, "erad", parsed.radiatedEnergy);
    readNumber(raw, "etot", parsed.totalEnergy);
    readNumber(raw, "drift_pct", parsed.energyDriftPct);
    readBool(raw, "estimated", parsed.energyEstimated);
    out = parsed;
    error.clear();
    return true;
}

} // namespace grav_protocol
