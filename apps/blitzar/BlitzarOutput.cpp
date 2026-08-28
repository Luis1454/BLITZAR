#include "BlitzarOutput.hpp"

#include <blitzar/blitzar.hpp>
#include <cstddef>
#include <filesystem>
#include <limits>
#include <new>
#include <span>
#include <stdexcept>
#include <string>

namespace blitzar_cli {

namespace {

[[nodiscard]] blitzar_io::MetadataRunInfo MakeMetadataInfo(const blitzar_sim::SimConfigRun& config)
{
    const blitzar::CapabilityReport capabilities = blitzar::capabilities();
    blitzar_io::MetadataRunInfo info;

    info.product_version = std::string(blitzar::version());
    info.plan_version = std::string(blitzar::plan_version());
    info.configuration.simulation = {static_cast<std::uint64_t>(config.particle_count),
        static_cast<std::uint64_t>(config.steps), config.timestep, config.solver,
        config.integrator};

    info.configuration.gravity = {config.gravitational_constant, config.softening};
    info.configuration.units = {config.length_scale, config.mass_scale, config.time_scale};
    info.configuration.barnes_hut = {config.barnes_hut.opening_angle,
        static_cast<std::uint64_t>(config.barnes_hut.max_particles),
        static_cast<std::uint64_t>(config.barnes_hut.max_cells),
        static_cast<std::uint64_t>(config.barnes_hut.leaf_capacity),
        static_cast<std::uint64_t>(config.barnes_hut.max_depth)};

    info.configuration.generation = {config.seed, config.deterministic};
    info.configuration.output = {config.output.enabled,
        static_cast<std::uint64_t>(config.output.every_steps), config.output.write_initial,
        config.output.write_final};

    info.configuration.diagnostics = {config.diagnostics.enabled,
        static_cast<std::uint64_t>(config.diagnostics.every_steps), config.diagnostics.energy,
        config.diagnostics.momentum, config.diagnostics.relative_error};

    info.capabilities = {capabilities.implemented_solver_mask, capabilities.unsupported_solver_mask,
        capabilities.deferred_feature_mask, capabilities.compiled_backend_mask};

    return info;
}

[[nodiscard]] bool HasValidParticleCount(const blitzar_sim::SimConfigRun& config) noexcept
{
    return config.particle_count > 0 && static_cast<std::uintmax_t>(config.particle_count) <=
                                            std::numeric_limits<std::size_t>::max();
}

[[nodiscard]] blitzar_core::SnapshotFrameView MakeFrame(std::uint64_t step,
    blitzar_core::Scalar time, blitzar_core::ParticleOutputView state,
    std::span<const std::uint64_t> ids) noexcept
{
    blitzar_core::SnapshotHeader header{};

    header.particle_count = state.count;
    header.step = step;
    header.time = time;

    const blitzar_core::SnapshotPayloadView payload{ids,
        std::span<const blitzar_core::Scalar>(state.x),
        std::span<const blitzar_core::Scalar>(state.y),
        std::span<const blitzar_core::Scalar>(state.z),
        std::span<const blitzar_core::Scalar>(state.velocity_x),
        std::span<const blitzar_core::Scalar>(state.velocity_y),
        std::span<const blitzar_core::Scalar>(state.velocity_z),
        std::span<const blitzar_core::Scalar>(state.mass)};

    return {header, payload};
}

} // namespace

BlitzarOutput::BlitzarOutput(const blitzar_sim::SimConfigRun& config) noexcept : config_(config) {}

blitzar_status BlitzarOutput::Prepare(std::span<const std::uint64_t> ids) noexcept
{
    if (prepared_) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    if (!config_.output.enabled) {
        prepared_ = true;

        return BLITZAR_STATUS_OK;
    }

    if (!HasValidParticleCount(config_) ||
        ids.size() != static_cast<std::size_t>(config_.particle_count)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    try {
        ids_ = ids;

        run_.emplace(config_.output.directory, MakeMetadataInfo(config_));

        const blitzar_status status = run_->Prepare();

        if (status != BLITZAR_STATUS_OK) {
            run_.reset();

            ids_ = {};

            return status;
        }

        prepared_ = true;

        return BLITZAR_STATUS_OK;
    }
    catch (const std::bad_alloc&) {
        run_.reset();

        ids_ = {};

        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    catch (const std::length_error&) {
        run_.reset();

        ids_ = {};

        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::filesystem::filesystem_error&) {
        run_.reset();

        ids_ = {};

        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
}

bool BlitzarOutput::ShouldWriteInitial() const noexcept
{
    return config_.output.enabled && config_.output.write_initial;
}

bool BlitzarOutput::ShouldWriteStep(std::uint64_t step) const noexcept
{
    if (!config_.output.enabled || config_.output.every_steps <= 0 || step <= config_.StartStep()) {
        return false;
    }

    const std::uint64_t final_step = config_.FinalStep();
    const std::uint64_t interval = static_cast<std::uint64_t>(config_.output.every_steps);

    if (step > final_step) {
        return false;
    }

    return step % interval == 0 || (config_.output.write_final && step == final_step);
}

blitzar_status BlitzarOutput::Publish(
    std::uint64_t step, blitzar_core::ParticleOutputView state) noexcept
{
    if (!prepared_ || !run_.has_value() || !HasValidParticleCount(config_) ||
        state.count != static_cast<std::size_t>(config_.particle_count) ||
        ids_.size() != state.count || !blitzar_core::IsValid(state) || step < config_.StartStep() ||
        step > config_.FinalStep()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const std::uint64_t start_step = config_.StartStep();
    const blitzar_core::Scalar time = step == start_step
        ? config_.StartTime()
        : static_cast<blitzar_core::Scalar>(step) * config_.timestep;

    const blitzar_core::SnapshotFrameView frame = MakeFrame(step, time, state, ids_);

    return run_->PublishSnapshot(frame);
}

std::size_t BlitzarOutput::SnapshotCount() const noexcept
{
    return run_.has_value() ? run_->CompletedOutputCount() : 0U;
}

std::size_t BlitzarOutput::DiagnosticsCount() const noexcept
{
    return 0U;
}

const std::filesystem::path& BlitzarOutput::OutputPath() const noexcept
{
    return config_.output.directory;
}

} // namespace blitzar_cli
