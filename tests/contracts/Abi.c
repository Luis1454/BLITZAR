#include "fixtures/Check.hpp"

#include <blitzar/blitzar.h>
#include <stddef.h>
#include <stdint.h>

_Static_assert(sizeof(blitzar_status) == sizeof(int32_t), "status ABI must be 32-bit");
_Static_assert(sizeof(blitzar_solver_kind) == sizeof(int32_t), "solver ABI must be 32-bit");
_Static_assert(sizeof(blitzar_backend_kind) == sizeof(int32_t), "backend ABI must be 32-bit");
_Static_assert(offsetof(blitzar_particle_input_v2, struct_size) == 0, "V2 size is first");
_Static_assert(offsetof(blitzar_particle_input_v2, abi_version) == sizeof(uint32_t),
    "V2 version follows size");
_Static_assert(offsetof(blitzar_particle_output_v2, struct_size) == 0, "output size is first");
_Static_assert(
    offsetof(blitzar_barnes_hut_config_v2, struct_size) == 0, "configuration size is first");

int main(void)
{
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

    BLITZAR_CHECK(
        blitzar_simulation_set_solver(simulation, BLITZAR_SOLVER_DIRECT) == BLITZAR_STATUS_OK);

    BLITZAR_CHECK(blitzar_simulation_set_integrator(simulation, BLITZAR_INTEGRATOR_LEAPFROG_KDK) ==
                  BLITZAR_STATUS_OK);

    BLITZAR_CHECK(blitzar_simulation_set_gravity(simulation, 1.0, 0.0) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(blitzar_simulation_set_timestep(simulation, 0.5) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(blitzar_simulation_set_particles(simulation, 2, position_x, position_y,
                      position_z, velocity_x, velocity_y, velocity_z, mass) == BLITZAR_STATUS_OK);

    blitzar_particle_input_v2 input = {sizeof(input), BLITZAR_ABI_VERSION_V2, 2, position_x,
        position_y, position_z, velocity_x, velocity_y, velocity_z, mass};

    BLITZAR_CHECK(blitzar_simulation_set_particles_v2(simulation, &input) == BLITZAR_STATUS_OK);

    blitzar_barnes_hut_config_v2 configuration = {
        sizeof(configuration), BLITZAR_ABI_VERSION_V2, 0.5, 2, 128, 1, 32};

    BLITZAR_CHECK(
        blitzar_simulation_set_barnes_hut_v2(simulation, &configuration) == BLITZAR_STATUS_OK);

    BLITZAR_CHECK(
        blitzar_simulation_set_solver(simulation, BLITZAR_SOLVER_DIRECT) == BLITZAR_STATUS_OK);

    double output_x[2] = {0.0, 0.0};
    double output_y[2] = {0.0, 0.0};
    double output_z[2] = {0.0, 0.0};
    double output_velocity_x[2] = {0.0, 0.0};
    double output_velocity_y[2] = {0.0, 0.0};
    double output_velocity_z[2] = {0.0, 0.0};
    double output_mass[2] = {0.0, 0.0};

    blitzar_particle_output_v2 output = {sizeof(output), BLITZAR_ABI_VERSION_V2, 2, output_x,
        output_y, output_z, output_velocity_x, output_velocity_y, output_velocity_z, output_mass};

    BLITZAR_CHECK(blitzar_simulation_step(simulation) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(
        blitzar_simulation_get_state(simulation, 2, output_x, output_y, output_z, output_velocity_x,
            output_velocity_y, output_velocity_z, output_mass) == BLITZAR_STATUS_OK);

    BLITZAR_CHECK(blitzar_simulation_get_state_v2(simulation, &output) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(output_mass[0] == 1.0);

    input.abi_version = 1;

    BLITZAR_CHECK(
        blitzar_simulation_set_particles_v2(simulation, &input) == BLITZAR_STATUS_INVALID_ARGUMENT);

    configuration.struct_size = (uint32_t)(sizeof(configuration) - 1U);

    BLITZAR_CHECK(blitzar_simulation_set_barnes_hut_v2(simulation, &configuration) ==
                  BLITZAR_STATUS_INVALID_ARGUMENT);

    blitzar_simulation_destroy(simulation);

    return 0;
}
