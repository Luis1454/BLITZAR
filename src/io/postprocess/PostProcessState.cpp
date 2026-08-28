#include "io/postprocess/PostProcessState.hpp"

#include "io/snapshot/SnapshotReader.hpp"
#include "simulation/initialization/SimConfigState.hpp"

#include <cstddef>
#include <cstdint>
#include <new>
#include <stdexcept>
#include <string>

namespace blitzar_io {

namespace {

[[nodiscard]] blitzar_status ResizeState(
    std::size_t count, blitzar_sim::SimConfigState& state) noexcept
{
    try {
        state.ids.resize(count);
        state.position_x.resize(count);
        state.position_y.resize(count);
        state.position_z.resize(count);
        state.velocity_x.resize(count);
        state.velocity_y.resize(count);
        state.velocity_z.resize(count);
        state.mass.resize(count);
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    return BLITZAR_STATUS_OK;
}

[[nodiscard]] blitzar_core::SnapshotMutablePayloadView MutablePayload(
    blitzar_sim::SimConfigState& state) noexcept
{
    return {state.ids, state.position_x, state.position_y, state.position_z, state.velocity_x,
        state.velocity_y, state.velocity_z, state.mass};
}

[[nodiscard]] blitzar_physics::GravityParameters MakeGravity(const MetadataRunInfo& info) noexcept
{
    const MetadataGravity& gravity = info.configuration.gravity;
    const MetadataUnits& units = info.configuration.units;

    return {gravity.gravitational_constant, gravity.softening,
        {units.length_scale, units.mass_scale, units.time_scale}};
}

} // namespace

blitzar_status ProcessSnapshots(const PostProcessInput& input, ConservationCsv& output) noexcept
{
    const std::size_t count =
        static_cast<std::size_t>(input.info.configuration.simulation.particle_count);

    blitzar_sim::SimConfigState state;
    const blitzar_status resize_status = ResizeState(count, state);

    if (resize_status != BLITZAR_STATUS_OK) {
        return resize_status;
    }

    SnapshotReader reader(count);
    const blitzar_physics::GravityParameters gravity = MakeGravity(input.info);

    const MetadataDiagnostics& diagnostics = input.info.configuration.diagnostics;

    for (const std::uint64_t step : input.completed_steps) {
        const std::string file_name = StateFileName(step);

        if (file_name.empty()) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        blitzar_core::SnapshotHeader header{};
        const blitzar_status read_status =
            reader.Read(input.states_path / file_name, header, MutablePayload(state));

        if (read_status != BLITZAR_STATUS_OK) {
            return read_status;
        }

        const blitzar_status frame_status = ValidateSnapshotFrame(header, input.info, step);

        if (frame_status != BLITZAR_STATUS_OK) {
            return frame_status;
        }

        blitzar_physics::ConservationMetrics metrics{};
        const blitzar_status metrics_status =
            blitzar_physics::ComputeConservationMetrics(state.Input(), gravity, metrics);

        if (metrics_status != BLITZAR_STATUS_OK) {
            return metrics_status;
        }

        if (ShouldWriteDiagnostic(input.info, step)) {
            const ConservationSample sample{step, header.time, header.particle_count, metrics,
                !diagnostics.enabled || diagnostics.energy,
                !diagnostics.enabled || diagnostics.momentum,
                !diagnostics.enabled || diagnostics.relative_error};

            const blitzar_status write_status = output.Append(sample);

            if (write_status != BLITZAR_STATUS_OK) {
                return write_status;
            }
        }
    }

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_io
