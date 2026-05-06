#ifndef BLITZAR_RUNTIME_INCLUDE_SERVER_DAEMON_PROTOCOL_DISPATCHER_HPP_
#define BLITZAR_RUNTIME_INCLUDE_SERVER_DAEMON_PROTOCOL_DISPATCHER_HPP_

#include "server/daemon/protocol/parser.hpp"
#include <string>

class ServerDaemon;

std::string dispatchDaemonProtocolRequest(ServerDaemon& daemon,
                                          const DaemonProtocolRequest& parsedRequest);
std::string dispatchDaemonStateCommand(ServerDaemon& daemon,
                                      const DaemonProtocolRequest& parsedRequest);
std::string dispatchDaemonConfigCommand(ServerDaemon& daemon,
                                       const DaemonProtocolRequest& parsedRequest);

#endif // BLITZAR_RUNTIME_INCLUDE_SERVER_DAEMON_PROTOCOL_DISPATCHER_HPP_
