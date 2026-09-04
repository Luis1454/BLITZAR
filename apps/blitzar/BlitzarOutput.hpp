#ifndef BLITZAR_APPS_BLITZAR_BLITZAR_OUTPUT_HPP
#define BLITZAR_APPS_BLITZAR_BLITZAR_OUTPUT_HPP

#include "core/CoreTypes.hpp"
#include "io/diagnostics/ConservationCsv.hpp"
#include "io/metadata/MetadataRun.hpp"
#include "mpi/runtime/MpiContext.hpp"
#include "simulation/config/SimConfigRun.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string_view>

namespace blitzar_cli {

class BlitzarOutput final {
public:
    explicit BlitzarOutput(const blitzar_sim::SimConfigRun& config) noexcept;

    [[nodiscard]] blitzar_status Prepare() noexcept;
    [[nodiscard]] bool ShouldWriteInitial() const noexcept;
    [[nodiscard]] bool ShouldWriteStep(std::uint64_t step) const noexcept;
    [[nodiscard]] bool ShouldWriteDiagnostics(std::uint64_t step) const noexcept;
    [[nodiscard]] blitzar_status Publish(std::uint64_t step, blitzar_core::ParticleOutputView state,
        std::span<const std::uint64_t> ids) noexcept;
    [[nodiscard]] blitzar_status PublishDiagnostics(
        std::uint64_t step, blitzar_core::ParticleOutputView state) noexcept;
    [[nodiscard]] blitzar_status SynchronizeStatus(
        blitzar_status local_status, std::string_view phase) const noexcept;
    [[nodiscard]] std::size_t SnapshotCount() const noexcept;
    [[nodiscard]] std::size_t DiagnosticsCount() const noexcept;
    [[nodiscard]] const std::filesystem::path& OutputPath() const noexcept;
    [[nodiscard]] bool IsSummaryOwner() const noexcept;

private:
    const blitzar_sim::SimConfigRun& config_;
    blitzar_parallel::MpiContext mpi_;
    std::optional<blitzar_io::MetadataRun> run_;
    std::optional<blitzar_io::ConservationCsv> diagnostics_;
    bool prepared_{};
};

} // namespace blitzar_cli

#endif
