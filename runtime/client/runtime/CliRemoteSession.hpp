/*
 * @file runtime/client/runtime/CliRemoteSession.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Private ownership boundary for the client remote session.
 */

#ifndef BLITZAR_RUNTIME_SRC_CLIENT_RUNTIME_REMOTESESSION_HPP_
#define BLITZAR_RUNTIME_SRC_CLIENT_RUNTIME_REMOTESESSION_HPP_

#include "client/runtime/CliBridge.hpp"
#include "client/runtime/CliBridgeState.hpp"
#include "protocol/client/PtcClient.hpp"
#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace bltzr_client {

class RemoteSession final {
public:
    RemoteSession(const std::string& configPath, std::string host, std::uint16_t port,
                  bool autoStart, std::string serverExecutable, std::string authToken,
                  std::uint32_t commandTimeoutMs, std::uint32_t statusTimeoutMs,
                  std::uint32_t snapshotTimeoutMs);

    bool start();
    void stop();
    bool sendOrQueue(const std::string& command, const std::string& fields = "");
    bool configure(const std::string& host, std::uint16_t port, bool autoStart,
                   const std::string& serverExecutable);
    bool tryConsumeSnapshot(std::vector<RenderParticle>& outSnapshot, std::size_t* outSourceSize);
    SimulationStats getStats();
    void setSnapshotCap(std::uint32_t maxPoints);
    void requestReconnect();
    bool isConnected() const;
    bool launchedByClient() const;
    LinkState linkState() const;
    std::string linkStateLabel() const;
    std::string serverOwnerLabel() const;
    bool shouldAutoStart() const;

private:
    static SimulationStats fromRemoteStatus(const bltzr_protocol::ClientStatus& status);
    bool sendNow(const std::string& command, const std::string& fields);
    bool ensureConnected(bool forceLog);
    void markDisconnected(const std::string& context, const std::string& reason);
    void autoStartServer();
    void queueCommand(const std::string& command, const std::string& fields);
    void flushCommands();
    void refreshStats();

    std::string _configPath;
    std::string _host;
    std::uint16_t _port;
    bool _autoStart;
    std::string _serverExecutable;
    std::string _authToken;
    bltzr_protocol::Client _client;
    bool _launchAttempted;
    BridgeState _state;
    SimulationStats _cachedStats;
    std::chrono::steady_clock::time_point _lastReconnectAttempt;
    std::chrono::steady_clock::time_point _lastReconnectErrorLog;
    std::chrono::steady_clock::time_point _lastRemoteErrorLog;
    std::chrono::milliseconds _retryDelay;
    std::uint32_t _commandTimeoutMs;
    std::uint32_t _statusTimeoutMs;
    std::uint32_t _snapshotTimeoutMs;
};

} // namespace bltzr_client

#endif // BLITZAR_RUNTIME_SRC_CLIENT_RUNTIME_REMOTESESSION_HPP_
