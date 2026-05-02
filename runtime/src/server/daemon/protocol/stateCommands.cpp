/*
 * @file runtime/src/server/daemon/protocol/stateCommands.cpp
 * @brief Handle daemon protocol commands that control runtime state.
 */

#include "server/ServerDaemon.hpp"
#include "server/daemon/protocol/dispatcher.hpp"
#include "engine/include/server/SimulationServer.hpp"
#include "protocol/ServerJsonCodec.hpp"
#include "protocol/ServerProtocol.hpp"
#include <cstdint>
#include <string>
#include <vector>

std::string dispatchDaemonStateCommand(ServerDaemon& daemon,
                                       const DaemonProtocolRequest& parsedRequest)
{
    const std::string& cmd = parsedRequest.envelope.cmd;

    if (cmd == bltzr_protocol::Status)
        return bltzr_protocol::ServerJsonCodec::makeStatusResponse(daemon._server.getStats());

    if (cmd == bltzr_protocol::GetSnapshot) {
        std::uint32_t maxPoints = bltzr_protocol::kSnapshotDefaultPoints;
        bltzr_protocol::ServerJsonCodec::readNumber(parsedRequest.rawRequest, "max_points",
                                                    maxPoints);
        maxPoints = bltzr_protocol::clampSnapshotPoints(maxPoints);
        std::vector<RenderParticle> snapshot;
        std::size_t sourceSize = 0u;
        const bool hasSnapshot = daemon._server.copyLatestSnapshot(
            snapshot, static_cast<std::size_t>(maxPoints), &sourceSize);
        return bltzr_protocol::ServerJsonCodec::makeSnapshotResponse(hasSnapshot, snapshot,
                                                                     sourceSize);
    }

    if (cmd == bltzr_protocol::Pause) {
        daemon._server.setPaused(true);
        return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
    }
    if (cmd == bltzr_protocol::Resume) {
        daemon._server.setPaused(false);
        return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
    }
    if (cmd == bltzr_protocol::Toggle) {
        daemon._server.togglePaused();
        return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
    }
    if (cmd == bltzr_protocol::Reset) {
        daemon._server.requestReset();
        return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
    }
    if (cmd == bltzr_protocol::Recover) {
        daemon._server.requestRecover();
        return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
    }
    if (cmd == bltzr_protocol::Step) {
        int count = 1;
        bltzr_protocol::ServerJsonCodec::readNumber(parsedRequest.rawRequest, "count", count);
        if (count < 1)
            count = 1;
        else if (count > 100000)
            count = 100000;
        for (int i = 0; i < count; ++i)
            daemon._server.stepOnce();
        return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
    }

    return {};
}
