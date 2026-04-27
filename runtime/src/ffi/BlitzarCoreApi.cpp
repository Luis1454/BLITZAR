// File: runtime/src/ffi/BlitzarCoreApi.cpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#include "ffi/BlitzarCoreApi.hpp"
#include "runtime/src/ffi/BlitzarCoreInternal.hpp"
#include <algorithm>
#include <cstring>
#include <new>

namespace grav_ffi_internal {
/// Description: Executes the writeErrorMessage operation.
static void writeErrorMessage(const char* message, char* buffer, std::size_t capacity)
{
    if (buffer == nullptr || capacity == 0u) {
        return;
    }
    if (message == nullptr) {
        buffer[0] = '\0';
        return;
    }
    const std::size_t length = std::min<std::size_t>(capacity - 1u, std::strlen(message));
    std::memcpy(buffer, message, length);
    buffer[length] = '\0';
}

/// Description: Executes the invalidArgumentIfNull operation.
static blitzar_core_result_t invalidArgumentIfNull(const void* value)
{
    return value == nullptr ? BLITZAR_CORE_INVALID_ARGUMENT : BLITZAR_CORE_OK;
}
} // namespace grav_ffi_internal

/// Description: Executes the blitzar_core operation.
blitzar_core::blitzar_core(const blitzar_core_config_t& config) : impl(config)
{
}

extern "C" {
/// Description: Executes the blitzar_core_default_config operation.
blitzar_core_config_t blitzar_core_default_config(void)
{
    return blitzar_core_config_t{10000u, 0.01f, "pairwise_cuda", "euler", "interactive", 0.01f,
                                 4u,     50u};
}

/// Description: Describes the blitzar core create operation contract.
blitzar_core_t* blitzar_core_create(const blitzar_core_config_t* config, char* error_buffer,
                                    size_t error_buffer_capacity)
{
    if (config == nullptr) {
        grav_ffi_internal::writeErrorMessage("config is required", error_buffer,
                                             error_buffer_capacity);
        return nullptr;
    }
    try {
        blitzar_core_t* core = new blitzar_core(*config);
        if (core->impl.copyLastError(error_buffer, error_buffer_capacity) != 0u) {
            delete core;
            return nullptr;
        }
        grav_ffi_internal::writeErrorMessage("", error_buffer, error_buffer_capacity);
        return core;
    }
    catch (const std::bad_alloc&) {
        grav_ffi_internal::writeErrorMessage("allocation failed", error_buffer,
                                             error_buffer_capacity);
        return nullptr;
    }
    catch (...) {
        grav_ffi_internal::writeErrorMessage("unexpected exception during core creation",
                                             error_buffer, error_buffer_capacity);
        return nullptr;
    }
}

/// Description: Executes the blitzar_core_destroy operation.
void blitzar_core_destroy(blitzar_core_t* core)
{
    delete core;
}

/// Description: Describes the blitzar core apply config operation contract.
blitzar_core_result_t blitzar_core_apply_config(blitzar_core_t* core,
                                                const blitzar_core_config_t* config)
{
    if (grav_ffi_internal::invalidArgumentIfNull(core) != BLITZAR_CORE_OK ||
        grav_ffi_internal::invalidArgumentIfNull(config) != BLITZAR_CORE_OK) {
        return BLITZAR_CORE_INVALID_ARGUMENT;
    }
    return core->impl.applyConfig(*config);
}

/// Description: Describes the blitzar core run steps operation contract.
blitzar_core_result_t blitzar_core_run_steps(blitzar_core_t* core, uint32_t steps,
                                             uint32_t timeout_ms)
{
    if (grav_ffi_internal::invalidArgumentIfNull(core) != BLITZAR_CORE_OK) {
        return BLITZAR_CORE_INVALID_ARGUMENT;
    }
    return core->impl.runSteps(steps, timeout_ms);
}

/// Description: Describes the blitzar core get status operation contract.
blitzar_core_result_t blitzar_core_get_status(const blitzar_core_t* core,
                                              blitzar_core_status_t* out_status)
{
    if (grav_ffi_internal::invalidArgumentIfNull(core) != BLITZAR_CORE_OK ||
        grav_ffi_internal::invalidArgumentIfNull(out_status) != BLITZAR_CORE_OK) {
        return BLITZAR_CORE_INVALID_ARGUMENT;
    }
    return core->impl.getStatus(*out_status);
}

/// Description: Describes the blitzar core get snapshot operation contract.
blitzar_core_result_t blitzar_core_get_snapshot(const blitzar_core_t* core, size_t max_points,
                                                blitzar_core_snapshot_t* out_snapshot)
{
    if (grav_ffi_internal::invalidArgumentIfNull(core) != BLITZAR_CORE_OK ||
        grav_ffi_internal::invalidArgumentIfNull(out_snapshot) != BLITZAR_CORE_OK) {
        return BLITZAR_CORE_INVALID_ARGUMENT;
    }
    out_snapshot->particles = nullptr;
    out_snapshot->count = 0u;
    return core->impl.getSnapshot(max_points, *out_snapshot);
}

/// Description: Executes the blitzar_core_free_snapshot operation.
void blitzar_core_free_snapshot(blitzar_core_snapshot_t* snapshot)
{
    if (snapshot == nullptr) {
        return;
    }
    delete[] snapshot->particles;
    snapshot->particles = nullptr;
    snapshot->count = 0u;
}

/// Description: Describes the blitzar core load state operation contract.
blitzar_core_result_t blitzar_core_load_state(blitzar_core_t* core, const char* path,
                                              const char* format, uint32_t timeout_ms)
{
    if (grav_ffi_internal::invalidArgumentIfNull(core) != BLITZAR_CORE_OK) {
        return BLITZAR_CORE_INVALID_ARGUMENT;
    }
    return core->impl.loadState(path, format, timeout_ms);
}

/// Description: Describes the blitzar core export state operation contract.
blitzar_core_result_t blitzar_core_export_state(blitzar_core_t* core, const char* path,
                                                const char* format, uint32_t timeout_ms)
{
    if (grav_ffi_internal::invalidArgumentIfNull(core) != BLITZAR_CORE_OK) {
        return BLITZAR_CORE_INVALID_ARGUMENT;
    }
    return core->impl.exportState(path, format, timeout_ms);
}

/// Description: Describes the blitzar core get last error operation contract.
size_t blitzar_core_get_last_error(const blitzar_core_t* core, char* error_buffer,
                                   size_t error_buffer_capacity)
{
    if (core == nullptr) {
        grav_ffi_internal::writeErrorMessage("core is required", error_buffer,
                                             error_buffer_capacity);
        return std::strlen("core is required");
    }
    return core->impl.copyLastError(error_buffer, error_buffer_capacity);
}
} // extern "C"
