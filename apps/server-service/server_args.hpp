#ifndef GRAVITY_APPS_SERVER_SERVICE_SERVER_ARGS_HPP_
#define GRAVITY_APPS_SERVER_SERVICE_SERVER_ARGS_HPP_
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>
namespace grav_server_service {
struct DaemonOptions {
    std::string host = "127.0.0.1";
    std::uint16_t port = 4545;
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
} // namespace grav_server_service
#endif // GRAVITY_APPS_SERVER_SERVICE_SERVER_ARGS_HPP_
