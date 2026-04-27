// File: runtime/include/ffi/BlitzarRuntimeBridgeApi.hpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#ifndef GRAVITY_RUNTIME_INCLUDE_FFI_BLITZARRUNTIMEBRIDGEAPI_HPP_
#define GRAVITY_RUNTIME_INCLUDE_FFI_BLITZARRUNTIMEBRIDGEAPI_HPP_
#include <cstddef>
#include <cstdint>
extern "C" {
/// Description: Defines the blitzar_runtime_bridge_t data or behavior contract.
struct blitzar_runtime_bridge_t;

/// Description: Defines the blitzar_runtime_string_view data or behavior contract.
struct blitzar_runtime_string_view {
    const std::uint8_t* data;
    std::size_t len;
};

/// Description: Defines the blitzar_runtime_pending_command_view data or behavior contract.
struct blitzar_runtime_pending_command_view {
    blitzar_runtime_string_view cmd;
    blitzar_runtime_string_view fields;
};

/// Description: Executes the blitzar_runtime_bridge_create operation.
blitzar_runtime_bridge_t* blitzar_runtime_bridge_create();
/// Description: Executes the blitzar_runtime_bridge_destroy operation.
void blitzar_runtime_bridge_destroy(blitzar_runtime_bridge_t* state);
/// Description: Executes the blitzar_runtime_bridge_set_connected operation.
void blitzar_runtime_bridge_set_connected(blitzar_runtime_bridge_t* state, bool connected);
/// Description: Executes the blitzar_runtime_bridge_is_connected operation.
bool blitzar_runtime_bridge_is_connected(const blitzar_runtime_bridge_t* state);
/// Description: Executes the blitzar_runtime_bridge_set_server_launched operation.
void blitzar_runtime_bridge_set_server_launched(blitzar_runtime_bridge_t* state, bool launched);
/// Description: Executes the blitzar_runtime_bridge_is_server_launched operation.
bool blitzar_runtime_bridge_is_server_launched(const blitzar_runtime_bridge_t* state);
/// Description: Describes the blitzar runtime bridge set snapshot cap operation contract.
std::uint32_t blitzar_runtime_bridge_set_snapshot_cap(blitzar_runtime_bridge_t* state,
                                                      std::uint32_t requested);
/// Description: Executes the blitzar_runtime_bridge_snapshot_cap operation.
std::uint32_t blitzar_runtime_bridge_snapshot_cap(const blitzar_runtime_bridge_t* state);
/// Description: Executes the blitzar_runtime_bridge_clear_pending_commands operation.
void blitzar_runtime_bridge_clear_pending_commands(blitzar_runtime_bridge_t* state);
/// Description: Describes the blitzar runtime bridge queue pending command operation contract.
bool blitzar_runtime_bridge_queue_pending_command(blitzar_runtime_bridge_t* state,
                                                  const std::uint8_t* cmdData, std::size_t cmdLen,
                                                  const std::uint8_t* fieldsData,
                                                  std::size_t fieldsLen);
/// Description: Executes the blitzar_runtime_bridge_pending_command_count operation.
std::size_t blitzar_runtime_bridge_pending_command_count(const blitzar_runtime_bridge_t* state);
/// Description: Describes the blitzar runtime bridge pending command view operation contract.
bool blitzar_runtime_bridge_pending_command_view(const blitzar_runtime_bridge_t* state,
                                                 std::size_t index,
                                                 blitzar_runtime_pending_command_view* outView);
/// Description: Describes the blitzar runtime bridge erase pending prefix operation contract.
void blitzar_runtime_bridge_erase_pending_prefix(blitzar_runtime_bridge_t* state,
                                                 std::size_t count);
blitzar_runtime_string_view
/// Description: Describes the blitzar runtime bridge link state label operation contract.
blitzar_runtime_bridge_link_state_label(const blitzar_runtime_bridge_t* state);
blitzar_runtime_string_view
/// Description: Describes the blitzar runtime bridge server owner label operation contract.
blitzar_runtime_bridge_server_owner_label(const blitzar_runtime_bridge_t* state);
}
#endif // GRAVITY_RUNTIME_INCLUDE_FFI_BLITZARRUNTIMEBRIDGEAPI_HPP_
