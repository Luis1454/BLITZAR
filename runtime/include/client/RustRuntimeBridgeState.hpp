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
    /// Description: Describes the rust runtime bridge state operation contract.
    RustRuntimeBridgeState();
    /// Description: Releases resources owned by RustRuntimeBridgeState.
    ~RustRuntimeBridgeState();
    /// Description: Describes the rust runtime bridge state operation contract.
    RustRuntimeBridgeState(const RustRuntimeBridgeState&) = delete;
    /// Description: Describes the operator= operation contract.
    RustRuntimeBridgeState& operator=(const RustRuntimeBridgeState&) = delete;
    /// Description: Describes the set connected operation contract.
    void setConnected(bool connected);
    /// Description: Describes the is connected operation contract.
    bool isConnected() const;
    /// Description: Describes the set server launched operation contract.
    void setServerLaunched(bool launched);
    /// Description: Describes the server launched operation contract.
    bool serverLaunched() const;
    /// Description: Describes the set remote snapshot cap operation contract.
    std::uint32_t setRemoteSnapshotCap(std::uint32_t requested);
    /// Description: Describes the remote snapshot cap operation contract.
    std::uint32_t remoteSnapshotCap() const;
    /// Description: Describes the clear pending commands operation contract.
    void clearPendingCommands();
    /// Description: Describes the queue pending command operation contract.
    bool queuePendingCommand(const std::string& cmd, const std::string& fields);
    /// Description: Describes the pending command count operation contract.
    std::size_t pendingCommandCount() const;
    /// Description: Describes the pending command at operation contract.
    std::pair<std::string, std::string> pendingCommandAt(std::size_t index) const;
    /// Description: Describes the erase pending prefix operation contract.
    void erasePendingPrefix(std::size_t count);
    /// Description: Describes the link state label operation contract.
    std::string linkStateLabel() const;
    /// Description: Describes the server owner label operation contract.
    std::string serverOwnerLabel() const;

private:
    /// Description: Describes the copy string view operation contract.
    static std::string copyStringView(blitzar_runtime_string_view view);
    blitzar_runtime_bridge_t* _state;
};
} // namespace grav_client
#endif // GRAVITY_RUNTIME_INCLUDE_CLIENT_RUSTRUNTIMEBRIDGESTATE_HPP_
