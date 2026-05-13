/*
 * @file runtime/src/client/runtime/BridgeState.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime implementation for protocol, command, client, and FFI boundaries.
 */

#include "client/runtime/BridgeState.hpp"
#include <stdexcept>

namespace bltzr_client {
BridgeState::BridgeState() : _state(blitzar_runtime_bridge_create())
{
    if (_state == nullptr) {
        throw std::runtime_error("failed to create Rust runtime bridge state");
    }
}

BridgeState::~BridgeState()
{
    blitzar_runtime_bridge_destroy(_state);
}

void BridgeState::setConnected(bool connected)
{
    blitzar_runtime_bridge_set_connected(_state, connected);
}

bool BridgeState::isConnected() const
{
    return blitzar_runtime_bridge_is_connected(_state);
}

void BridgeState::setServerLaunched(bool launched)
{
    blitzar_runtime_bridge_set_server_launched(_state, launched);
}

bool BridgeState::serverLaunched() const
{
    return blitzar_runtime_bridge_is_server_launched(_state);
}

std::uint32_t BridgeState::setRemoteSnapshotCap(std::uint32_t requested)
{
    return blitzar_runtime_bridge_set_snapshot_cap(_state, requested);
}

std::uint32_t BridgeState::remoteSnapshotCap() const
{
    return blitzar_runtime_bridge_snapshot_cap(_state);
}

void BridgeState::clearPendingCommands()
{
    blitzar_runtime_bridge_clear_pending_commands(_state);
}

bool BridgeState::queuePendingCommand(const std::string& cmd, const std::string& fields)
{
    return blitzar_runtime_bridge_queue_pending_command(
        _state, reinterpret_cast<const std::uint8_t*>(cmd.data()), cmd.size(),
        reinterpret_cast<const std::uint8_t*>(fields.data()), fields.size());
}

std::size_t BridgeState::pendingCommandCount() const
{
    return blitzar_runtime_bridge_pending_command_count(_state);
}

std::pair<std::string, std::string>
BridgeState::pendingCommandAt(std::size_t index) const
{
    blitzar_runtime_pending_command_view view{};
    if (!blitzar_runtime_bridge_pending_command_view(_state, index, &view)) {
        return std::pair<std::string, std::string>();
    }
    return std::pair<std::string, std::string>(copyStringView(view.cmd),
                                               copyStringView(view.fields));
}

void BridgeState::erasePendingPrefix(std::size_t count)
{
    blitzar_runtime_bridge_erase_pending_prefix(_state, count);
}

std::string BridgeState::linkStateLabel() const
{
    return copyStringView(blitzar_runtime_bridge_link_state_label(_state));
}

std::string BridgeState::serverOwnerLabel() const
{
    return copyStringView(blitzar_runtime_bridge_server_owner_label(_state));
}

std::string BridgeState::copyStringView(blitzar_runtime_string_view view)
{
    if (view.data == nullptr || view.len == 0u)
        return std::string();
    return std::string(reinterpret_cast<const char*>(view.data), view.len);
}
} // namespace bltzr_client
