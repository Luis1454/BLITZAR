#include "io/postprocess/PostProcessInput.hpp"

#include "io/metadata/MetadataReader.hpp"

#include <blitzar/blitzar.h>
#include <filesystem>
#include <limits>
#include <new>
#include <set>
#include <string>
#include <system_error>

namespace blitzar_io {

namespace {

[[nodiscard]] bool IsCurrentVersion(const MetadataRunInfo& info) noexcept
{
    return info.product_version == blitzar_version() && info.plan_version == blitzar_plan_version();
}

[[nodiscard]] bool HasValidSteps(
    const std::vector<std::uint64_t>& steps, std::uint64_t requested_steps) noexcept
{
    if (steps.empty() || steps.size() > requested_steps + 1U) {
        return false;
    }

    std::uint64_t previous{};
    bool has_previous = false;

    for (const std::uint64_t step : steps) {
        if (step > requested_steps || step > MetadataMaxStateStep ||
            (has_previous && step <= previous)) {
            return false;
        }

        previous = step;
        has_previous = true;
    }

    return true;
}

[[nodiscard]] blitzar_status ValidateStatesDirectory(const std::filesystem::path& path,
    const std::vector<std::uint64_t>& steps, MetadataOutputFormat format) noexcept
{
    std::error_code status_error;
    const std::filesystem::file_status status = std::filesystem::symlink_status(path, status_error);

    if (status_error || !std::filesystem::is_directory(status)) {
        return status_error ? BLITZAR_STATUS_INTERNAL_ERROR : BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    try {
        std::set<std::string> expected_names;

        for (const std::uint64_t step : steps) {
            const std::string name = StateFileName(step, format);

            if (name.empty()) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }

            expected_names.insert(name);
        }

        const std::filesystem::directory_iterator end;
        std::filesystem::directory_iterator entry(path, status_error);

        if (status_error) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        for (; entry != end; entry.increment(status_error)) {
            if (status_error) {
                return BLITZAR_STATUS_INTERNAL_ERROR;
            }

            const std::filesystem::file_status entry_status = entry->symlink_status(status_error);

            if (status_error) {
                return BLITZAR_STATUS_INTERNAL_ERROR;
            }

            if (!std::filesystem::is_regular_file(entry_status) ||
                expected_names.erase(entry->path().filename().string()) == 0U) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }
        }

        return expected_names.empty() ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
}

} // namespace

blitzar_status ReadPostProcessInput(
    const std::filesystem::path& run_directory, PostProcessInput& input) noexcept
{
    if (run_directory.empty()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    std::error_code status_error;
    const std::filesystem::file_status status =
        std::filesystem::symlink_status(run_directory, status_error);

    if (status_error || !std::filesystem::is_directory(status)) {
        return status_error ? BLITZAR_STATUS_INTERNAL_ERROR : BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    try {
        MetadataReader reader;
        const blitzar_status metadata_status =
            reader.Read(run_directory / "manifest.json", input.info, input.completed_steps);

        if (metadata_status != BLITZAR_STATUS_OK) {
            return metadata_status;
        }

        const blitzar_status info_status = input.info.Validate();

        if (info_status != BLITZAR_STATUS_OK) {
            return info_status;
        }

        if (!IsCurrentVersion(input.info) || !input.info.configuration.output.enabled ||
            !HasValidSteps(
                input.completed_steps, input.info.configuration.simulation.requested_steps)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        input.states_path = run_directory / "states";

        return ValidateStatesDirectory(
            input.states_path, input.completed_steps, input.info.configuration.output.format);
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
}

blitzar_status ValidateSnapshotFrame(const blitzar_core::SnapshotHeader& header,
    const MetadataRunInfo& info, std::uint64_t expected_step) noexcept
{
    const MetadataSimulation& simulation = info.configuration.simulation;

    const double expected_time = static_cast<double>(expected_step) * simulation.timestep;

    if (header.particle_count != simulation.particle_count || header.step != expected_step ||
        header.time != expected_time || header.rank_count != info.rank_count ||
        header.rank_index != info.rank_index) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return BLITZAR_STATUS_OK;
}

bool ShouldWriteDiagnostic(const MetadataRunInfo& info, std::uint64_t step) noexcept
{
    const MetadataDiagnostics& diagnostics = info.configuration.diagnostics;

    return !diagnostics.enabled || step % diagnostics.every_steps == 0U;
}

} // namespace blitzar_io
