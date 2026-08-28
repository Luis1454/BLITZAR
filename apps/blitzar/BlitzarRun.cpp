#include "BlitzarRun.hpp"

#include "BlitzarOutput.hpp"
#include "BlitzarRestart.hpp"
#include "BlitzarSummary.hpp"
#include "simulation/config/SimConfigFile.hpp"
#include "simulation/config/SimConfigRun.hpp"
#include "simulation/initialization/SimConfigState.hpp"

#include <blitzar/blitzar.hpp>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <new>
#include <stdexcept>
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

int PrintFailure(BlitzarStreams streams, std::string_view phase, blitzar_status status,
    BlitzarExitCode exit_code)
{
    const BlitzarFailure failure{status, phase, exit_code};

    (void)WriteFailure(streams.standard_error, failure);

    return static_cast<int>(exit_code);
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

struct RunResult final {
    blitzar_status status{BLITZAR_STATUS_OK};
    std::string_view phase{};
    BlitzarExitCode exit_code{BlitzarExitCode::Runtime};
    std::uint64_t completed_steps{};
};

[[nodiscard]] blitzar_status CaptureState(
    blitzar::Simulation& simulation, blitzar_sim::SimConfigState& state) noexcept
{
    blitzar::ParticleOutput output = ToOutput(state.Output());

    return static_cast<blitzar_status>(simulation.get_state(output));
}

[[nodiscard]] RunResult RunSteps(const blitzar_sim::SimConfigRun& config,
    blitzar::Simulation& simulation, blitzar_sim::SimConfigState& state,
    BlitzarOutput& output) noexcept
{
    const std::uint64_t start_step = config.StartStep();
    const std::uint64_t final_step = config.FinalStep();
    std::uint64_t completed_steps = start_step;

    const bool write_initial = output.ShouldWriteInitial();
    const bool write_initial_diagnostics = output.ShouldWriteDiagnostics(start_step);

    if (write_initial || write_initial_diagnostics) {
        blitzar_status status = CaptureState(simulation, state);

        if (status != BLITZAR_STATUS_OK) {
            return {status, "output-state", BlitzarExitCode::Output, completed_steps};
        }

        if (write_initial) {
            status = output.Publish(start_step, state.Output());

            if (status != BLITZAR_STATUS_OK) {
                return {status, "output-publish", BlitzarExitCode::Output, completed_steps};
            }
        }

        if (write_initial_diagnostics) {
            status = output.PublishDiagnostics(start_step, state.Output());

            if (status != BLITZAR_STATUS_OK) {
                return {status, "diagnostics-publish", BlitzarExitCode::Output, completed_steps};
            }
        }
    }

    for (std::uint64_t step = start_step + 1U; step <= final_step; ++step) {
        const blitzar::Status status = simulation.step();

        if (status != blitzar::Status::Ok) {
            return {static_cast<blitzar_status>(status), "step", BlitzarExitCode::Runtime,
                completed_steps};
        }

        completed_steps = step;

        const bool write_step = output.ShouldWriteStep(step);
        const bool write_diagnostics = output.ShouldWriteDiagnostics(step);

        if (!write_step && !write_diagnostics) {
            continue;
        }

        blitzar_status output_status = CaptureState(simulation, state);

        if (output_status != BLITZAR_STATUS_OK) {
            return {output_status, "output-state", BlitzarExitCode::Output, completed_steps};
        }

        if (write_step) {
            output_status = output.Publish(step, state.Output());

            if (output_status != BLITZAR_STATUS_OK) {
                return {output_status, "output-publish", BlitzarExitCode::Output, completed_steps};
            }
        }

        if (write_diagnostics) {
            output_status = output.PublishDiagnostics(step, state.Output());

            if (output_status != BLITZAR_STATUS_OK) {
                return {
                    output_status, "diagnostics-publish", BlitzarExitCode::Output, completed_steps};
            }
        }
    }

    return {BLITZAR_STATUS_OK, {}, BlitzarExitCode::Runtime, completed_steps};
}

[[nodiscard]] int PrintSummary(BlitzarStreams streams, const blitzar_sim::SimConfigRun& config,
    const BlitzarOutput& output_writer, std::uint64_t completed_steps)
{
    try {
        const BlitzarSummary summary{BLITZAR_STATUS_OK, config.FinalStep(), completed_steps,
            static_cast<std::uint64_t>(config.particle_count), config.solver,
            static_cast<std::uint64_t>(output_writer.SnapshotCount()),
            static_cast<std::uint64_t>(output_writer.DiagnosticsCount()),
            output_writer.OutputPath()};

        if (WriteSummary(streams.standard_output, summary)) {
            return static_cast<int>(BlitzarExitCode::Success);
        }
    }
    catch (const std::bad_alloc&) {
        return PrintFailure(
            streams, "summary", BLITZAR_STATUS_ALLOCATION_FAILURE, BlitzarExitCode::Output);
    }
    catch (const std::length_error&) {
        return PrintFailure(
            streams, "summary", BLITZAR_STATUS_INVALID_ARGUMENT, BlitzarExitCode::Output);
    }
    catch (const std::filesystem::filesystem_error&) {
        return PrintFailure(
            streams, "summary", BLITZAR_STATUS_INTERNAL_ERROR, BlitzarExitCode::Output);
    }

    return PrintFailure(streams, "summary", BLITZAR_STATUS_INTERNAL_ERROR, BlitzarExitCode::Output);
}

} // namespace

int RunSmoke()
{
    const blitzar::Context context{};

    if (!context.valid()) {
        const auto status = static_cast<blitzar_status>(context.status());

        std::cerr << "BLITZAR context error: " << blitzar_status_message(status) << '\n';

        return static_cast<int>(BlitzarExitCode::Runtime);
    }

    std::cout << "BLITZAR context ready\n";

    return 0;
}

int RunConfig(const std::filesystem::path& path)
{
    return RunConfig(path, {std::cout, std::cerr});
}

int RunConfig(const std::filesystem::path& path, BlitzarStreams streams)
{
    blitzar_sim::SimConfigFile source;
    blitzar_status status = blitzar_sim::LoadConfig(path, source);

    if (status != BLITZAR_STATUS_OK) {
        return PrintFailure(streams, "load", status, BlitzarExitCode::Configuration);
    }

    blitzar_sim::SimConfigRun config;

    const std::filesystem::path config_directory = path.parent_path();

    status = blitzar_sim::BuildRunConfig(source, config_directory, config);

    if (status != BLITZAR_STATUS_OK) {
        return PrintFailure(streams, "semantic", status, BlitzarExitCode::Configuration);
    }

    blitzar_sim::SimConfigState state;

    status = config.restart.enabled ? LoadRestartState(config, state)
                                    : blitzar_sim::BuildState(config, state);

    if (status != BLITZAR_STATUS_OK) {
        return PrintFailure(streams, config.restart.enabled ? "restart" : "state", status,
            BlitzarExitCode::Configuration);
    }

    blitzar::Context context{};

    if (!context.valid()) {
        return PrintFailure(streams, "context", static_cast<blitzar_status>(context.status()),
            BlitzarExitCode::Runtime);
    }

    blitzar::Simulation simulation(context, config.particle_count);

    if (!simulation.valid()) {
        return PrintFailure(streams, "simulation", static_cast<blitzar_status>(simulation.status()),
            BlitzarExitCode::Runtime);
    }

    status = ConfigureSimulation(config, simulation);

    if (status != BLITZAR_STATUS_OK) {
        return PrintFailure(streams, "configure", status, BlitzarExitCode::Configuration);
    }

    status = static_cast<blitzar_status>(simulation.set_particles(ToInput(state.Input())));

    if (status != BLITZAR_STATUS_OK) {
        return PrintFailure(streams, "particles", status, BlitzarExitCode::Configuration);
    }

    BlitzarOutput output_writer(config);

    status = output_writer.Prepare(state.Ids());

    if (status != BLITZAR_STATUS_OK) {
        return PrintFailure(streams, "output-prepare", status, BlitzarExitCode::Output);
    }

    const RunResult run_result = RunSteps(config, simulation, state, output_writer);

    if (run_result.status != BLITZAR_STATUS_OK) {
        return PrintFailure(streams, run_result.phase, run_result.status, run_result.exit_code);
    }

    return PrintSummary(streams, config, output_writer, run_result.completed_steps);
}

} // namespace blitzar_cli
