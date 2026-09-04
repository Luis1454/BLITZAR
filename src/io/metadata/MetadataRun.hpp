#ifndef BLITZAR_IO_METADATA_METADATA_RUN_HPP
#define BLITZAR_IO_METADATA_METADATA_RUN_HPP

#include "io/hdf5/Hdf5Writer.hpp"
#include "io/metadata/MetadataManifest.hpp"
#include "io/snapshot/SnapshotWriter.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace blitzar_io {

class MetadataRun final {
public:
    explicit MetadataRun(std::filesystem::path root, MetadataRunInfo info);

    [[nodiscard]] blitzar_status Prepare() noexcept;
    [[nodiscard]] blitzar_status PublishSnapshot(blitzar_core::SnapshotFrameView frame) noexcept;
    [[nodiscard]] std::filesystem::path StatePath(std::uint64_t step) const;
    [[nodiscard]] const std::filesystem::path& Root() const noexcept;
    [[nodiscard]] const std::filesystem::path& ManifestPath() const noexcept;
    [[nodiscard]] const std::filesystem::path& StatesPath() const noexcept;
    [[nodiscard]] const std::filesystem::path& DiagnosticsPath() const noexcept;
    [[nodiscard]] std::size_t CompletedOutputCount() const noexcept;

private:
    [[nodiscard]] blitzar_status ValidateFrame(
        blitzar_core::SnapshotFrameView frame) const noexcept;
    [[nodiscard]] bool HasCompletedStep(std::uint64_t step) const noexcept;

    std::filesystem::path root_;
    std::filesystem::path manifest_path_;
    std::filesystem::path states_path_;
    std::filesystem::path diagnostics_path_;
    MetadataManifest manifest_;
    SnapshotWriter snapshot_writer_;
    Hdf5Writer hdf5_writer_;
    std::vector<std::uint64_t> completed_steps_;
    bool prepared_{};
};

} // namespace blitzar_io

#endif
