#include <blitzar/blitzar.h>
#include <stddef.h>

int main(void)
{
    blitzar_context* context = NULL;
    blitzar_simulation* simulation = NULL;

    const blitzar_status context_status = blitzar_context_create(&context);
    const blitzar_status simulation_status =
        context_status == BLITZAR_STATUS_OK ? blitzar_simulation_create(context, 2, &simulation)
                                            : BLITZAR_STATUS_INTERNAL_ERROR;

    if (context_status != BLITZAR_STATUS_OK || simulation_status != BLITZAR_STATUS_OK) {
        blitzar_context_destroy(context);

        return 10;
    }

    const double position_x[] = {0.0, 1.0};
    const double position_y[] = {0.0, 0.0};
    const double position_z[] = {0.0, 0.0};
    const double velocity_x[] = {0.0, 0.0};
    const double velocity_y[] = {0.0, 0.0};
    const double velocity_z[] = {0.0, 0.0};
    const double mass[] = {1.0, 1.0};

    if (blitzar_simulation_set_gravity(simulation, 1.0, 0.0) != BLITZAR_STATUS_OK ||
        blitzar_simulation_set_timestep(simulation, 0.5) != BLITZAR_STATUS_OK) {
        blitzar_simulation_destroy(simulation);
        blitzar_context_destroy(context);

        return 20;
    }

    const blitzar_particle_input_v2 input = {sizeof(input), BLITZAR_ABI_VERSION_V2, 2, position_x,
        position_y, position_z, velocity_x, velocity_y, velocity_z, mass};

    const blitzar_barnes_hut_config_v2 configuration = {
        sizeof(configuration), BLITZAR_ABI_VERSION_V2, 0.5, 2, 128, 1, 32};

    const blitzar_status input_status = blitzar_simulation_set_particles_v2(simulation, &input);
    const blitzar_status configuration_status =
        blitzar_simulation_set_barnes_hut_v2(simulation, &configuration);

    const blitzar_status step_status = blitzar_simulation_step(simulation);

    if (input_status != BLITZAR_STATUS_OK || configuration_status != BLITZAR_STATUS_OK ||
        step_status != BLITZAR_STATUS_OK) {
        blitzar_simulation_destroy(simulation);
        blitzar_context_destroy(context);

        return 20;
    }

    double output_x[2] = {0.0, 0.0};
    double output_y[2] = {0.0, 0.0};
    double output_z[2] = {0.0, 0.0};
    double output_velocity_x[2] = {0.0, 0.0};
    double output_velocity_y[2] = {0.0, 0.0};
    double output_velocity_z[2] = {0.0, 0.0};
    double output_mass[2] = {0.0, 0.0};
    const blitzar_particle_output_v2 output = {sizeof(output), BLITZAR_ABI_VERSION_V2, 2, output_x,
        output_y, output_z, output_velocity_x, output_velocity_y, output_velocity_z, output_mass};

    const blitzar_status status = blitzar_simulation_get_state_v2(simulation, &output);

    blitzar_simulation_destroy(simulation);
    blitzar_context_destroy(context);

    return status == BLITZAR_STATUS_OK && output_mass[0] == 1.0 ? 0 : 1;
}
