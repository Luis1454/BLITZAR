#include "command/CommandTransport.hpp"

namespace grav_cmd {

ServerClientCommandTransport::ServerClientCommandTransport(int timeoutMs)
    : _client()
{
    _client.setSocketTimeoutMs(timeoutMs);
}

bool ServerClientCommandTransport::connect(const std::string &host, std::uint16_t port)
{
    return _client.connect(host, port);
}

void ServerClientCommandTransport::disconnect()
{
    _client.disconnect();
}

bool ServerClientCommandTransport::isConnected() const
{
    return _client.isConnected();
}

ServerClientResponse ServerClientCommandTransport::sendCommand(const std::string &cmd, const std::string &fieldsJson)
{
    return _client.sendCommand(cmd, fieldsJson);
}

ServerClientResponse ServerClientCommandTransport::getStatus(ServerClientStatus &outStatus)
{
    return _client.getStatus(outStatus);
}

} // namespace grav_cmd
