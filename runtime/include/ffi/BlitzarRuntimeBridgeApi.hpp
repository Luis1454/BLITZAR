/*
 * @file runtime/include/ffi/BlitzarRuntimeBridgeApi.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime public interfaces for protocol, command, client, and FFI boundaries.
 */

#ifndef BLITZAR_RUNTIME_INCLUDE_FFI_BLITZARRUNTIMEBRIDGEAPI_HPP_
#define BLITZAR_RUNTIME_INCLUDE_FFI_BLITZARRUNTIMEBRIDGEAPI_HPP_
#include <cstddef>
#include <cstdint>
extern "C" {
struct blitzar_runtime_bridge_t;

struct blitzar_runtime_string_view {
    const std::uint8_t* data;
    std::size_t len;
};

struct blitzar_runtime_pending_command_view {
    blitzar_runtime_string_view cmd;
    blitzar_runtime_string_view fields;
};

blitzar_runtime_bridge_t* blitzar_runtime_bridge_create();
void blitzar_runtime_bridge_destroy(blitzar_runtime_bridge_t* state);
void blitzar_runtime_bridge_set_connected(blitzar_runtime_bridge_t* state, bool connected);
bool blitzar_runtime_bridge_is_connected(const blitzar_runtime_bridge_t* state);
void blitzar_runtime_bridge_set_server_launched(blitzar_runtime_bridge_t* state, bool launched);
bool blitzar_runtime_bridge_is_server_launched(const blitzar_runtime_bridge_t* state);
std::uint32_t blitzar_runtime_bridge_set_snapshot_cap(blitzar_runtime_bridge_t* state,
                                                      std::uint32_t requested);
std::uint32_t blitzar_runtime_bridge_snapshot_cap(const blitzar_runtime_bridge_t* state);
void blitzar_runtime_bridge_clear_pending_commands(blitzar_runtime_bridge_t* state);
bool blitzar_runtime_bridge_queue_pending_command(blitzar_runtime_bridge_t* state,
                                                  const std::uint8_t* cmdData, std::size_t cmdLen,
                                                  const std::uint8_t* fieldsData,
                                                  std::size_t fieldsLen);
std::size_t blitzar_runtime_bridge_pending_command_count(const blitzar_runtime_bridge_t* state);
bool blitzar_runtime_bridge_pending_command_view(const blitzar_runtime_bridge_t* state,
                                                 std::size_t index,
                                                 blitzar_runtime_pending_command_view* outView);
void blitzar_runtime_bridge_erase_pending_prefix(blitzar_runtime_bridge_t* state,
                                                 std::size_t count);
blitzar_runtime_string_view
blitzar_runtime_bridge_link_state_label(const blitzar_runtime_bridge_t* state);
blitzar_runtime_string_view
blitzar_runtime_bridge_server_owner_label(const blitzar_runtime_bridge_t* state);
}
#endif // BLITZAR_RUNTIME_INCLUDE_FFI_BLITZARRUNTIMEBRIDGEAPI_HPP_
