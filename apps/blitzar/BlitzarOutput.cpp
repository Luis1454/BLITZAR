#include "BlitzarOutput.hpp"

#include <blitzar/blitzar.hpp>
#include <cstddef>
#include <filesystem>
#include <limits>
#include <new>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace blitzar_cli {

namespace {

[[nodiscard]] blitzar_io::MetadataRunInfo MakeMetadataInfo(
    const blitzar_sim::SimConfigRun& config, std::uint32_t rank_count, std::uint32_t rank_index)
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
    info.configuration.execution = {config.execution.mode, config.execution.cpu,
        config.execution.hip, config.execution.mpi, "float64",
        blitzar_io::CurrentCompilerIdentity(), "host-cpu", "seeded-jitter-v1",
        "direct-plain;diagnostics-neumaier-v1", "stable-particle-id-v1",
        config.execution.IsBitwiseReproducible()};

    const blitzar_io::MetadataOutputFormat output_format =
        config.output.format == blitzar_sim::SimConfigOutputFormat::Hdf5
            ? blitzar_io::MetadataOutputFormat::Hdf5
            : blitzar_io::MetadataOutputFormat::Binary;

    info.configuration.output = {config.output.enabled,
        static_cast<std::uint64_t>(config.output.every_steps), config.output.write_initial,
        config.output.write_final, output_format};

    info.configuration.diagnostics = {config.diagnostics.enabled,
        static_cast<std::uint64_t>(config.diagnostics.every_steps), config.diagnostics.energy,
        config.diagnostics.momentum, config.diagnostics.relative_error};

    info.capabilities = {capabilities.implemented_solver_mask, capabilities.unsupported_solver_mask,
        capabilities.deferred_feature_mask, capabilities.compiled_backend_mask};

    info.rank_count = rank_count;
    info.rank_index = rank_index;

    return info;
}

[[nodiscard]] bool HasValidParticleCount(const blitzar_sim::SimConfigRun& config) noexcept
{
    return config.particle_count > 0 && static_cast<std::uintmax_t>(config.particle_count) <=
                                            std::numeric_limits<std::size_t>::max();
}

[[nodiscard]] blitzar_core::SnapshotFrameView MakeFrame(std::uint64_t step,
    blitzar_core::Scalar time, blitzar_core::ParticleOutputView state,
    std::span<const std::uint64_t> ids, std::uint32_t rank_count, std::uint32_t rank_index) noexcept
{
    blitzar_core::SnapshotHeader header{};

    header.particle_count = state.count;
    header.step = step;
    header.time = time;
    header.rank_count = rank_count;
    header.rank_index = rank_index;
    header.distribution = rank_count == 1U ? blitzar_core::SnapshotDistribution::SingleRank
                                           : blitzar_core::SnapshotDistribution::Sharded;

    header.id_policy = rank_count == 1U ? blitzar_core::SnapshotIdPolicy::GlobalContiguous
                                        : blitzar_core::SnapshotIdPolicy::GlobalStable;

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

[[nodiscard]] blitzar_status Synchronize(const blitzar_parallel::MpiContext& context,
    blitzar_status local_status, std::string_view phase) noexcept
{
    if (!context.IsDistributed()) {
        return local_status;
    }

    blitzar_status global_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status =
        context.SynchronizeStatus(local_status, "BlitzarOutput", phase, global_status);

    return synchronization_status == BLITZAR_STATUS_OK ? global_status : synchronization_status;
}

[[nodiscard]] blitzar_status ValidateMpiTopology(
    const blitzar_parallel::MpiContext& context) noexcept
{
    if (!context.IsUsable()) {
        return context.Status();
    }

    return context.Rank() >= 0 && context.Size() > 0 && context.Rank() < context.Size() &&
                   static_cast<std::uint64_t>(context.Size()) <=
                       static_cast<std::uint64_t>(blitzar_io::MetadataMaxRankIndex) + 1U
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INVALID_ARGUMENT;
}

[[nodiscard]] blitzar_physics::GravityParameters MakeGravity(
    const blitzar_sim::SimConfigRun& config) noexcept
{
    const blitzar_core::UnitSystem units{config.length_scale, config.mass_scale, config.time_scale};

    return {config.gravitational_constant, config.softening, units};
}

} // namespace

BlitzarOutput::BlitzarOutput(const blitzar_sim::SimConfigRun& config) noexcept
    : config_(config), mpi_()
{
}

blitzar_status BlitzarOutput::Prepare() noexcept
{
    if (prepared_) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_status topology_status = ValidateMpiTopology(mpi_);

    if (topology_status != BLITZAR_STATUS_OK) {
        return topology_status;
    }

    if (!config_.output.enabled) {
        if (config_.diagnostics.enabled) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        prepared_ = true;

        return BLITZAR_STATUS_OK;
    }

    if (!HasValidParticleCount(config_)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    if (mpi_.IsDistributed() && config_.diagnostics.enabled) {
        return BLITZAR_STATUS_UNSUPPORTED;
    }

    blitzar_status local_status = BLITZAR_STATUS_OK;

    try {
        const auto rank_count = static_cast<std::uint32_t>(mpi_.Size());
        const auto rank_index = static_cast<std::uint32_t>(mpi_.Rank());

        run_.emplace(config_.output.directory, MakeMetadataInfo(config_, rank_count, rank_index));

        local_status = run_->Prepare(!mpi_.IsDistributed() || mpi_.Rank() == 0);

        if (local_status == BLITZAR_STATUS_OK && config_.diagnostics.enabled) {
            diagnostics_.emplace(run_->DiagnosticsPath() / blitzar_io::ConservationFileName);

            local_status = diagnostics_->Prepare();
        }

        if (local_status != BLITZAR_STATUS_OK) {
            diagnostics_.reset();
            run_.reset();
        }
    }
    catch (const std::bad_alloc&) {
        diagnostics_.reset();
        run_.reset();

        local_status = BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    catch (const std::length_error&) {
        diagnostics_.reset();
        run_.reset();

        local_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::filesystem::filesystem_error&) {
        diagnostics_.reset();
        run_.reset();

        local_status = BLITZAR_STATUS_INTERNAL_ERROR;
    }

    const blitzar_status status = Synchronize(mpi_, local_status, "prepare");

    if (status != BLITZAR_STATUS_OK) {
        diagnostics_.reset();
        run_.reset();

        return status;
    }

    prepared_ = true;

    return BLITZAR_STATUS_OK;
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

bool BlitzarOutput::ShouldWriteDiagnostics(std::uint64_t step) const noexcept
{
    if (!config_.diagnostics.enabled || config_.diagnostics.every_steps <= 0 ||
        !config_.output.enabled) {
        return false;
    }

    const bool output_checkpoint =
        step == config_.StartStep() ? ShouldWriteInitial() : ShouldWriteStep(step);

    if (!output_checkpoint) {
        return false;
    }

    const std::uint64_t interval = static_cast<std::uint64_t>(config_.diagnostics.every_steps);

    return step % interval == 0U;
}

blitzar_status BlitzarOutput::Publish(std::uint64_t step, blitzar_core::ParticleOutputView state,
    std::span<const std::uint64_t> ids) noexcept
{
    const bool distributed = mpi_.IsDistributed();
    const bool valid =
        prepared_ && run_.has_value() && HasValidParticleCount(config_) &&
        (distributed ? state.count <= static_cast<std::size_t>(config_.particle_count)
                     : state.count == static_cast<std::size_t>(config_.particle_count)) &&
        ids.size() == state.count && blitzar_core::IsValid(state) && step >= config_.StartStep() &&
        step <= config_.FinalStep();

    blitzar_status status = Synchronize(
        mpi_, valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT, "publish-preflight");

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    const std::uint64_t start_step = config_.StartStep();
    const blitzar_core::Scalar time =
        step == start_step ? config_.StartTime()
                           : static_cast<blitzar_core::Scalar>(step) * config_.timestep;

    const blitzar_core::SnapshotFrameView frame = MakeFrame(step, time, state, ids,
        static_cast<std::uint32_t>(mpi_.Size()), static_cast<std::uint32_t>(mpi_.Rank()));

    status = run_->PublishSnapshot(frame);

    if (!distributed) {
        return status;
    }

    status = Synchronize(mpi_, status, "publish-shard");

    if (status != BLITZAR_STATUS_OK) {
        const blitzar_status cleanup_status = run_->DiscardDistributedSnapshot(step);

        (void)Synchronize(mpi_, cleanup_status, "publish-cleanup");

        return status;
    }

    const blitzar_status commit_status =
        mpi_.Rank() == 0 ? run_->CommitDistributedSnapshot(step) : BLITZAR_STATUS_OK;

    status = Synchronize(mpi_, commit_status, "publish-manifest");

    if (status != BLITZAR_STATUS_OK) {
        const blitzar_status cleanup_status = run_->DiscardDistributedSnapshot(step);

        (void)Synchronize(mpi_, cleanup_status, "publish-cleanup");
    }

    return status;
}

blitzar_status BlitzarOutput::PublishDiagnostics(
    std::uint64_t step, blitzar_core::ParticleOutputView state) noexcept
{
    if (!prepared_ || !diagnostics_.has_value() || !HasValidParticleCount(config_) ||
        state.count != static_cast<std::size_t>(config_.particle_count) ||
        !blitzar_core::IsValid(state) || !ShouldWriteDiagnostics(step)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_core::ParticleStateView input{state.count,
        std::span<const blitzar_core::Scalar>(state.x),
        std::span<const blitzar_core::Scalar>(state.y),
        std::span<const blitzar_core::Scalar>(state.z),
        std::span<const blitzar_core::Scalar>(state.velocity_x),
        std::span<const blitzar_core::Scalar>(state.velocity_y),
        std::span<const blitzar_core::Scalar>(state.velocity_z),
        std::span<const blitzar_core::Scalar>(state.mass), state.count};

    const blitzar_physics::GravityParameters gravity = MakeGravity(config_);

    blitzar_physics::ConservationMetrics metrics{};
    const blitzar_status metrics_status =
        blitzar_physics::ComputeConservationMetrics(input, gravity, metrics);

    if (metrics_status != BLITZAR_STATUS_OK) {
        return metrics_status;
    }

    const std::uint64_t start_step = config_.StartStep();
    const blitzar_core::Scalar time =
        step == start_step ? config_.StartTime()
                           : static_cast<blitzar_core::Scalar>(step) * config_.timestep;

    const blitzar_sim::SimConfigDiagnostics& diagnostics = config_.diagnostics;

    return diagnostics_->Append({step, time, state.count, metrics, diagnostics.energy,
        diagnostics.momentum, diagnostics.relative_error});
}

blitzar_status BlitzarOutput::SynchronizeStatus(
    blitzar_status local_status, std::string_view phase) const noexcept
{
    return Synchronize(mpi_, local_status, phase);
}

std::size_t BlitzarOutput::SnapshotCount() const noexcept
{
    return run_.has_value() ? run_->CompletedOutputCount() : 0U;
}

std::size_t BlitzarOutput::DiagnosticsCount() const noexcept
{
    return diagnostics_.has_value() ? diagnostics_->RecordCount() : 0U;
}

const std::filesystem::path& BlitzarOutput::OutputPath() const noexcept
{
    return config_.output.directory;
}

bool BlitzarOutput::IsSummaryOwner() const noexcept
{
    return !mpi_.IsDistributed() || mpi_.Rank() == 0;
}

} // namespace blitzar_cli
