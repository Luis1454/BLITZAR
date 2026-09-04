#include "io/metadata/MetadataManifest.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <fstream>
#include <ios>
#include <limits>
#include <new>
#include <system_error>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#endif

namespace blitzar_io {

namespace {

[[nodiscard]] bool IsFinitePositive(double value) noexcept
{
    return std::isfinite(value) && value > 0.0;
}

[[nodiscard]] bool IsFiniteNonNegative(double value) noexcept
{
    return std::isfinite(value) && value >= 0.0;
}

[[nodiscard]] bool HasValidCompletedSteps(
    std::span<const std::uint64_t> completed_steps, std::uint64_t requested_steps) noexcept
{
    if (completed_steps.size() > requested_steps + 1U) {
        return false;
    }

    std::uint64_t previous = 0;
    bool has_previous = false;

    for (const std::uint64_t step : completed_steps) {
        if (step > requested_steps || step > MetadataMaxStateStep ||
            (has_previous && step <= previous)) {
            return false;
        }

        previous = step;
        has_previous = true;
    }

    return true;
}

[[nodiscard]] bool ReplaceFileAtomically(const std::filesystem::path& temporary,
    const std::filesystem::path& target, std::error_code& error)
{
#ifdef _WIN32
    if (MoveFileExW(temporary.c_str(), target.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0) {
        error.clear();

        return true;
    }

    error = std::error_code(static_cast<int>(GetLastError()), std::system_category());

    return false;
#else

    std::filesystem::rename(temporary, target, error);

    return !error;
#endif
}

} // namespace

blitzar_status MetadataExecution::Validate() const noexcept
{
    if (ExecutionModeName(mode).empty() || !cpu.IsValid() || !hip.IsValid() || !mpi.IsValid() ||
        precision != "float64" || compiler.empty() || device.empty() || rng.empty() ||
        compensator.empty() || ordering.empty()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const bool expected_bitwise_reproducible =
        mode == blitzar_core::ExecutionMode::Strict && cpu.IsBitwiseReproducible() &&
        hip.IsBitwiseReproducible() && mpi.IsBitwiseReproducible();

    return bitwise_reproducible == expected_bitwise_reproducible ? BLITZAR_STATUS_OK
                                                                 : BLITZAR_STATUS_INVALID_ARGUMENT;
}

blitzar_status MetadataRunInfo::Validate() const noexcept
{
    const MetadataSimulation& simulation = configuration.simulation;
    const MetadataGravity& gravity = configuration.gravity;
    const MetadataUnits& units = configuration.units;
    const MetadataBarnesHut& barnes_hut = configuration.barnes_hut;
    const MetadataExecution& execution = configuration.execution;
    const MetadataOutput& output = configuration.output;
    const MetadataDiagnostics& diagnostics = configuration.diagnostics;

    if (product_version.empty() || plan_version.empty() || simulation.particle_count == 0 ||
        simulation.particle_count > blitzar_core::SnapshotMaxParticleCount ||
        simulation.requested_steps == 0 || simulation.requested_steps > MetadataMaxStepCount ||
        !IsFinitePositive(simulation.timestep) ||
        !IsFinitePositive(gravity.gravitational_constant) ||
        !IsFiniteNonNegative(gravity.softening) || !IsFinitePositive(units.length_scale) ||
        !IsFinitePositive(units.mass_scale) || !IsFinitePositive(units.time_scale) ||
        !IsFiniteNonNegative(barnes_hut.opening_angle) || barnes_hut.max_particles == 0 ||
        barnes_hut.max_particles < simulation.particle_count || barnes_hut.max_cells == 0 ||
        barnes_hut.leaf_capacity == 0 || barnes_hut.max_depth == 0) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    if (configuration.generation.deterministic == false) {
        return BLITZAR_STATUS_UNSUPPORTED;
    }

    const blitzar_status execution_status = execution.Validate();

    if (execution_status != BLITZAR_STATUS_OK) {
        return execution_status;
    }

    if (output.enabled && output.every_steps == 0) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    if (MetadataOutputFormatName(output.format).empty()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    if (diagnostics.enabled &&
        (diagnostics.every_steps == 0 ||
            (!diagnostics.energy && !diagnostics.momentum && !diagnostics.relative_error))) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    if (rank_count == 0 || rank_index >= rank_count || rank_index > MetadataMaxRankIndex ||
        rank_count > MetadataMaxRankIndex + 1U) {
        return BLITZAR_STATUS_UNSUPPORTED;
    }

    switch (simulation.solver) {
    case BLITZAR_SOLVER_DIRECT:
    case BLITZAR_SOLVER_BARNES_HUT:
    case BLITZAR_SOLVER_FMM:
    case BLITZAR_SOLVER_KIFMM:

        break;

    case BLITZAR_SOLVER_PM:
    case BLITZAR_SOLVER_TREEPM:

        return BLITZAR_STATUS_UNSUPPORTED;

    default:

        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    if (simulation.integrator != BLITZAR_INTEGRATOR_LEAPFROG_KDK) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return BLITZAR_STATUS_OK;
}

std::string StateFileName(std::uint64_t step)
{
    return StateFileName(step, MetadataOutputFormat::Binary);
}

std::string StateFileName(std::uint64_t step, MetadataOutputFormat format)
{
    if (step > MetadataMaxStateStep) {
        return {};
    }

    std::array<char, 8> digits{};
    const auto conversion = std::to_chars(digits.data(), digits.data() + digits.size(), step);

    if (conversion.ec != std::errc{}) {
        return {};
    }

    const std::size_t digit_count = static_cast<std::size_t>(conversion.ptr - digits.data());
    std::string name{"state-"};

    name.append(digits.size() - digit_count, '0');
    name.append(digits.data(), digit_count);

    switch (format) {
    case MetadataOutputFormat::Binary:

        name.append(".bin");

        break;

    case MetadataOutputFormat::Hdf5:

        name.append(".h5");

        break;

    default:

        return {};
    }

    return name;
}

std::string StateShardFileName(
    std::uint64_t step, std::uint32_t rank_index, MetadataOutputFormat format)
{
    if (rank_index > MetadataMaxRankIndex) {
        return {};
    }

    const std::string state_name = StateFileName(step, format);

    if (state_name.empty()) {
        return {};
    }

    std::array<char, 8> digits{};
    const auto conversion = std::to_chars(
        digits.data(), digits.data() + digits.size(), static_cast<std::uint64_t>(rank_index));

    if (conversion.ec != std::errc{}) {
        return {};
    }

    const std::size_t digit_count = static_cast<std::size_t>(conversion.ptr - digits.data());
    const std::size_t extension_position = state_name.find_last_of('.');
    std::string name = state_name.substr(0, extension_position);

    name.append(".rank-");
    name.append(digits.size() - digit_count, '0');
    name.append(digits.data(), digit_count);
    name.append(state_name.substr(extension_position));

    return name;
}

std::string CurrentCompilerIdentity()
{
#if defined(__clang__)
    return "clang-" + std::to_string(__clang_major__) + "." + std::to_string(__clang_minor__) +
           "." + std::to_string(__clang_patchlevel__);
#elif defined(__GNUC__)

    return "gcc-" + std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__) + "." +
           std::to_string(__GNUC_PATCHLEVEL__);
#elif defined(_MSC_VER)

    return "msvc-" + std::to_string(_MSC_VER);
#else
    return "unknown";
#endif
}

std::string_view MetadataOutputFormatName(MetadataOutputFormat format) noexcept
{
    switch (format) {
    case MetadataOutputFormat::Binary:

        return "binary";

    case MetadataOutputFormat::Hdf5:

        return "hdf5";

    default:

        return {};
    }
}

bool ParseMetadataOutputFormat(std::string_view text, MetadataOutputFormat& format) noexcept
{
    if (text == "binary") {
        format = MetadataOutputFormat::Binary;

        return true;
    }

    if (text == "hdf5") {
        format = MetadataOutputFormat::Hdf5;

        return true;
    }

    return false;
}

MetadataManifest::MetadataManifest(MetadataRunInfo info) : info_(std::move(info)) {}

const MetadataRunInfo& MetadataManifest::Info() const noexcept
{
    return info_;
}

blitzar_status MetadataManifest::WriteFile(
    const std::filesystem::path& path, std::span<const std::uint64_t> completed_steps) const
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);

    if (!output.is_open() || !WriteDocument(output, completed_steps)) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    output.flush();

    return output ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INTERNAL_ERROR;
}

blitzar_status MetadataManifest::WriteAtomic(
    const std::filesystem::path& path, std::span<const std::uint64_t> completed_steps) const
{
    if (path.empty()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_status info_status = Info().Validate();

    if (info_status != BLITZAR_STATUS_OK) {
        return info_status;
    }

    if (!HasValidCompletedSteps(completed_steps, info_.configuration.simulation.requested_steps)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    try {
        std::filesystem::path temporary = path;

        temporary += ".tmp";

        std::error_code status_error;

        if (std::filesystem::exists(temporary, status_error) || status_error) {
            return status_error ? BLITZAR_STATUS_INTERNAL_ERROR : BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        const blitzar_status write_status = WriteFile(temporary, completed_steps);

        if (write_status != BLITZAR_STATUS_OK) {
            std::error_code cleanup_error;

            std::filesystem::remove(temporary, cleanup_error);

            return write_status;
        }

        std::error_code replace_error;

        if (!ReplaceFileAtomically(temporary, path, replace_error)) {
            std::error_code cleanup_error;

            std::filesystem::remove(temporary, cleanup_error);

            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        return BLITZAR_STATUS_OK;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::ios_base::failure&) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
}

} // namespace blitzar_io
