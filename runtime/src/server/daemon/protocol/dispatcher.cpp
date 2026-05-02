/*
 * @file runtime/src/server/daemon/protocol/dispatcher.cpp
 * @brief Route parsed daemon protocol requests to the correct command family.
 */

#include "server/ServerDaemon.hpp"
#include "server/daemon/protocol/dispatcher.hpp"
#include "protocol/ServerJsonCodec.hpp"
#include "protocol/ServerProtocol.hpp"

std::string dispatchDaemonProtocolRequest(ServerDaemon& daemon,
                                          const DaemonProtocolRequest& parsedRequest)
{
    const std::string& cmd = parsedRequest.envelope.cmd;
    if (cmd == bltzr_protocol::Status || cmd == bltzr_protocol::GetSnapshot ||
        cmd == bltzr_protocol::Pause || cmd == bltzr_protocol::Resume ||
        cmd == bltzr_protocol::Toggle || cmd == bltzr_protocol::Reset ||
        cmd == bltzr_protocol::Recover || cmd == bltzr_protocol::Step) {
        const std::string response = dispatchDaemonStateCommand(daemon, parsedRequest);
        if (!response.empty())
            return response;
    }
    else if (cmd == bltzr_protocol::SetDt || cmd == bltzr_protocol::SetSolver ||
             cmd == bltzr_protocol::SetIntegrator || cmd == bltzr_protocol::SetPerformanceProfile ||
             cmd == bltzr_protocol::SetParticleCount || cmd == bltzr_protocol::SetSph ||
             cmd == bltzr_protocol::SetOctree || cmd == bltzr_protocol::SetSphParams ||
             cmd == bltzr_protocol::SetSubsteps || cmd == bltzr_protocol::SetEnergyMeasure ||
             cmd == bltzr_protocol::SetGpuTelemetry || cmd == bltzr_protocol::SetSnapshotPublishCadence ||
             cmd == bltzr_protocol::SetSnapshotTransferCap || cmd == bltzr_protocol::Load ||
             cmd == bltzr_protocol::Export || cmd == bltzr_protocol::SaveCheckpoint ||
             cmd == bltzr_protocol::LoadCheckpoint || cmd == bltzr_protocol::Shutdown) {
        const std::string response = dispatchDaemonConfigCommand(daemon, parsedRequest);
        if (!response.empty())
            return response;
    }
    return bltzr_protocol::ServerJsonCodec::makeErrorResponse(cmd, "unknown command");
}
