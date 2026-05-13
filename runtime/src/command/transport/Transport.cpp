/*
 * @file runtime/src/command/transport/Transport.cpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Runtime implementation for protocol, command, client, and FFI boundaries.
 */

#include "command/transport/Transport.hpp"

namespace bltzr_cmd {
ServerTransport::ServerTransport(int timeoutMs) : _client()
{
    _client.setSocketTimeoutMs(timeoutMs);
}

bool ServerTransport::connect(const std::string& host, std::uint16_t port)
{
    return _client.connect(host, port);
}

void ServerTransport::disconnect()
{
    _client.disconnect();
}

bool ServerTransport::isConnected() const
{
    return _client.isConnected();
}

bltzr_protocol::Response ServerTransport::sendCommand(const std::string& cmd,
                                                               const std::string& fieldsJson)
{
    return _client.sendCommand(cmd, fieldsJson);
}

bltzr_protocol::Response ServerTransport::getStatus(bltzr_protocol::ClientStatus& outStatus)
{
    return _client.getStatus(outStatus);
}
} // namespace bltzr_cmd
