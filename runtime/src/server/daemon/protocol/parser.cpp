/*
 * @file runtime/src/server/daemon/protocol/parser.cpp
 * @brief Parse and authorize daemon protocol requests.
 */

#include "server/ServerDaemon.hpp"
#include "server/daemon/protocol/parser.hpp"

bool parseDaemonProtocolRequest(const ServerDaemon& daemon,
                                const std::string& request,
                                DaemonProtocolRequest& parsedRequest,
                                std::string& errorResponse)
{
    parsedRequest.rawRequest = request;
    parsedRequest.envelope = {};

    std::string parseError;
    if (!bltzr_protocol::ServerJsonCodec::parseCommandRequest(
            request, parsedRequest.envelope, parseError)) {
        errorResponse = bltzr_protocol::ServerJsonCodec::makeErrorResponse("unknown", parseError);
        return false;
    }
    if (!daemon._authToken.empty() && parsedRequest.envelope.token != daemon._authToken) {
        errorResponse = bltzr_protocol::ServerJsonCodec::makeErrorResponse("auth", "unauthorized");
        return false;
    }
    return true;
}
