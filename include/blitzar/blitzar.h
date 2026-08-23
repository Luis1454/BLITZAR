#ifndef BLITZAR_BLITZAR_H
#define BLITZAR_BLITZAR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) && defined(BLITZAR_SHARED)
#if defined(BLITZAR_BUILDING)
#define BLITZAR_API __declspec(dllexport)
#else
#define BLITZAR_API __declspec(dllimport)
#endif
#elif defined(__GNUC__) || defined(__clang__)
#define BLITZAR_API __attribute__((visibility("default")))
#else
#define BLITZAR_API
#endif

typedef struct blitzar_context blitzar_context;
typedef struct blitzar_simulation blitzar_simulation;

/*
 * A context is a creation capability, not a borrowed simulation owner.
 * A successful simulation creation copies the required runtime ownership, so
 * the context may be destroyed immediately afterwards. Operations on one
 * live simulation are non-reentrant: a concurrent call is rejected with
 * BLITZAR_STATUS_INTERNAL_ERROR. Destruction of a context or simulation must
 * not race with another operation on the same handle.
 */

typedef int32_t blitzar_status;

#define BLITZAR_STATUS_OK ((blitzar_status)0)
#define BLITZAR_STATUS_INVALID_ARGUMENT ((blitzar_status)1)
#define BLITZAR_STATUS_ALLOCATION_FAILURE ((blitzar_status)2)
#define BLITZAR_STATUS_INTERNAL_ERROR ((blitzar_status)3)
#define BLITZAR_STATUS_SINGULARITY ((blitzar_status)4)
#define BLITZAR_STATUS_UNSUPPORTED ((blitzar_status)5)

typedef int32_t blitzar_solver_kind;

#define BLITZAR_SOLVER_DIRECT ((blitzar_solver_kind)0)
#define BLITZAR_SOLVER_BARNES_HUT ((blitzar_solver_kind)1)
#define BLITZAR_SOLVER_FMM ((blitzar_solver_kind)2)
#define BLITZAR_SOLVER_PM ((blitzar_solver_kind)3)
#define BLITZAR_SOLVER_TREEPM ((blitzar_solver_kind)4)

typedef int32_t blitzar_backend_kind;

#define BLITZAR_BACKEND_CPU ((blitzar_backend_kind)0)
#define BLITZAR_BACKEND_HIP ((blitzar_backend_kind)1)

typedef int32_t blitzar_integrator_kind;

#define BLITZAR_INTEGRATOR_LEAPFROG_KDK ((blitzar_integrator_kind)0)

/* Product/API semantic version and the frozen implementation plan revision. */
BLITZAR_API const char* blitzar_version(void);

BLITZAR_API const char* blitzar_plan_version(void);

BLITZAR_API blitzar_status blitzar_context_create(blitzar_context** context);

BLITZAR_API void blitzar_context_destroy(blitzar_context* context);

BLITZAR_API blitzar_status blitzar_context_status(const blitzar_context* context);

BLITZAR_API const char* blitzar_status_message(blitzar_status status);

BLITZAR_API blitzar_status blitzar_simulation_create(
    blitzar_context* context, int64_t particle_count, blitzar_simulation** simulation);

BLITZAR_API void blitzar_simulation_destroy(blitzar_simulation* simulation);

BLITZAR_API blitzar_status blitzar_simulation_status(const blitzar_simulation* simulation);

BLITZAR_API blitzar_status blitzar_simulation_backend(
    const blitzar_simulation* simulation, blitzar_backend_kind* backend);

BLITZAR_API blitzar_status blitzar_simulation_particle_count(
    const blitzar_simulation* simulation, int64_t* particle_count);

BLITZAR_API blitzar_status blitzar_simulation_set_solver(
    blitzar_simulation* simulation, blitzar_solver_kind solver);

BLITZAR_API blitzar_status blitzar_simulation_set_integrator(
    blitzar_simulation* simulation, blitzar_integrator_kind integrator);

BLITZAR_API blitzar_status blitzar_simulation_set_gravity(
    blitzar_simulation* simulation, double gravitational_constant, double softening);

BLITZAR_API blitzar_status blitzar_simulation_set_units(
    blitzar_simulation* simulation, double length_scale, double mass_scale, double time_scale);

BLITZAR_API blitzar_status blitzar_simulation_set_barnes_hut(blitzar_simulation* simulation,
    double opening_angle, int64_t max_particles, int64_t max_cells, int64_t leaf_capacity,
    int64_t max_depth);

BLITZAR_API blitzar_status blitzar_simulation_set_timestep(
    blitzar_simulation* simulation, double timestep);

BLITZAR_API blitzar_status blitzar_simulation_set_seed(
    blitzar_simulation* simulation, uint64_t seed);

BLITZAR_API blitzar_status blitzar_simulation_set_particles(blitzar_simulation* simulation,
    int64_t particle_count, const double* position_x, const double* position_y,
    const double* position_z, const double* velocity_x, const double* velocity_y,
    const double* velocity_z, const double* mass);

BLITZAR_API blitzar_status blitzar_simulation_get_state(const blitzar_simulation* simulation,
    int64_t capacity, double* position_x, double* position_y, double* position_z,
    double* velocity_x, double* velocity_y, double* velocity_z, double* mass);

BLITZAR_API blitzar_status blitzar_simulation_step(blitzar_simulation* simulation);

#ifdef __cplusplus
}
#endif

#endif
