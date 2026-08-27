#ifndef BLITZAR_IO_SNAPSHOT_SNAPSHOT_WRITER_HPP
#define BLITZAR_IO_SNAPSHOT_SNAPSHOT_WRITER_HPP

#include "core/CoreSnapshot.hpp"

#include <cstddef>
#include <filesystem>

namespace blitzar_io {

class SnapshotWriter final {
public:
    explicit SnapshotWriter(std::size_t max_particle_count) noexcept;

    [[nodiscard]] blitzar_status Write(
        const std::filesystem::path& path, blitzar_core::SnapshotFrameView frame) const;

private:
    std::size_t max_particle_count_;
};

} // namespace blitzar_io

#endif
