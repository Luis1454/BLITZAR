// File: runtime/src/client/RustRuntimeBridgeState.cpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#include "client/RustRuntimeBridgeState.hpp"
#include <stdexcept>
namespace grav_client {
/// Description: Executes the RustRuntimeBridgeState operation.
RustRuntimeBridgeState::RustRuntimeBridgeState() : _state(blitzar_runtime_bridge_create())
{
    if (_state == nullptr) {
        /// Description: Executes the runtime_error operation.
        throw std::runtime_error("failed to create Rust runtime bridge state");
    }
}
/// Description: Releases resources owned by RustRuntimeBridgeState.
RustRuntimeBridgeState::~RustRuntimeBridgeState()
{
    /// Description: Executes the blitzar_runtime_bridge_destroy operation.
    blitzar_runtime_bridge_destroy(_state);
}
/// Description: Executes the setConnected operation.
void RustRuntimeBridgeState::setConnected(bool connected)
{
    /// Description: Executes the blitzar_runtime_bridge_set_connected operation.
    blitzar_runtime_bridge_set_connected(_state, connected);
}
/// Description: Executes the isConnected operation.
bool RustRuntimeBridgeState::isConnected() const
{
    return blitzar_runtime_bridge_is_connected(_state);
}
/// Description: Executes the setServerLaunched operation.
void RustRuntimeBridgeState::setServerLaunched(bool launched)
{
    /// Description: Executes the blitzar_runtime_bridge_set_server_launched operation.
    blitzar_runtime_bridge_set_server_launched(_state, launched);
}
/// Description: Executes the serverLaunched operation.
bool RustRuntimeBridgeState::serverLaunched() const
{
    return blitzar_runtime_bridge_is_server_launched(_state);
}
/// Description: Executes the setRemoteSnapshotCap operation.
std::uint32_t RustRuntimeBridgeState::setRemoteSnapshotCap(std::uint32_t requested)
{
    return blitzar_runtime_bridge_set_snapshot_cap(_state, requested);
}
/// Description: Executes the remoteSnapshotCap operation.
std::uint32_t RustRuntimeBridgeState::remoteSnapshotCap() const
{
    return blitzar_runtime_bridge_snapshot_cap(_state);
}
/// Description: Executes the clearPendingCommands operation.
void RustRuntimeBridgeState::clearPendingCommands()
{
    /// Description: Executes the blitzar_runtime_bridge_clear_pending_commands operation.
    blitzar_runtime_bridge_clear_pending_commands(_state);
}
/// Description: Executes the queuePendingCommand operation.
bool RustRuntimeBridgeState::queuePendingCommand(const std::string& cmd, const std::string& fields)
{
    return blitzar_runtime_bridge_queue_pending_command(
        _state, reinterpret_cast<const std::uint8_t*>(cmd.data()), cmd.size(),
        reinterpret_cast<const std::uint8_t*>(fields.data()), fields.size());
}
/// Description: Executes the pendingCommandCount operation.
std::size_t RustRuntimeBridgeState::pendingCommandCount() const
{
    return blitzar_runtime_bridge_pending_command_count(_state);
}
std::pair<std::string, std::string>
/// Description: Executes the pendingCommandAt operation.
RustRuntimeBridgeState::pendingCommandAt(std::size_t index) const
{
    blitzar_runtime_pending_command_view view{};
    if (!blitzar_runtime_bridge_pending_command_view(_state, index, &view)) {
        return std::pair<std::string, std::string>();
    }
    return std::pair<std::string, std::string>(copyStringView(view.cmd),
                                               /// Description: Executes the copyStringView operation.
                                               copyStringView(view.fields));
}
/// Description: Executes the erasePendingPrefix operation.
void RustRuntimeBridgeState::erasePendingPrefix(std::size_t count)
{
    /// Description: Executes the blitzar_runtime_bridge_erase_pending_prefix operation.
    blitzar_runtime_bridge_erase_pending_prefix(_state, count);
}
/// Description: Executes the linkStateLabel operation.
std::string RustRuntimeBridgeState::linkStateLabel() const
{
    return copyStringView(blitzar_runtime_bridge_link_state_label(_state));
}
/// Description: Executes the serverOwnerLabel operation.
std::string RustRuntimeBridgeState::serverOwnerLabel() const
{
    return copyStringView(blitzar_runtime_bridge_server_owner_label(_state));
}
/// Description: Executes the copyStringView operation.
std::string RustRuntimeBridgeState::copyStringView(blitzar_runtime_string_view view)
{
    if (view.data == nullptr || view.len == 0u)
        return std::string();
    return std::string(reinterpret_cast<const char*>(view.data), view.len);
}
} // namespace grav_client
