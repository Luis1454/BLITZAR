#include "BlitzarRun.hpp"

#include "simulation/input/SimConfigFile.hpp"
#include "simulation/input/SimConfigRun.hpp"
#include "simulation/input/SimConfigState.hpp"

#include <blitzar/blitzar.hpp>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string_view>

namespace blitzar_cli {

namespace {

[[nodiscard]] blitzar::ParticleInput ToInput(blitzar_core::ParticleStateView input) noexcept
{
    return {input.x, input.y, input.z, input.velocity_x, input.velocity_y, input.velocity_z,
        input.mass};
}

[[nodiscard]] blitzar::ParticleOutput ToOutput(blitzar_core::ParticleOutputView output) noexcept
{
    return {output.x, output.y, output.z, output.velocity_x, output.velocity_y, output.velocity_z,
        output.mass};
}

int PrintFailure(std::string_view phase, blitzar_status status)
{
    std::cerr << "BLITZAR CONFIG status=" << status << " phase=" << phase
              << " message=" << blitzar_status_message(status) << '\n';

    return 1;
}

[[nodiscard]] blitzar_status ConfigureSimulation(
    const blitzar_sim::SimConfigRun& config, blitzar::Simulation& simulation) noexcept
{
    blitzar::Status status = simulation.set_solver(static_cast<blitzar::SolverKind>(config.solver));

    if (status != blitzar::Status::Ok) {
        return static_cast<blitzar_status>(status);
    }

    status = simulation.set_integrator(static_cast<blitzar::IntegratorKind>(config.integrator));

    if (status != blitzar::Status::Ok) {
        return static_cast<blitzar_status>(status);
    }

    status = simulation.set_gravity(config.gravitational_constant, config.softening);

    if (status != blitzar::Status::Ok) {
        return static_cast<blitzar_status>(status);
    }

    status = simulation.set_units(config.length_scale, config.mass_scale, config.time_scale);

    if (status != blitzar::Status::Ok) {
        return static_cast<blitzar_status>(status);
    }

    status = simulation.set_barnes_hut({config.barnes_hut.opening_angle,
        config.barnes_hut.max_particles, config.barnes_hut.max_cells,
        config.barnes_hut.leaf_capacity, config.barnes_hut.max_depth});

    if (status != blitzar::Status::Ok) {
        return static_cast<blitzar_status>(status);
    }

    status = simulation.set_timestep(config.timestep);

    if (status != blitzar::Status::Ok) {
        return static_cast<blitzar_status>(status);
    }

    return static_cast<blitzar_status>(simulation.set_seed(config.seed));
}

[[nodiscard]] blitzar_status RunSteps(
    const blitzar_sim::SimConfigRun& config, blitzar::Simulation& simulation) noexcept
{
    for (std::int64_t step = 0; step < config.steps; ++step) {
        const blitzar::Status status = simulation.step();

        if (status != blitzar::Status::Ok) {
            return static_cast<blitzar_status>(status);
        }
    }

    return BLITZAR_STATUS_OK;
}

void PrintResult(const blitzar_sim::SimConfigRun& config, blitzar::ParticleOutput output)
{
    const std::size_t last = output.position_x.size() - 1U;

    std::cout << std::setprecision(17)
              << "BLITZAR RUN schema=1 status=0 particles=" << config.particle_count
              << " steps=" << config.steps << " seed=" << config.seed
              << " solver=" << static_cast<std::int32_t>(config.solver)
              << " first_x=" << output.position_x.front() << " last_x=" << output.position_x[last]
              << " first_vx=" << output.velocity_x.front() << " last_vx=" << output.velocity_x[last]
              << '\n';
}

} // namespace

int RunSmoke()
{
    const blitzar::Context context{};

    if (!context.valid()) {
        const auto status = static_cast<blitzar_status>(context.status());

        std::cerr << "BLITZAR context error: " << blitzar_status_message(status) << '\n';

        return 1;
    }

    std::cout << "BLITZAR context ready\n";

    return 0;
}

int RunConfig(const std::filesystem::path& path)
{
    blitzar_sim::SimConfigFile source;
    blitzar_status status = blitzar_sim::LoadConfig(path, source);

    if (status != BLITZAR_STATUS_OK) {
        return PrintFailure("load", status);
    }

    blitzar_sim::SimConfigRun config;

    status = blitzar_sim::BuildRunConfig(source, config);

    if (status != BLITZAR_STATUS_OK) {
        return PrintFailure("semantic", status);
    }

    blitzar_sim::SimConfigState state;

    status = blitzar_sim::BuildState(config, state);

    if (status != BLITZAR_STATUS_OK) {
        return PrintFailure("state", status);
    }

    blitzar::Context context{};

    if (!context.valid()) {
        return PrintFailure("context", static_cast<blitzar_status>(context.status()));
    }

    blitzar::Simulation simulation(context, config.particle_count);

    if (!simulation.valid()) {
        return PrintFailure("simulation", static_cast<blitzar_status>(simulation.status()));
    }

    status = ConfigureSimulation(config, simulation);

    if (status != BLITZAR_STATUS_OK) {
        return PrintFailure("configure", status);
    }

    status = static_cast<blitzar_status>(simulation.set_particles(ToInput(state.Input())));

    if (status != BLITZAR_STATUS_OK) {
        return PrintFailure("particles", status);
    }

    status = RunSteps(config, simulation);

    if (status != BLITZAR_STATUS_OK) {
        return PrintFailure("step", status);
    }

    blitzar::ParticleOutput output = ToOutput(state.Output());

    status = static_cast<blitzar_status>(simulation.get_state(output));

    if (status != BLITZAR_STATUS_OK) {
        return PrintFailure("state-output", status);
    }

    PrintResult(config, output);

    return 0;
}

} // namespace blitzar_cli
