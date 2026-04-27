// File: tests/support/client_utils.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "tests/support/client_utils.hpp"
namespace testsupport {
grav_client::ClientTransportArgs makeTransport(std::uint16_t port,
                                               const std::string& serverExecutable)
{
    grav_client::ClientTransportArgs transport{};
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
