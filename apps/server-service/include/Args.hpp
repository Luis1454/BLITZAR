/*
 * @file apps/server-service/include/Args.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Application entry points and host executables for BLITZAR.
 */

#ifndef BLITZAR_APPS_SERVER_SERVICE_ARGS_HPP_
#define BLITZAR_APPS_SERVER_SERVICE_ARGS_HPP_
#include "Constants.hpp"
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace bltzr_server_service {
struct DaemonOptions {
    std::string host = kDefaultLoopbackHost;
    std::uint16_t port = kDefaultServerPort;
    std::string authToken;
    bool allowRemoteBind = false;
    bool startPaused = false;
    bool showServerHelp = false;
    std::vector<std::string_view> simArgs;
};

bool parseServerArgs(const std::vector<std::string_view>& rawArgs, DaemonOptions& outOptions,
                     std::ostream& outError);
void printServerHelp(std::string_view programName);
bool isLoopbackBindHost(std::string_view host);
void installStopSignalHandlers();
bool stopRequested();
void resetStopRequested();
} // namespace bltzr_server_service
#endif // BLITZAR_APPS_SERVER_SERVICE_ARGS_HPP_
