#include "server/ServerDaemon.hpp"
#include "server/daemon/protocol/dispatcher.hpp"
#include "server/daemon/protocol/parser.hpp"
#include <string>

std::string ServerDaemon::processRequest(const std::string& request)
{
    try {
        DaemonProtocolRequest parsedRequest{};
        std::string errorResponse;
        if (!parseDaemonProtocolRequest(*this, request, parsedRequest, errorResponse)) {
            return errorResponse;
        }
        return dispatchDaemonProtocolRequest(*this, parsedRequest);
    }
    catch (const std::exception& ex) {
        return bltzr_protocol::ServerJsonCodec::makeErrorResponse(
            "request", std::string("[ipc] processRequest: ") + ex.what());
    }
    catch (...) {
        return bltzr_protocol::ServerJsonCodec::makeErrorResponse(
            "request", "[ipc] processRequest: non-standard exception");
    }
}
