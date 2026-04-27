// File: runtime/include/client/RustRuntimeBridgeState.hpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#ifndef GRAVITY_RUNTIME_INCLUDE_CLIENT_RUSTRUNTIMEBRIDGESTATE_HPP_
#define GRAVITY_RUNTIME_INCLUDE_CLIENT_RUSTRUNTIMEBRIDGESTATE_HPP_
#include "ffi/BlitzarRuntimeBridgeApi.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
namespace grav_client {
class RustRuntimeBridgeState {
public:
    RustRuntimeBridgeState();
    ~RustRuntimeBridgeState();
    RustRuntimeBridgeState(const RustRuntimeBridgeState&) = delete;
    RustRuntimeBridgeState& operator=(const RustRuntimeBridgeState&) = delete;
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
} // namespace grav_client
#endif // GRAVITY_RUNTIME_INCLUDE_CLIENT_RUSTRUNTIMEBRIDGESTATE_HPP_
