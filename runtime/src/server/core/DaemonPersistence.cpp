/*
 * @file runtime/src/server/core/DaemonPersistence.cpp
 * @brief Runtime daemon load, export, and checkpoint commands.
 */

#include "server/core/Daemon.hpp"

#include "protocol/Protocol.hpp"
#include "protocol/codec/JsonCodec.hpp"
#include "server/SimulationServer.hpp"

std::optional<std::string> Daemon::processPersistenceCommand(const std::string& request,
                                                             const std::string& command)
{
    if (command == bltzr_protocol::Load) {
        std::string path;
        if (!bltzr_protocol::JsonCodec::readString(request, "path", path) || path.empty()) {
            return bltzr_protocol::JsonCodec::makeErrorResponse(command, "missing path");
        }
        std::string format = "auto";
        bltzr_protocol::JsonCodec::readString(request, "format", format);
        _server.setInitialStateFile(path, format);
        _server.requestReset();
        return bltzr_protocol::JsonCodec::makeOkResponse(command);
    }
    if (command == bltzr_protocol::Export) {
        std::string path;
        std::string format;
        bltzr_protocol::JsonCodec::readString(request, "path", path);
        if (!bltzr_protocol::JsonCodec::readString(request, "format", format)) {
            format = "vtk";
        }
        _server.requestExportSnapshot(path, format);
        return bltzr_protocol::JsonCodec::makeOkResponse(command);
    }
    if (command == bltzr_protocol::SaveCheckpoint) {
        std::string path;
        if (!bltzr_protocol::JsonCodec::readString(request, "path", path) || path.empty()) {
            return bltzr_protocol::JsonCodec::makeErrorResponse(command, "missing path");
        }
        if (!_server.saveCheckpoint(path)) {
            return bltzr_protocol::JsonCodec::makeErrorResponse(command,
                                                                "failed to save checkpoint");
        }
        return bltzr_protocol::JsonCodec::makeOkResponse(command);
    }
    if (command == bltzr_protocol::LoadCheckpoint) {
        std::string path;
        if (!bltzr_protocol::JsonCodec::readString(request, "path", path) || path.empty()) {
            return bltzr_protocol::JsonCodec::makeErrorResponse(command, "missing path");
        }
        std::string error;
        if (!_server.loadCheckpoint(path, &error)) {
            return bltzr_protocol::JsonCodec::makeErrorResponse(
                command, error.empty() ? "failed to load checkpoint" : error);
        }
        return bltzr_protocol::JsonCodec::makeOkResponse(command);
    }
    return std::nullopt;
}
