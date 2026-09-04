#include "io/postprocess/PostProcessState.hpp"

#include "io/hdf5/Hdf5Reader.hpp"
#include "io/snapshot/SnapshotReader.hpp"
#include "simulation/initialization/SimConfigState.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <new>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

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

[[nodiscard]] blitzar_status ReadSnapshot(const std::filesystem::path& path,
    MetadataOutputFormat format, SnapshotReader& binary_reader, Hdf5Reader& hdf5_reader,
    blitzar_sim::SimConfigState& state, blitzar_core::SnapshotHeader& header) noexcept
{
    return format == MetadataOutputFormat::Hdf5
               ? hdf5_reader.Read(path, header, MutablePayload(state))
               : binary_reader.Read(path, header, MutablePayload(state));
}

[[nodiscard]] blitzar_status CopyShard(const blitzar_sim::SimConfigState& shard,
    std::size_t shard_count, blitzar_sim::SimConfigState& state, std::span<std::uint8_t> seen,
    std::size_t& total_count) noexcept
{
    const std::size_t global_count = state.ids.size();

    if (shard_count > global_count || shard_count > global_count - total_count ||
        seen.size() != global_count || shard.ids.size() < shard_count ||
        shard.position_x.size() < shard_count || shard.position_y.size() < shard_count ||
        shard.position_z.size() < shard_count || shard.velocity_x.size() < shard_count ||
        shard.velocity_y.size() < shard_count || shard.velocity_z.size() < shard_count ||
        shard.mass.size() < shard_count) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    for (std::size_t index = 0; index < shard_count; ++index) {
        const std::uint64_t id = shard.ids[index];

        if (id >= static_cast<std::uint64_t>(global_count) ||
            seen[static_cast<std::size_t>(id)] != 0U) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        const std::size_t destination = static_cast<std::size_t>(id);

        seen[destination] = 1U;
        state.ids[destination] = id;
        state.position_x[destination] = shard.position_x[index];
        state.position_y[destination] = shard.position_y[index];
        state.position_z[destination] = shard.position_z[index];
        state.velocity_x[destination] = shard.velocity_x[index];
        state.velocity_y[destination] = shard.velocity_y[index];
        state.velocity_z[destination] = shard.velocity_z[index];
        state.mass[destination] = shard.mass[index];
    }

    total_count += shard_count;

    return BLITZAR_STATUS_OK;
}

[[nodiscard]] bool HasCompleteIds(std::span<const std::uint8_t> seen) noexcept
{
    return std::all_of(seen.begin(), seen.end(), [](std::uint8_t value) { return value != 0U; });
}

[[nodiscard]] blitzar_status ProcessSingleSnapshot(const PostProcessInput& input,
    std::uint64_t step, SnapshotReader& binary_reader, Hdf5Reader& hdf5_reader,
    blitzar_sim::SimConfigState& state, blitzar_core::SnapshotHeader& header) noexcept
{
    const std::string file_name = StateFileName(step, input.info.configuration.output.format);

    if (file_name.empty()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_status read_status = ReadSnapshot(input.states_path / file_name,
        input.info.configuration.output.format, binary_reader, hdf5_reader, state, header);

    if (read_status != BLITZAR_STATUS_OK) {
        return read_status;
    }

    return ValidateSnapshotFrame(header, input.info, step);
}

[[nodiscard]] blitzar_status ProcessDistributedSnapshot(const PostProcessInput& input,
    std::uint64_t step, SnapshotReader& binary_reader, Hdf5Reader& hdf5_reader,
    blitzar_sim::SimConfigState& shard, blitzar_sim::SimConfigState& state,
    std::span<std::uint8_t> seen) noexcept
{
    std::size_t total_count = 0U;
    const MetadataOutputFormat format = input.info.configuration.output.format;

    for (std::uint32_t rank = 0; rank < input.info.rank_count; ++rank) {
        const std::string file_name = StateShardFileName(step, rank, format);

        if (file_name.empty()) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        blitzar_core::SnapshotHeader header{};
        const blitzar_status read_status = ReadSnapshot(
            input.states_path / file_name, format, binary_reader, hdf5_reader, shard, header);

        if (read_status != BLITZAR_STATUS_OK) {
            return read_status;
        }

        const blitzar_status frame_status = ValidateSnapshotFrame(header, input.info, step, rank);

        if (frame_status != BLITZAR_STATUS_OK) {
            return frame_status;
        }

        const blitzar_status copy_status = CopyShard(
            shard, static_cast<std::size_t>(header.particle_count), state, seen, total_count);

        if (copy_status != BLITZAR_STATUS_OK) {
            return copy_status;
        }
    }

    return total_count == state.ids.size() && HasCompleteIds(seen)
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INVALID_ARGUMENT;
}

} // namespace

blitzar_status ProcessSnapshots(const PostProcessInput& input, ConservationCsv& output) noexcept
{
    try {
        const std::size_t count =
            static_cast<std::size_t>(input.info.configuration.simulation.particle_count);

        blitzar_sim::SimConfigState state;
        const blitzar_status resize_status = ResizeState(count, state);

        if (resize_status != BLITZAR_STATUS_OK) {
            return resize_status;
        }

        blitzar_sim::SimConfigState shard;
        std::vector<std::uint8_t> seen;

        if (input.info.rank_count > 1U) {
            const blitzar_status shard_resize_status = ResizeState(count, shard);

            if (shard_resize_status != BLITZAR_STATUS_OK) {
                return shard_resize_status;
            }

            seen.resize(count);
        }

        SnapshotReader binary_reader(count);
        Hdf5Reader hdf5_reader(count);
        const blitzar_physics::GravityParameters gravity = MakeGravity(input.info);

        const MetadataDiagnostics& diagnostics = input.info.configuration.diagnostics;

        const bool distributed = input.info.rank_count > 1U;

        for (const std::uint64_t step : input.completed_steps) {
            std::fill(seen.begin(), seen.end(), 0U);

            blitzar_core::SnapshotHeader header{};
            const blitzar_status frame_status =
                distributed
                    ? ProcessDistributedSnapshot(
                          input, step, binary_reader, hdf5_reader, shard, state, seen)
                    : ProcessSingleSnapshot(input, step, binary_reader, hdf5_reader, state, header);

            if (frame_status != BLITZAR_STATUS_OK) {
                return frame_status;
            }

            if (distributed) {
                header.step = step;
                header.time =
                    static_cast<double>(step) * input.info.configuration.simulation.timestep;
            }

            blitzar_physics::ConservationMetrics metrics{};
            const blitzar_status metrics_status =
                blitzar_physics::ComputeConservationMetrics(state.Input(), gravity, metrics);

            if (metrics_status != BLITZAR_STATUS_OK) {
                return metrics_status;
            }

            if (ShouldWriteDiagnostic(input.info, step)) {
                const ConservationSample sample{step, header.time,
                    static_cast<std::uint64_t>(state.ids.size()), metrics,
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
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
}

} // namespace blitzar_io
