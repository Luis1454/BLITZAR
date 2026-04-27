/*
 * @file runtime/include/ffi/BlitzarCoreApi.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime public interfaces for protocol, command, client, and FFI boundaries.
 */

#ifndef GRAVITY_RUNTIME_INCLUDE_FFI_BLITZARCOREAPI_H_
#define GRAVITY_RUNTIME_INCLUDE_FFI_BLITZARCOREAPI_H_
#include <stddef.h>
#include <stdint.h>
extern "C" {
typedef struct blitzar_core blitzar_core_t;

typedef enum blitzar_core_result {
    BLITZAR_CORE_OK = 0,
    BLITZAR_CORE_INVALID_ARGUMENT = 1,
    BLITZAR_CORE_NOT_READY = 2,
    BLITZAR_CORE_TIMEOUT = 3,
    BLITZAR_CORE_INTERNAL_ERROR = 4
} blitzar_core_result_t;

enum {
    BLITZAR_CORE_TEXT_CAPACITY = 64,
    BLITZAR_CORE_ERROR_CAPACITY = 256
};

typedef struct blitzar_core_config {
    uint32_t particle_count;
    float dt;
    const char* solver_name;
    const char* integrator_name;
    const char* performance_profile;
    float substep_target_dt;
    uint32_t max_substeps;
    uint32_t snapshot_publish_period_ms;
} blitzar_core_config_t;

typedef struct blitzar_render_particle {
    float x;
    float y;
    float z;
    float mass;
    float pressure_norm;
    float temperature;
} blitzar_render_particle_t;

typedef struct blitzar_core_snapshot {
    blitzar_render_particle_t* particles;
    size_t count;
} blitzar_core_snapshot_t;

typedef struct blitzar_core_status {
    uint64_t steps;
    float dt;
    uint8_t paused;
    uint8_t faulted;
    uint64_t fault_step;
    uint8_t sph_enabled;
    float server_fps;
    float substep_target_dt;
    float substep_dt;
    uint32_t substeps;
    uint32_t max_substeps;
    uint32_t snapshot_publish_period_ms;
    uint32_t particle_count;
    float kinetic_energy;
    float potential_energy;
    float thermal_energy;
    float radiated_energy;
    float total_energy;
    float energy_drift_pct;
    uint8_t energy_estimated;
    char solver_name[BLITZAR_CORE_TEXT_CAPACITY];
    char integrator_name[BLITZAR_CORE_TEXT_CAPACITY];
    char performance_profile[BLITZAR_CORE_TEXT_CAPACITY];
    char fault_reason[BLITZAR_CORE_ERROR_CAPACITY];
} blitzar_core_status_t;

blitzar_core_config_t blitzar_core_default_config(void);
blitzar_core_t* blitzar_core_create(const blitzar_core_config_t* config, char* error_buffer,
                                    size_t error_buffer_capacity);
void blitzar_core_destroy(blitzar_core_t* core);
blitzar_core_result_t blitzar_core_apply_config(blitzar_core_t* core,
                                                const blitzar_core_config_t* config);
blitzar_core_result_t blitzar_core_run_steps(blitzar_core_t* core, uint32_t steps,
                                             uint32_t timeout_ms);
blitzar_core_result_t blitzar_core_get_status(const blitzar_core_t* core,
                                              blitzar_core_status_t* out_status);
blitzar_core_result_t blitzar_core_get_snapshot(const blitzar_core_t* core, size_t max_points,
                                                blitzar_core_snapshot_t* out_snapshot);
void blitzar_core_free_snapshot(blitzar_core_snapshot_t* snapshot);
blitzar_core_result_t blitzar_core_load_state(blitzar_core_t* core, const char* path,
                                              const char* format, uint32_t timeout_ms);
blitzar_core_result_t blitzar_core_export_state(blitzar_core_t* core, const char* path,
                                                const char* format, uint32_t timeout_ms);
size_t blitzar_core_get_last_error(const blitzar_core_t* core, char* error_buffer,
                                   size_t error_buffer_capacity);
}
#endif // GRAVITY_RUNTIME_INCLUDE_FFI_BLITZARCOREAPI_H_
