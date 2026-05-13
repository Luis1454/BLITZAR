/*
 * @file runtime/include/client/runtime/BridgeState.hpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Runtime public interfaces for protocol, command, client, and FFI boundaries.
 */

#ifndef BLITZAR_RUNTIME_INCLUDE_CLIENT_RUSTRUNTIMEBRIDGESTATE_HPP_
#define BLITZAR_RUNTIME_INCLUDE_CLIENT_RUSTRUNTIMEBRIDGESTATE_HPP_
#include "ffi/bridge/Api.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace bltzr_client {

class BridgeState {
public:
    BridgeState();
    ~BridgeState();
    BridgeState(const BridgeState&) = delete;
    BridgeState& operator=(const BridgeState&) = delete;
    void setConnected(bool connected);
    bool isConnected() const;
    void setServerLaunched(bool launched);
    bool serverLaunched() const;
    std::uint32_t setRemoteSnapshotCap(std::uint32_t requested);
    std::uint32_t remoteSnapshotCap() const;
    void clearPendingCommands();
    bool queuePendingCommand(const std::string& cmd, const std::string& fields);
    std::size_t pendingCommandCount() const;
    std::pair<std::string, std::string> pendingCommandAt(std::size_t index) const;
    void erasePendingPrefix(std::size_t count);
    std::string linkStateLabel() const;
    std::string serverOwnerLabel() const;

private:
    static std::string copyStringView(blitzar_runtime_string_view view);
    blitzar_runtime_bridge_t* _state;
};

} // namespace bltzr_client

#endif // BLITZAR_RUNTIME_INCLUDE_CLIENT_RUSTRUNTIMEBRIDGESTATE_HPP_
