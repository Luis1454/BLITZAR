#ifndef BLITZAR_APPS_BLITZAR_BLITZAR_SUMMARY_HPP
#define BLITZAR_APPS_BLITZAR_BLITZAR_SUMMARY_HPP

#include <blitzar/blitzar.h>
#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <string_view>

namespace blitzar_cli {

enum class BlitzarExitCode : int {
    Success = 0,
    Usage = 2,
    Configuration = 3,
    Runtime = 4,
    Output = 5
};

struct BlitzarSummary final {
    static constexpr std::uint32_t SchemaVersion = 1;

    blitzar_status status{BLITZAR_STATUS_OK};
    std::uint64_t requested_steps{};
    std::uint64_t completed_steps{};
    std::uint64_t particle_count{};
    blitzar_solver_kind solver{BLITZAR_SOLVER_DIRECT};
    std::uint64_t snapshot_count{};
    std::uint64_t diagnostics_count{};
    std::filesystem::path output_path;
};

struct BlitzarFailure final {
    blitzar_status status{BLITZAR_STATUS_INTERNAL_ERROR};
    std::string_view phase;
    BlitzarExitCode exit_code{BlitzarExitCode::Runtime};
};

[[nodiscard]] bool WriteSummary(std::ostream& output, const BlitzarSummary& summary);
[[nodiscard]] bool WriteFailure(std::ostream& output, const BlitzarFailure& failure) noexcept;

} // namespace blitzar_cli

#endif
