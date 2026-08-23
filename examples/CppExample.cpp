#include <array>
#include <blitzar/blitzar.hpp>

int main()
{
    if (blitzar::version()[0] == '\0' || blitzar::plan_version()[0] == '\0') {
        return 1;
    }

    blitzar::Context context{};
    if (!context.valid()) {
        return 1;
    }
    blitzar::Simulation simulation(context, 2);
    if (!simulation.valid()) {
        return 1;
    }
    const std::array<double, 2> position_x{0.0, 1.0};
    const std::array<double, 2> position_y{0.0, 0.0};
    const std::array<double, 2> position_z{0.0, 0.0};
    const std::array<double, 2> velocity_x{0.0, 0.0};
    const std::array<double, 2> velocity_y{0.0, 0.0};
    const std::array<double, 2> velocity_z{0.0, 0.0};
    const std::array<double, 2> mass{1.0, 1.0};
    if (simulation.set_particles(position_x, position_y, position_z, velocity_x, velocity_y,
            velocity_z, mass) != blitzar::Status::Ok ||
        simulation.set_timestep(0.5) != blitzar::Status::Ok ||
        simulation.step() != blitzar::Status::Ok) {
        return 1;
    }
    std::array<double, 2> output_x{};
    std::array<double, 2> output_y{};
    std::array<double, 2> output_z{};
    std::array<double, 2> output_velocity_x{};
    std::array<double, 2> output_velocity_y{};
    std::array<double, 2> output_velocity_z{};
    std::array<double, 2> output_mass{};
    return simulation.get_state(output_x, output_y, output_z, output_velocity_x, output_velocity_y,
               output_velocity_z, output_mass) == blitzar::Status::Ok
               ? 0
               : 1;
}
