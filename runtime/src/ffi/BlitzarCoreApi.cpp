/*
 * @file runtime/src/ffi/BlitzarCoreApi.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime implementation for protocol, command, client, and FFI boundaries.
 */

#include "ffi/BlitzarCoreApi.hpp"
#include "runtime/src/ffi/BlitzarCoreInternal.hpp"
#include <algorithm>
#include <cstring>
#include <new>

namespace bltzr_ffi_internal {
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

static blitzar_core_result_t invalidArgumentIfNull(const void* value)
{
    return value == nullptr ? BLITZAR_CORE_INVALID_ARGUMENT : BLITZAR_CORE_OK;
}
} // namespace bltzr_ffi_internal

/*
 * @brief Documents the blitzar core operation contract.
 * @param config Input value used by this contract.
 * @return blitzar_core:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
blitzar_core::blitzar_core(const blitzar_core_config_t& config) : impl(config)
{
}

extern "C" {
blitzar_core_config_t blitzar_core_default_config(void)
{
    return blitzar_core_config_t{10000u, 0.01f, "pairwise_cuda", "euler", "interactive", 0.01f,
                                 4u,     50u};
}

blitzar_core_t* blitzar_core_create(const blitzar_core_config_t* config, char* error_buffer,
                                    size_t error_buffer_capacity)
{
    if (config == nullptr) {
        bltzr_ffi_internal::writeErrorMessage("config is required", error_buffer,
                                              error_buffer_capacity);
        return nullptr;
    }
    try {
        blitzar_core_t* core = new blitzar_core(*config);
        if (core->impl.copyLastError(error_buffer, error_buffer_capacity) != 0u) {
            delete core;
            return nullptr;
        }
        bltzr_ffi_internal::writeErrorMessage("", error_buffer, error_buffer_capacity);
        return core;
    }
    catch (const std::bad_alloc&) {
        bltzr_ffi_internal::writeErrorMessage("allocation failed", error_buffer,
                                              error_buffer_capacity);
        return nullptr;
    }
    catch (...) {
        bltzr_ffi_internal::writeErrorMessage("unexpected exception during core creation",
                                              error_buffer, error_buffer_capacity);
        return nullptr;
    }
}

void blitzar_core_destroy(blitzar_core_t* core)
{
    delete core;
}

blitzar_core_result_t blitzar_core_apply_config(blitzar_core_t* core,
                                                const blitzar_core_config_t* config)
{
    if (bltzr_ffi_internal::invalidArgumentIfNull(core) != BLITZAR_CORE_OK ||
        bltzr_ffi_internal::invalidArgumentIfNull(config) != BLITZAR_CORE_OK) {
        return BLITZAR_CORE_INVALID_ARGUMENT;
    }
    return core->impl.applyConfig(*config);
}

blitzar_core_result_t blitzar_core_run_steps(blitzar_core_t* core, uint32_t steps,
                                             uint32_t timeout_ms)
{
    if (bltzr_ffi_internal::invalidArgumentIfNull(core) != BLITZAR_CORE_OK) {
        return BLITZAR_CORE_INVALID_ARGUMENT;
    }
    return core->impl.runSteps(steps, timeout_ms);
}

blitzar_core_result_t blitzar_core_get_status(const blitzar_core_t* core,
                                              blitzar_core_status_t* out_status)
{
    if (bltzr_ffi_internal::invalidArgumentIfNull(core) != BLITZAR_CORE_OK ||
        bltzr_ffi_internal::invalidArgumentIfNull(out_status) != BLITZAR_CORE_OK) {
        return BLITZAR_CORE_INVALID_ARGUMENT;
    }
    return core->impl.getStatus(*out_status);
}

blitzar_core_result_t blitzar_core_get_snapshot(const blitzar_core_t* core, size_t max_points,
                                                blitzar_core_snapshot_t* out_snapshot)
{
    if (bltzr_ffi_internal::invalidArgumentIfNull(core) != BLITZAR_CORE_OK ||
        bltzr_ffi_internal::invalidArgumentIfNull(out_snapshot) != BLITZAR_CORE_OK) {
        return BLITZAR_CORE_INVALID_ARGUMENT;
    }
    out_snapshot->particles = nullptr;
    out_snapshot->count = 0u;
    return core->impl.getSnapshot(max_points, *out_snapshot);
}

void blitzar_core_free_snapshot(blitzar_core_snapshot_t* snapshot)
{
    if (snapshot == nullptr) {
        return;
    }
    delete[] snapshot->particles;
    snapshot->particles = nullptr;
    snapshot->count = 0u;
}

blitzar_core_result_t blitzar_core_load_state(blitzar_core_t* core, const char* path,
                                              const char* format, uint32_t timeout_ms)
{
    if (bltzr_ffi_internal::invalidArgumentIfNull(core) != BLITZAR_CORE_OK) {
        return BLITZAR_CORE_INVALID_ARGUMENT;
    }
    return core->impl.loadState(path, format, timeout_ms);
}

blitzar_core_result_t blitzar_core_export_state(blitzar_core_t* core, const char* path,
                                                const char* format, uint32_t timeout_ms)
{
    if (bltzr_ffi_internal::invalidArgumentIfNull(core) != BLITZAR_CORE_OK) {
        return BLITZAR_CORE_INVALID_ARGUMENT;
    }
    return core->impl.exportState(path, format, timeout_ms);
}

size_t blitzar_core_get_last_error(const blitzar_core_t* core, char* error_buffer,
                                   size_t error_buffer_capacity)
{
    if (core == nullptr) {
        bltzr_ffi_internal::writeErrorMessage("core is required", error_buffer,
                                              error_buffer_capacity);
        return std::strlen("core is required");
    }
    return core->impl.copyLastError(error_buffer, error_buffer_capacity);
}
} // extern "C"
