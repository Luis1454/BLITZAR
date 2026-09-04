#ifndef BLITZAR_IO_POSTPROCESS_POST_PROCESS_VALIDATION_HPP
#define BLITZAR_IO_POSTPROCESS_POST_PROCESS_VALIDATION_HPP

#include "io/metadata/MetadataManifest.hpp"

#include <blitzar/blitzar.h>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace blitzar_io {

struct PostProcessInput final {
    MetadataRunInfo info;
    std::vector<std::uint64_t> completed_steps;
    std::filesystem::path states_path;
};

[[nodiscard]] blitzar_status ReadPostProcessInput(
    const std::filesystem::path& run_directory, PostProcessInput& input) noexcept;
[[nodiscard]] blitzar_status ValidateSnapshotFrame(const blitzar_core::SnapshotHeader& header,
    const MetadataRunInfo& info, std::uint64_t expected_step,
    std::uint32_t expected_rank = 0U) noexcept;
[[nodiscard]] bool ShouldWriteDiagnostic(const MetadataRunInfo& info, std::uint64_t step) noexcept;

} // namespace blitzar_io

#endif
