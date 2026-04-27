// File: runtime/src/command/CommandTransport.cpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#include "command/CommandTransport.hpp"

namespace grav_cmd {
/// Description: Executes the ServerClientCommandTransport operation.
ServerClientCommandTransport::ServerClientCommandTransport(int timeoutMs) : _client()
{
    _client.setSocketTimeoutMs(timeoutMs);
}

/// Description: Executes the connect operation.
bool ServerClientCommandTransport::connect(const std::string& host, std::uint16_t port)
{
    return _client.connect(host, port);
}

/// Description: Executes the disconnect operation.
void ServerClientCommandTransport::disconnect()
{
    _client.disconnect();
}

/// Description: Executes the isConnected operation.
bool ServerClientCommandTransport::isConnected() const
{
    return _client.isConnected();
}

/// Description: Describes the send command operation contract.
ServerClientResponse ServerClientCommandTransport::sendCommand(const std::string& cmd,
                                                               const std::string& fieldsJson)
{
    return _client.sendCommand(cmd, fieldsJson);
}

/// Description: Executes the getStatus operation.
ServerClientResponse ServerClientCommandTransport::getStatus(ServerClientStatus& outStatus)
{
    return _client.getStatus(outStatus);
}
} // namespace grav_cmd
