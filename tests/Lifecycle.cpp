#include <blitzar/blitzar.hpp>

#include <array>
#include "Check.hpp"
#include <utility>

int main()
{
    blitzar::Context first;
    BLITZAR_CHECK(first.valid());
    BLITZAR_CHECK(first.status() == blitzar::Status::Ok);

    blitzar::Context second(std::move(first));
    BLITZAR_CHECK(!first.valid());
    BLITZAR_CHECK(second.valid());

    blitzar::Context third;
    third = std::move(second);
    BLITZAR_CHECK(!second.valid());
    BLITZAR_CHECK(third.valid());

    blitzar::Simulation simulation(third, 2);
    BLITZAR_CHECK(simulation.valid());
    BLITZAR_CHECK(simulation.set_solver(blitzar::SolverKind::Direct) ==
           blitzar::Status::Ok);
    BLITZAR_CHECK(simulation.set_solver(blitzar::SolverKind::Fmm) ==
           blitzar::Status::Unsupported);
    BLITZAR_CHECK(simulation.set_integrator(blitzar::IntegratorKind::LeapfrogKdk) ==
           blitzar::Status::Ok);
    BLITZAR_CHECK(simulation.set_gravity(1.0, 0.0) == blitzar::Status::Ok);
    BLITZAR_CHECK(simulation.set_units(2.0, 3.0, 4.0) == blitzar::Status::Ok);
    BLITZAR_CHECK(simulation.set_timestep(0.5) == blitzar::Status::Ok);
    BLITZAR_CHECK(simulation.set_seed(42) == blitzar::Status::Ok);
    const std::array<double, 2> position_x{0.0, 1.0};
    const std::array<double, 2> position_y{0.0, 0.0};
    const std::array<double, 2> position_z{0.0, 0.0};
    const std::array<double, 2> velocity_x{0.0, 0.0};
    const std::array<double, 2> velocity_y{0.0, 0.0};
    const std::array<double, 2> velocity_z{0.0, 0.0};
    const std::array<double, 2> mass{1.0, 1.0};
    BLITZAR_CHECK(simulation.set_particles(
               position_x,
               position_y,
               position_z,
               velocity_x,
               velocity_y,
               velocity_z,
               mass) == blitzar::Status::Ok);
    BLITZAR_CHECK(simulation.step() == blitzar::Status::Ok);
    std::array<double, 2> output_x{};
    std::array<double, 2> output_y{};
    std::array<double, 2> output_z{};
    std::array<double, 2> output_velocity_x{};
    std::array<double, 2> output_velocity_y{};
    std::array<double, 2> output_velocity_z{};
    std::array<double, 2> output_mass{};
    BLITZAR_CHECK(simulation.get_state(
               output_x,
               output_y,
               output_z,
               output_velocity_x,
               output_velocity_y,
               output_velocity_z,
               output_mass) == blitzar::Status::Ok);
    BLITZAR_CHECK(output_x[0] != 0.0);
    BLITZAR_CHECK(output_mass[0] == 1.0);

    blitzar::Simulation moved_simulation(std::move(simulation));
    BLITZAR_CHECK(!simulation.valid());
    BLITZAR_CHECK(moved_simulation.valid());
    return 0;
}
