#include "BlitzarRestart.hpp"

#include "io/hdf5/Hdf5Reader.hpp"
#include "io/metadata/MetadataReader.hpp"
#include "io/snapshot/SnapshotReader.hpp"
#include "mpi/runtime/MpiContext.hpp"

#include <algorithm>
#include <array>
#include <blitzar/blitzar.hpp>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <new>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace blitzar_cli {

namespace {

[[nodiscard]] bool EqualSimulation(
    const blitzar_sim::SimConfigRun& config, const blitzar_io::MetadataRunInfo& source) noexcept
{
    const blitzar_io::MetadataSimulation& simulation = source.configuration.simulation;

    return simulation.particle_count == static_cast<std::uint64_t>(config.particle_count) &&
           simulation.timestep == config.timestep && simulation.solver == config.solver &&
           simulation.integrator == config.integrator;
}

[[nodiscard]] bool EqualGravity(
    const blitzar_sim::SimConfigRun& config, const blitzar_io::MetadataRunInfo& source) noexcept
{
    const blitzar_io::MetadataGravity& gravity = source.configuration.gravity;

    return gravity.gravitational_constant == config.gravitational_constant &&
           gravity.softening == config.softening;
}

[[nodiscard]] bool EqualUnits(
    const blitzar_sim::SimConfigRun& config, const blitzar_io::MetadataRunInfo& source) noexcept
{
    const blitzar_io::MetadataUnits& units = source.configuration.units;

    return units.length_scale == config.length_scale && units.mass_scale == config.mass_scale &&
           units.time_scale == config.time_scale;
}

[[nodiscard]] bool EqualBarnesHut(
    const blitzar_sim::SimConfigRun& config, const blitzar_io::MetadataRunInfo& source) noexcept
{
    const blitzar_io::MetadataBarnesHut& barnes_hut = source.configuration.barnes_hut;

    return barnes_hut.opening_angle == config.barnes_hut.opening_angle &&
           barnes_hut.max_particles ==
               static_cast<std::uint64_t>(config.barnes_hut.max_particles) &&
           barnes_hut.max_cells == static_cast<std::uint64_t>(config.barnes_hut.max_cells) &&
           barnes_hut.leaf_capacity ==
               static_cast<std::uint64_t>(config.barnes_hut.leaf_capacity) &&
           barnes_hut.max_depth == static_cast<std::uint64_t>(config.barnes_hut.max_depth);
}

[[nodiscard]] bool EqualGeneration(
    const blitzar_sim::SimConfigRun& config, const blitzar_io::MetadataRunInfo& source) noexcept
{
    const blitzar_io::MetadataGeneration& generation = source.configuration.generation;

    return generation.seed == config.seed && generation.deterministic == config.deterministic;
}

[[nodiscard]] bool IsCompatible(
    const blitzar_sim::SimConfigRun& config, const blitzar_io::MetadataRunInfo& source) noexcept
{
    return source.product_version == blitzar::version() &&
           source.plan_version == blitzar::plan_version() && EqualSimulation(config, source) &&
           EqualGravity(config, source) && EqualUnits(config, source) &&
           EqualBarnesHut(config, source) && EqualGeneration(config, source);
}

[[nodiscard]] bool ContainsStep(
    std::span<const std::uint64_t> completed_steps, std::uint64_t step) noexcept
{
    for (const std::uint64_t completed_step : completed_steps) {
        if (completed_step == step) {
            return true;
        }
    }

    return false;
}

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

[[nodiscard]] blitzar_status ReadRestartSnapshot(const blitzar_sim::SimConfigRun& config,
    const blitzar_io::MetadataRunInfo& source, blitzar_sim::SimConfigState& state,
    blitzar_core::SnapshotHeader& header) noexcept
{
    try {
        const blitzar_io::MetadataOutputFormat format = source.configuration.output.format;
        const std::string file_name = blitzar_io::StateFileName(config.restart.step, format);

        if (file_name.empty()) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        const std::filesystem::path path = config.restart.directory / "states" / file_name;
        blitzar_io::SnapshotReader binary_reader(
            static_cast<std::size_t>(blitzar_core::SnapshotMaxParticleCount));

        blitzar_io::Hdf5Reader hdf5_reader(
            static_cast<std::size_t>(blitzar_core::SnapshotMaxParticleCount));

        const blitzar_status status = format == blitzar_io::MetadataOutputFormat::Hdf5
                                          ? hdf5_reader.Read(path, header, MutablePayload(state))
                                          : binary_reader.Read(path, header, MutablePayload(state));

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }

        const double expected_time = static_cast<double>(config.restart.step) * config.timestep;

        if (header.distribution != blitzar_core::SnapshotDistribution::SingleRank ||
            header.id_policy != blitzar_core::SnapshotIdPolicy::GlobalContiguous ||
            header.rank_count != 1U || header.rank_index != 0U ||
            header.particle_count != static_cast<std::uint64_t>(config.particle_count) ||
            header.step != config.restart.step || header.time != expected_time ||
            !std::isfinite(header.time)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    return BLITZAR_STATUS_OK;
}

[[nodiscard]] blitzar_status CopyShard(const blitzar_sim::SimConfigState& shard,
    std::size_t shard_count, blitzar_sim::SimConfigState& destination, std::span<std::uint8_t> seen,
    std::size_t& total_count) noexcept
{
    const std::size_t global_count = destination.ids.size();

    if (total_count > global_count || shard_count > global_count - total_count ||
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

        const std::size_t destination_index = static_cast<std::size_t>(id);

        seen[destination_index] = 1U;
        destination.ids[destination_index] = id;
        destination.position_x[destination_index] = shard.position_x[index];
        destination.position_y[destination_index] = shard.position_y[index];
        destination.position_z[destination_index] = shard.position_z[index];
        destination.velocity_x[destination_index] = shard.velocity_x[index];
        destination.velocity_y[destination_index] = shard.velocity_y[index];
        destination.velocity_z[destination_index] = shard.velocity_z[index];
        destination.mass[destination_index] = shard.mass[index];
    }

    total_count += shard_count;

    return BLITZAR_STATUS_OK;
}

[[nodiscard]] blitzar_status ReadDistributedRestart(const blitzar_sim::SimConfigRun& config,
    const blitzar_io::MetadataRunInfo& source, blitzar_sim::SimConfigState& destination) noexcept
{
    try {
        const std::size_t count = static_cast<std::size_t>(config.particle_count);
        blitzar_sim::SimConfigState shard;
        const blitzar_status shard_resize_status = ResizeState(count, shard);

        if (shard_resize_status != BLITZAR_STATUS_OK) {
            return shard_resize_status;
        }

        std::vector<std::uint8_t> seen(count);
        blitzar_io::SnapshotReader binary_reader(
            static_cast<std::size_t>(blitzar_core::SnapshotMaxParticleCount));

        blitzar_io::Hdf5Reader hdf5_reader(
            static_cast<std::size_t>(blitzar_core::SnapshotMaxParticleCount));

        std::size_t total_count = 0U;
        const double expected_time = static_cast<double>(config.restart.step) * config.timestep;
        const blitzar_io::MetadataOutputFormat format = source.configuration.output.format;

        for (std::uint32_t rank = 0; rank < source.rank_count; ++rank) {
            const std::string file_name =
                blitzar_io::StateShardFileName(config.restart.step, rank, format);

            if (file_name.empty()) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }

            blitzar_core::SnapshotHeader header{};
            const std::filesystem::path path = config.restart.directory / "states" / file_name;
            const blitzar_status read_status =
                format == blitzar_io::MetadataOutputFormat::Hdf5
                    ? hdf5_reader.Read(path, header, MutablePayload(shard))
                    : binary_reader.Read(path, header, MutablePayload(shard));

            if (read_status != BLITZAR_STATUS_OK) {
                return read_status;
            }

            if (header.distribution != blitzar_core::SnapshotDistribution::Sharded ||
                header.id_policy != blitzar_core::SnapshotIdPolicy::GlobalStable ||
                header.rank_count != source.rank_count || header.rank_index != rank ||
                header.step != config.restart.step || header.time != expected_time ||
                !std::isfinite(header.time)) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }

            const blitzar_status copy_status = CopyShard(shard,
                static_cast<std::size_t>(header.particle_count), destination, seen, total_count);

            if (copy_status != BLITZAR_STATUS_OK) {
                return copy_status;
            }
        }

        return total_count == count && std::all_of(seen.begin(), seen.end(),
                                           [](std::uint8_t value) { return value != 0U; })
                   ? BLITZAR_STATUS_OK
                   : BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
}

[[nodiscard]] blitzar_status Synchronize(const blitzar_parallel::MpiContext& context,
    blitzar_status local_status, std::string_view phase) noexcept
{
    if (!context.IsDistributed()) {
        return local_status;
    }

    blitzar_status global_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status =
        context.SynchronizeStatus(local_status, "BlitzarRestart", phase, global_status);

    return synchronization_status == BLITZAR_STATUS_OK ? global_status : synchronization_status;
}

} // namespace

blitzar_status LoadRestartState(
    blitzar_sim::SimConfigRun& config, blitzar_sim::SimConfigState& destination) noexcept
{
    if (!config.restart.enabled || config.restart.step >= config.FinalStep() ||
        config.particle_count <= 0 ||
        config.particle_count > blitzar_sim::SimConfigRun::MaxParticleCount) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    blitzar_parallel::MpiContext context;

    if (!context.IsUsable()) {
        return context.Status();
    }

    const bool root = context.Rank() == 0;
    blitzar_io::MetadataRunInfo source;
    std::vector<std::uint64_t> completed_steps;
    blitzar_sim::SimConfigState candidate;
    blitzar_core::SnapshotHeader header{};
    blitzar_status local_status = BLITZAR_STATUS_OK;

    try {
        if (root) {
            blitzar_io::MetadataReader reader;

            local_status =
                reader.Read(config.restart.directory / "manifest.json", source, completed_steps);

            if (local_status == BLITZAR_STATUS_OK &&
                (!IsCompatible(config, source) || source.rank_index != 0U ||
                    source.rank_count != static_cast<std::uint32_t>(context.Size()) ||
                    !ContainsStep(completed_steps, config.restart.step))) {
                local_status = BLITZAR_STATUS_INVALID_ARGUMENT;
            }
        }
    }
    catch (const std::length_error&) {
        local_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        local_status = BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    blitzar_status status = Synchronize(context, local_status, "metadata");

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    try {
        const blitzar_status resize_status =
            root ? ResizeState(static_cast<std::size_t>(config.particle_count), candidate)
                 : BLITZAR_STATUS_OK;

        local_status = resize_status;

        if (local_status == BLITZAR_STATUS_OK && root) {
            local_status = source.rank_count == 1U
                               ? ReadRestartSnapshot(config, source, candidate, header)
                               : ReadDistributedRestart(config, source, candidate);

            if (local_status == BLITZAR_STATUS_OK && source.rank_count > 1U) {
                header.step = config.restart.step;
                header.time = static_cast<double>(config.restart.step) * config.timestep;
            }
        }
    }
    catch (const std::length_error&) {
        local_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        local_status = BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    status = Synchronize(context, local_status, "state");

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    std::array<blitzar_core::Scalar, 1> restart_time{root ? header.time : blitzar_core::Scalar{}};

    status = context.Broadcast(std::span<blitzar_core::Scalar>(restart_time), 0);

    if (status != BLITZAR_STATUS_OK || !std::isfinite(restart_time[0])) {
        return status == BLITZAR_STATUS_OK ? BLITZAR_STATUS_INVALID_ARGUMENT : status;
    }

    if (root) {
        destination = std::move(candidate);
    }

    config.restart.time = restart_time[0];

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_cli
