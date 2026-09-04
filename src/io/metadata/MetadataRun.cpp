#include "io/metadata/MetadataRun.hpp"

#include <algorithm>
#include <filesystem>
#include <new>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace blitzar_io {

namespace {

[[nodiscard]] bool IsEmptyDirectory(const std::filesystem::path& path, std::error_code& error)
{
    if (!std::filesystem::is_directory(path, error) || error) {
        return false;
    }

    const std::filesystem::directory_iterator end;
    const std::filesystem::directory_iterator first(path, error);

    return !error && first == end;
}

[[nodiscard]] blitzar_status PrepareRoot(const std::filesystem::path& path, bool& created)
{
    if (path.empty()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    std::error_code status_error;
    const std::filesystem::file_status status = std::filesystem::symlink_status(path, status_error);

    if (status_error &&
        status_error != std::make_error_code(std::errc::no_such_file_or_directory)) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    status_error.clear();

    if (!std::filesystem::exists(status)) {
        std::filesystem::create_directories(path, status_error);

        if (status_error) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        created = true;

        return BLITZAR_STATUS_OK;
    }

    if (!std::filesystem::is_directory(status)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    if (!IsEmptyDirectory(path, status_error)) {
        return status_error ? BLITZAR_STATUS_INTERNAL_ERROR : BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return BLITZAR_STATUS_OK;
}

[[nodiscard]] blitzar_status CreateChildDirectory(const std::filesystem::path& path)
{
    std::error_code create_error;
    const bool created = std::filesystem::create_directory(path, create_error);

    if (create_error) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    return created ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT;
}

void CleanupDirectories(const std::filesystem::path& root, const std::filesystem::path& states,
    const std::filesystem::path& diagnostics, bool root_created) noexcept
{
    std::error_code cleanup_error;

    std::filesystem::remove(diagnostics, cleanup_error);
    cleanup_error.clear();
    std::filesystem::remove(states, cleanup_error);

    if (root_created) {
        cleanup_error.clear();
        std::filesystem::remove(root, cleanup_error);
    }
}

} // namespace

MetadataRun::MetadataRun(std::filesystem::path root, MetadataRunInfo info)
    : root_(std::move(root).lexically_normal()), manifest_path_(root_ / "manifest.json"),
      states_path_(root_ / "states"), diagnostics_path_(root_ / "diagnostics"),
      manifest_(std::move(info)),
      snapshot_writer_(static_cast<std::size_t>(blitzar_core::SnapshotMaxParticleCount)),
      hdf5_writer_(static_cast<std::size_t>(blitzar_core::SnapshotMaxParticleCount))
{
    const std::uint64_t requested_steps =
        std::min(manifest_.Info().configuration.simulation.requested_steps, MetadataMaxStepCount);

    completed_steps_.reserve(static_cast<std::size_t>(requested_steps + 1U));
}

blitzar_status MetadataRun::Prepare() noexcept
{
    return Prepare(true);
}

blitzar_status MetadataRun::Prepare(bool manifest_owner) noexcept
{
    if (prepared_) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_status info_status = manifest_.Info().Validate();

    if (info_status != BLITZAR_STATUS_OK) {
        return info_status;
    }

    if (manifest_.Info().configuration.output.format == MetadataOutputFormat::Hdf5 &&
        !Hdf5Writer::IsAvailable()) {
        return BLITZAR_STATUS_UNSUPPORTED;
    }

    if (manifest_.Info().rank_count == 1U && !manifest_owner) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    if (manifest_.Info().rank_count > 1U && manifest_owner && manifest_.Info().rank_index != 0U) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    if (!manifest_owner) {
        prepared_ = true;

        return BLITZAR_STATUS_OK;
    }

    bool root_created = false;

    try {
        const blitzar_status root_status = PrepareRoot(root_, root_created);

        if (root_status != BLITZAR_STATUS_OK) {
            return root_status;
        }

        const blitzar_status states_status = CreateChildDirectory(states_path_);

        if (states_status != BLITZAR_STATUS_OK) {
            CleanupDirectories(root_, states_path_, diagnostics_path_, root_created);

            return states_status;
        }

        const blitzar_status diagnostics_status = CreateChildDirectory(diagnostics_path_);

        if (diagnostics_status != BLITZAR_STATUS_OK) {
            CleanupDirectories(root_, states_path_, diagnostics_path_, root_created);

            return diagnostics_status;
        }

        const blitzar_status manifest_status =
            manifest_.WriteAtomic(manifest_path_, std::span<const std::uint64_t>{});

        if (manifest_status != BLITZAR_STATUS_OK) {
            CleanupDirectories(root_, states_path_, diagnostics_path_, root_created);

            return manifest_status;
        }

        prepared_ = true;

        return BLITZAR_STATUS_OK;
    }
    catch (const std::bad_alloc&) {
        CleanupDirectories(root_, states_path_, diagnostics_path_, root_created);

        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    catch (const std::length_error&) {
        CleanupDirectories(root_, states_path_, diagnostics_path_, root_created);

        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
}

std::filesystem::path MetadataRun::StatePath(std::uint64_t step) const
{
    const MetadataRunInfo& info = manifest_.Info();

    const std::string file_name =
        info.rank_count == 1U
            ? StateFileName(step, info.configuration.output.format)
            : StateShardFileName(step, info.rank_index, info.configuration.output.format);

    return states_path_ / file_name;
}

blitzar_status MetadataRun::ValidateFrame(blitzar_core::SnapshotFrameView frame) const noexcept
{
    const blitzar_status frame_status = frame.Validate();

    if (frame_status != BLITZAR_STATUS_OK) {
        return frame_status;
    }

    const MetadataRunInfo& info = manifest_.Info();
    const MetadataSimulation& simulation = info.configuration.simulation;

    const bool single_rank = info.rank_count == 1U;
    const bool distribution_valid =
        single_rank ? frame.header.distribution == blitzar_core::SnapshotDistribution::SingleRank &&
                          frame.header.particle_count == simulation.particle_count
                    : frame.header.distribution == blitzar_core::SnapshotDistribution::Sharded &&
                          frame.header.id_policy == blitzar_core::SnapshotIdPolicy::GlobalStable &&
                          frame.header.particle_count <= simulation.particle_count;

    if (!distribution_valid || frame.header.step > simulation.requested_steps ||
        frame.header.rank_count != info.rank_count || frame.header.rank_index != info.rank_index) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return BLITZAR_STATUS_OK;
}

bool MetadataRun::HasCompletedStep(std::uint64_t step) const noexcept
{
    return std::find(completed_steps_.begin(), completed_steps_.end(), step) !=
           completed_steps_.end();
}

blitzar_status MetadataRun::PublishSnapshot(blitzar_core::SnapshotFrameView frame) noexcept
{
    if (!prepared_) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_status frame_status = ValidateFrame(frame);

    if (frame_status != BLITZAR_STATUS_OK || HasCompletedStep(frame.header.step) ||
        (!completed_steps_.empty() && frame.header.step <= completed_steps_.back())) {
        return frame_status != BLITZAR_STATUS_OK ? frame_status : BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    try {
        const std::filesystem::path destination = StatePath(frame.header.step);
        const blitzar_status write_status =
            manifest_.Info().configuration.output.format == MetadataOutputFormat::Hdf5
                ? hdf5_writer_.WriteAtomic(destination, frame)
                : snapshot_writer_.WriteAtomic(destination, frame);

        if (write_status != BLITZAR_STATUS_OK) {
            return write_status;
        }

        if (manifest_.Info().rank_count > 1U) {
            return BLITZAR_STATUS_OK;
        }

        completed_steps_.push_back(frame.header.step);

        const blitzar_status manifest_status =
            manifest_.WriteAtomic(manifest_path_, completed_steps_);

        if (manifest_status != BLITZAR_STATUS_OK) {
            completed_steps_.pop_back();

            std::error_code cleanup_error;

            std::filesystem::remove(destination, cleanup_error);

            return manifest_status;
        }

        return BLITZAR_STATUS_OK;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
}

blitzar_status MetadataRun::CommitDistributedSnapshot(std::uint64_t step) noexcept
{
    if (!prepared_ || manifest_.Info().rank_count <= 1U || manifest_.Info().rank_index != 0U ||
        HasCompletedStep(step) || (!completed_steps_.empty() && step <= completed_steps_.back())) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    try {
        completed_steps_.push_back(step);

        const blitzar_status manifest_status =
            manifest_.WriteAtomic(manifest_path_, completed_steps_);

        if (manifest_status != BLITZAR_STATUS_OK) {
            completed_steps_.pop_back();

            return manifest_status;
        }

        return BLITZAR_STATUS_OK;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
}

blitzar_status MetadataRun::DiscardDistributedSnapshot(std::uint64_t step) noexcept
{
    if (!prepared_ || manifest_.Info().rank_count <= 1U) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    std::error_code remove_error;

    std::filesystem::remove(StatePath(step), remove_error);

    if (remove_error) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    return BLITZAR_STATUS_OK;
}

const std::filesystem::path& MetadataRun::Root() const noexcept
{
    return root_;
}

const std::filesystem::path& MetadataRun::ManifestPath() const noexcept
{
    return manifest_path_;
}

const std::filesystem::path& MetadataRun::StatesPath() const noexcept
{
    return states_path_;
}

const std::filesystem::path& MetadataRun::DiagnosticsPath() const noexcept
{
    return diagnostics_path_;
}

std::size_t MetadataRun::CompletedOutputCount() const noexcept
{
    return completed_steps_.size();
}

} // namespace blitzar_io
