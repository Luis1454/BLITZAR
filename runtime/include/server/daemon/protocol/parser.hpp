#ifndef BLITZAR_RUNTIME_INCLUDE_SERVER_DAEMON_PROTOCOL_PARSER_HPP_
#define BLITZAR_RUNTIME_INCLUDE_SERVER_DAEMON_PROTOCOL_PARSER_HPP_

#include "protocol/ServerJsonCodec.hpp"
#include <string>

class ServerDaemon;

struct DaemonProtocolRequest {
    std::string rawRequest;
    bltzr_protocol::ServerCommandRequest envelope;
};

bool parseDaemonProtocolRequest(const ServerDaemon& daemon,
                                const std::string& request,
                                DaemonProtocolRequest& parsedRequest,
                                std::string& errorResponse);

#endif // BLITZAR_RUNTIME_INCLUDE_SERVER_DAEMON_PROTOCOL_PARSER_HPP_
