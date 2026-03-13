#include "tests/support/client_utils.hpp"

#include "client/ILocalServer.hpp"

#include <memory>

namespace testsupport {

grav_client::ClientTransportArgs makeTransport(std::uint16_t port, const std::string &serverExecutable)
{
    grav_client::ClientTransportArgs transport{};
    transport.remoteMode = true;
    transport.remoteHost = "127.0.0.1";
    transport.remotePort = port;
    transport.remoteAutoStart = false;
    transport.serverExecutable = serverExecutable;
    transport.remoteCommandTimeoutMs = 80u;
    transport.remoteStatusTimeoutMs = 40u;
    transport.remoteSnapshotTimeoutMs = 120u;
    return transport;
}

} // namespace testsupport

namespace grav_client {

std::unique_ptr<grav_client::ILocalServer> createLocalServer(const std::string &)
{
    return nullptr;
}

} // namespace grav_client

