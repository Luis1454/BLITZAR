#include "tests/support/frontend_utils.hpp"

#include "frontend/ILocalBackend.hpp"

#include <memory>

namespace testsupport {

grav_frontend::FrontendTransportArgs makeTransport(std::uint16_t port, const std::string &backendExecutable)
{
    grav_frontend::FrontendTransportArgs transport{};
    transport.remoteMode = true;
    transport.remoteHost = "127.0.0.1";
    transport.remotePort = port;
    transport.remoteAutoStart = false;
    transport.backendExecutable = backendExecutable;
    transport.remoteCommandTimeoutMs = 80u;
    transport.remoteStatusTimeoutMs = 40u;
    transport.remoteSnapshotTimeoutMs = 120u;
    return transport;
}

} // namespace testsupport

namespace grav_frontend {

std::unique_ptr<grav_frontend::ILocalBackend> createLocalBackend(const std::string &)
{
    return nullptr;
}

} // namespace grav_frontend

