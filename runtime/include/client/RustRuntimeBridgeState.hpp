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
/// Description: Defines the RustRuntimeBridgeState data or behavior contract.
class RustRuntimeBridgeState {
public:
    /// Description: Executes the RustRuntimeBridgeState operation.
    RustRuntimeBridgeState();
    /// Description: Releases resources owned by RustRuntimeBridgeState.
    ~RustRuntimeBridgeState();
    RustRuntimeBridgeState(const RustRuntimeBridgeState&) = delete;
    RustRuntimeBridgeState& operator=(const RustRuntimeBridgeState&) = delete;
    /// Description: Executes the setConnected operation.
    void setConnected(bool connected);
    /// Description: Executes the isConnected operation.
    bool isConnected() const;
    /// Description: Executes the setServerLaunched operation.
    void setServerLaunched(bool launched);
    /// Description: Executes the serverLaunched operation.
    bool serverLaunched() const;
    /// Description: Executes the setRemoteSnapshotCap operation.
    std::uint32_t setRemoteSnapshotCap(std::uint32_t requested);
    /// Description: Executes the remoteSnapshotCap operation.
    std::uint32_t remoteSnapshotCap() const;
    /// Description: Executes the clearPendingCommands operation.
    void clearPendingCommands();
    /// Description: Executes the queuePendingCommand operation.
    bool queuePendingCommand(const std::string& cmd, const std::string& fields);
    /// Description: Executes the pendingCommandCount operation.
    std::size_t pendingCommandCount() const;
    /// Description: Executes the pendingCommandAt operation.
    std::pair<std::string, std::string> pendingCommandAt(std::size_t index) const;
    /// Description: Executes the erasePendingPrefix operation.
    void erasePendingPrefix(std::size_t count);
    /// Description: Executes the linkStateLabel operation.
    std::string linkStateLabel() const;
    /// Description: Executes the serverOwnerLabel operation.
    std::string serverOwnerLabel() const;

private:
    /// Description: Executes the copyStringView operation.
    static std::string copyStringView(blitzar_runtime_string_view view);
    blitzar_runtime_bridge_t* _state;
};
} // namespace grav_client
#endif // GRAVITY_RUNTIME_INCLUDE_CLIENT_RUSTRUNTIMEBRIDGESTATE_HPP_
