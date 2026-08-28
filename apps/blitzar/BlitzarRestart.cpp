#include "BlitzarRestart.hpp"

#include "io/metadata/MetadataReader.hpp"
#include "io/snapshot/SnapshotReader.hpp"

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
    blitzar_sim::SimConfigState& state, blitzar_core::SnapshotHeader& header) noexcept
{
    try {
        const std::string file_name = blitzar_io::StateFileName(config.restart.step);

        if (file_name.empty()) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        const std::filesystem::path path = config.restart.directory / "states" / file_name;
        blitzar_io::SnapshotReader reader(
            static_cast<std::size_t>(blitzar_core::SnapshotMaxParticleCount));

        const blitzar_status status = reader.Read(path, header, MutablePayload(state));

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }

        const double expected_time = static_cast<double>(config.restart.step) * config.timestep;

        if (header.particle_count != static_cast<std::uint64_t>(config.particle_count) ||
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

} // namespace

blitzar_status LoadRestartState(
    blitzar_sim::SimConfigRun& config, blitzar_sim::SimConfigState& destination) noexcept
{
    if (!config.restart.enabled || config.restart.step >= config.FinalStep() ||
        config.particle_count <= 0 ||
        config.particle_count > blitzar_sim::SimConfigRun::MaxParticleCount) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    try {
        blitzar_io::MetadataRunInfo source;
        std::vector<std::uint64_t> completed_steps;
        blitzar_io::MetadataReader reader;

        const blitzar_status metadata_status =
            reader.Read(config.restart.directory / "manifest.json", source, completed_steps);

        if (metadata_status != BLITZAR_STATUS_OK) {
            return metadata_status;
        }

        if (!IsCompatible(config, source) || !ContainsStep(completed_steps, config.restart.step)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        const std::size_t count = static_cast<std::size_t>(config.particle_count);
        blitzar_sim::SimConfigState candidate;
        const blitzar_status resize_status = ResizeState(count, candidate);

        if (resize_status != BLITZAR_STATUS_OK) {
            return resize_status;
        }

        blitzar_core::SnapshotHeader header{};
        const blitzar_status snapshot_status = ReadRestartSnapshot(config, candidate, header);

        if (snapshot_status != BLITZAR_STATUS_OK) {
            return snapshot_status;
        }

        destination = std::move(candidate);
        config.restart.time = header.time;

        return BLITZAR_STATUS_OK;
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
}

} // namespace blitzar_cli
