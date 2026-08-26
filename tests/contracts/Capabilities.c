#include "fixtures/Check.hpp"

#include <blitzar/blitzar.h>
#include <stddef.h>
#include <stdint.h>

_Static_assert(offsetof(blitzar_capabilities_v2, struct_size) == 0, "capability size is first");
_Static_assert(offsetof(blitzar_capabilities_v2, abi_version) == sizeof(uint32_t),
    "capability version follows size");

int main(void)
{
    blitzar_capabilities_v2 capabilities = {
        sizeof(capabilities), BLITZAR_ABI_VERSION_V2, 0, 0, 0, 0};

    BLITZAR_CHECK(blitzar_get_capabilities_v2(&capabilities) == BLITZAR_STATUS_OK);

    BLITZAR_CHECK((capabilities.implemented_solver_mask & BLITZAR_SOLVER_MASK_DIRECT) != 0);
    BLITZAR_CHECK((capabilities.implemented_solver_mask & BLITZAR_SOLVER_MASK_BARNES_HUT) != 0);
    BLITZAR_CHECK((capabilities.implemented_solver_mask & BLITZAR_SOLVER_MASK_FMM) != 0);
    BLITZAR_CHECK((capabilities.unsupported_solver_mask & BLITZAR_SOLVER_MASK_PM) != 0);
    BLITZAR_CHECK((capabilities.unsupported_solver_mask & BLITZAR_SOLVER_MASK_TREEPM) != 0);
    BLITZAR_CHECK((capabilities.deferred_feature_mask & BLITZAR_FEATURE_GRID) != 0);
    BLITZAR_CHECK((capabilities.deferred_feature_mask & BLITZAR_FEATURE_SNAPSHOT_PERSISTENCE) != 0);
    BLITZAR_CHECK((capabilities.compiled_backend_mask & BLITZAR_BACKEND_MASK_CPU) != 0);
    BLITZAR_CHECK(blitzar_get_capabilities_v2(NULL) == BLITZAR_STATUS_INVALID_ARGUMENT);

    blitzar_context* context = NULL;
    blitzar_simulation* simulation = NULL;

    BLITZAR_CHECK(blitzar_context_create(&context) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(blitzar_simulation_create(context, 2, &simulation) == BLITZAR_STATUS_OK);

    blitzar_context_destroy(context);

    const double position_x[] = {0.0, 1.0};
    const double position_y[] = {0.0, 0.0};
    const double position_z[] = {0.0, 0.0};
    const double velocity_x[] = {0.0, 0.0};
    const double velocity_y[] = {0.0, 0.0};
    const double velocity_z[] = {0.0, 0.0};
    const double mass[] = {1.0, 1.0};

    BLITZAR_CHECK(blitzar_simulation_set_particles(simulation, 2, position_x, position_y,
                      position_z, velocity_x, velocity_y, velocity_z, mass) == BLITZAR_STATUS_OK);

    BLITZAR_CHECK(
        blitzar_simulation_set_barnes_hut(simulation, 0.5, 2, 128, 1, 32) == BLITZAR_STATUS_OK);

    BLITZAR_CHECK(blitzar_simulation_set_gravity(simulation, 1.0, 0.1) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(blitzar_simulation_set_timestep(simulation, 0.01) == BLITZAR_STATUS_OK);

    BLITZAR_CHECK(
        blitzar_simulation_set_solver(simulation, BLITZAR_SOLVER_FMM) == BLITZAR_STATUS_OK);

    BLITZAR_CHECK(
        blitzar_simulation_set_solver(simulation, BLITZAR_SOLVER_PM) == BLITZAR_STATUS_UNSUPPORTED);

    BLITZAR_CHECK(blitzar_simulation_step(simulation) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(blitzar_simulation_set_solver(simulation, BLITZAR_SOLVER_TREEPM) ==
                  BLITZAR_STATUS_UNSUPPORTED);

    BLITZAR_CHECK(blitzar_simulation_step(simulation) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(blitzar_simulation_set_solver(simulation, (blitzar_solver_kind)99) ==
                  BLITZAR_STATUS_INVALID_ARGUMENT);

    BLITZAR_CHECK(blitzar_simulation_step(simulation) == BLITZAR_STATUS_OK);

    if ((capabilities.compiled_backend_mask & BLITZAR_BACKEND_MASK_HIP) == 0) {
        blitzar_backend_kind backend = BLITZAR_BACKEND_HIP;

        BLITZAR_CHECK(blitzar_simulation_backend(simulation, &backend) == BLITZAR_STATUS_OK);
        BLITZAR_CHECK(backend == BLITZAR_BACKEND_CPU);
    }

    blitzar_simulation_destroy(simulation);

    return 0;
}
