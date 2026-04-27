// File: apps/server-service/server_args.hpp
// Purpose: Application entry point or host support for BLITZAR executables.

#ifndef GRAVITY_APPS_SERVER_SERVICE_SERVER_ARGS_HPP_
#define GRAVITY_APPS_SERVER_SERVICE_SERVER_ARGS_HPP_
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace grav_server_service {
/// Description: Defines the DaemonOptions data or behavior contract.
struct DaemonOptions {
    std::string host = "127.0.0.1";
    std::uint16_t port = 4545;
    std::string authToken;
    bool allowRemoteBind = false;
    bool startPaused = false;
    bool showServerHelp = false;
    std::vector<std::string_view> simArgs;
};

/// Description: Describes the parse server args operation contract.
bool parseServerArgs(const std::vector<std::string_view>& rawArgs, DaemonOptions& outOptions,
                     std::ostream& outError);
/// Description: Executes the printServerHelp operation.
void printServerHelp(std::string_view programName);
/// Description: Executes the isLoopbackBindHost operation.
bool isLoopbackBindHost(std::string_view host);
/// Description: Executes the installStopSignalHandlers operation.
void installStopSignalHandlers();
/// Description: Executes the stopRequested operation.
bool stopRequested();
/// Description: Executes the resetStopRequested operation.
void resetStopRequested();
} // namespace grav_server_service
#endif // GRAVITY_APPS_SERVER_SERVICE_SERVER_ARGS_HPP_
