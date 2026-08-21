#include <blitzar/blitzar.h>

int main(void)
{
    blitzar_context* context = NULL;
    const blitzar_status status = blitzar_context_create(&context);
    if (status != BLITZAR_STATUS_OK) {
        return (int)status;
    }

    blitzar_simulation* simulation = NULL;
    if (blitzar_simulation_create(context, 2, &simulation) !=
        BLITZAR_STATUS_OK) {
        blitzar_context_destroy(context);
        return 1;
    }
    const double position_x[] = {0.0, 1.0};
    const double position_y[] = {0.0, 0.0};
    const double position_z[] = {0.0, 0.0};
    const double velocity_x[] = {0.0, 0.0};
    const double velocity_y[] = {0.0, 0.0};
    const double velocity_z[] = {0.0, 0.0};
    const double mass[] = {1.0, 1.0};
    if (blitzar_simulation_set_particles(
            simulation,
            2,
            position_x,
            position_y,
            position_z,
            velocity_x,
            velocity_y,
            velocity_z,
            mass) != BLITZAR_STATUS_OK ||
        blitzar_simulation_set_timestep(simulation, 0.5) != BLITZAR_STATUS_OK ||
        blitzar_simulation_step(simulation) != BLITZAR_STATUS_OK) {
        blitzar_simulation_destroy(simulation);
        blitzar_context_destroy(context);
        return 1;
    }
    double output_x[2] = {0.0, 0.0};
    double output_y[2] = {0.0, 0.0};
    double output_z[2] = {0.0, 0.0};
    double output_velocity_x[2] = {0.0, 0.0};
    double output_velocity_y[2] = {0.0, 0.0};
    double output_velocity_z[2] = {0.0, 0.0};
    double output_mass[2] = {0.0, 0.0};
    if (blitzar_simulation_get_state(
            simulation,
            2,
            output_x,
            output_y,
            output_z,
            output_velocity_x,
            output_velocity_y,
            output_velocity_z,
            output_mass) != BLITZAR_STATUS_OK) {
        blitzar_simulation_destroy(simulation);
        blitzar_context_destroy(context);
        return 1;
    }
    blitzar_simulation_destroy(simulation);
    blitzar_context_destroy(context);
    return 0;
}
