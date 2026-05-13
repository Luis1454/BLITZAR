/*
 * @file runtime/src/ffi/bridge/Api.cpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief C++ fallback implementation of the client runtime bridge ABI.
 */

#include "ffi/bridge/Api.hpp"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

struct blitzar_runtime_bridge_t {
    bool connected = false;
    bool serverLaunched = false;
    std::uint32_t snapshotCap = 8192u;
    std::vector<std::pair<std::string, std::string>> pendingCommands;
    std::string linkStateLabel = "reconnecting";
    std::string serverOwnerLabel = "external";
};

namespace {
constexpr std::size_t kPendingCommandLimit = 128u;
constexpr std::uint32_t kSnapshotCapMin = 1u;
constexpr std::uint32_t kSnapshotCapMax = 1'000'000u;

blitzar_runtime_string_view stringView(const std::string& value)
{
    return blitzar_runtime_string_view{
        reinterpret_cast<const std::uint8_t*>(value.data()),
        value.size(),
    };
}
} // namespace

extern "C" {
blitzar_runtime_bridge_t* blitzar_runtime_bridge_create()
{
    return new blitzar_runtime_bridge_t();
}

void blitzar_runtime_bridge_destroy(blitzar_runtime_bridge_t* state)
{
    delete state;
}

void blitzar_runtime_bridge_set_connected(blitzar_runtime_bridge_t* state, bool connected)
{
    if (state == nullptr)
        return;
    state->connected = connected;
    state->linkStateLabel = connected ? "connected" : "reconnecting";
}

bool blitzar_runtime_bridge_is_connected(const blitzar_runtime_bridge_t* state)
{
    return state != nullptr && state->connected;
}

void blitzar_runtime_bridge_set_server_launched(blitzar_runtime_bridge_t* state, bool launched)
{
    if (state == nullptr)
        return;
    state->serverLaunched = launched;
    state->serverOwnerLabel = launched ? "client" : "external";
}

bool blitzar_runtime_bridge_is_server_launched(const blitzar_runtime_bridge_t* state)
{
    return state != nullptr && state->serverLaunched;
}

std::uint32_t blitzar_runtime_bridge_set_snapshot_cap(blitzar_runtime_bridge_t* state,
                                                      std::uint32_t requested)
{
    if (state == nullptr)
        return 0u;
    state->snapshotCap = std::clamp(requested, kSnapshotCapMin, kSnapshotCapMax);
    return state->snapshotCap;
}

std::uint32_t blitzar_runtime_bridge_snapshot_cap(const blitzar_runtime_bridge_t* state)
{
    return state == nullptr ? 0u : state->snapshotCap;
}

void blitzar_runtime_bridge_clear_pending_commands(blitzar_runtime_bridge_t* state)
{
    if (state != nullptr) {
        state->pendingCommands.clear();
    }
}

bool blitzar_runtime_bridge_queue_pending_command(blitzar_runtime_bridge_t* state,
                                                  const std::uint8_t* cmdData, std::size_t cmdLen,
                                                  const std::uint8_t* fieldsData,
                                                  std::size_t fieldsLen)
{
    if (state == nullptr || cmdData == nullptr)
        return false;
    bool droppedOldest = false;
    if (state->pendingCommands.size() >= kPendingCommandLimit) {
        state->pendingCommands.erase(state->pendingCommands.begin());
        droppedOldest = true;
    }
    const std::string cmd(reinterpret_cast<const char*>(cmdData), cmdLen);
    const std::string fields(fieldsData == nullptr ? "" : reinterpret_cast<const char*>(fieldsData),
                             fieldsData == nullptr ? 0u : fieldsLen);
    state->pendingCommands.emplace_back(cmd, fields);
    return droppedOldest;
}

std::size_t blitzar_runtime_bridge_pending_command_count(const blitzar_runtime_bridge_t* state)
{
    return state == nullptr ? 0u : state->pendingCommands.size();
}

bool blitzar_runtime_bridge_pending_command_view(const blitzar_runtime_bridge_t* state,
                                                 std::size_t index,
                                                 blitzar_runtime_pending_command_view* outView)
{
    if (state == nullptr || outView == nullptr || index >= state->pendingCommands.size())
        return false;
    const auto& command = state->pendingCommands[index];
    outView->cmd = stringView(command.first);
    outView->fields = stringView(command.second);
    return true;
}

void blitzar_runtime_bridge_erase_pending_prefix(blitzar_runtime_bridge_t* state,
                                                 std::size_t count)
{
    if (state == nullptr || count == 0u)
        return;
    const std::size_t erased = std::min(count, state->pendingCommands.size());
    state->pendingCommands.erase(state->pendingCommands.begin(),
                                 state->pendingCommands.begin() +
                                     static_cast<std::ptrdiff_t>(erased));
}

blitzar_runtime_string_view
blitzar_runtime_bridge_link_state_label(const blitzar_runtime_bridge_t* state)
{
    static const std::string kReconnecting = "reconnecting";
    return stringView(state == nullptr ? kReconnecting : state->linkStateLabel);
}

blitzar_runtime_string_view
blitzar_runtime_bridge_server_owner_label(const blitzar_runtime_bridge_t* state)
{
    static const std::string kExternal = "external";
    return stringView(state == nullptr ? kExternal : state->serverOwnerLabel);
}
}
